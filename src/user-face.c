#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <sys/statfs.h>
#include <dirent.h>
#include <sys/stat.h>
#include "user.h"
#include "user-face.h"
#include "user-share.h"
static char gcPicBuf[PICMAX][50];   //照片
/******************************************************************************
* 函数:              UpdataFace
*
* 说明:  更新用户头像
*
* 输入:
*        选择头像的下标号
*
* 返回:
*
* 作者:  zhuyaliang  09/05/2018
******************************************************************************/
static void UpdataFace(int nCount,UserAdmin *ua)
{
    GtkWidget *image;
    GdkPixbuf *face;

    GdkPixbuf *pb, *pb2;
    char BasePath[100] = { 0 };
    sprintf(BasePath, FACEDIR"/%s", gcPicBuf[nCount]);
    /*设置button上的图片*/
    pb = gdk_pixbuf_new_from_file(BasePath,NULL);
    pb2 = gdk_pixbuf_scale_simple (pb,96,96, GDK_INTERP_BILINEAR);
    image = gtk_image_new_from_pixbuf(pb2);
    gtk_button_set_image(GTK_BUTTON(ua->ButtonFace),image);
    /*设置列表用户头像尺寸*/
    face = SetUserFaceSize (BasePath, 50);

    gtk_list_store_set(ua->ListSTore,&ua->ul[gnCurrentUserIndex].Iter,
                      COL_USER_FACE, face,
                      LIST_COLOR,"black",
                      LIST_FRONT,1985,-1);
    act_user_set_icon_file (ua->ul[gnCurrentUserIndex].User,BasePath);
    memset(ua->ul[gnCurrentUserIndex].UserIcon,
          '\0',
          strlen(ua->ul[gnCurrentUserIndex].UserIcon));

    memcpy(ua->ul[gnCurrentUserIndex].UserIcon,BasePath,strlen(BasePath));

}
static void Unbind(gpointer data)
{
    UserAdmin *ua = (UserAdmin *) data;
    gtk_widget_destroy(ua->IconWindow);
    if(ua->MouseId != 0)
    {    
        g_signal_handler_disconnect(ua->MainWindow,ua->MouseId);
    ua->MouseId = 0;
    }
    if(ua->KeyId != 0)
    {        
        g_signal_handler_disconnect(ua->MainWindow,ua->KeyId);
        ua->KeyId = 0;
    }    
    ua->IconWindow = NULL; 

}        
/******************************************************************************
* 函数:              face_widget_activated
*
* 说明:  选择用户头像相应函数
*
* 输入:
*
*
* 返回:
*
* 作者:  zhuyaliang  09/05/2018
******************************************************************************/
static void face_widget_activated (GtkFlowBox *flowbox,
                                   GtkFlowBoxChild *child,
                                   gpointer data)
{
    UserAdmin *ua = (UserAdmin *) data;
    int Count;

   /* 获得选择的照片的下标号*/
    Count = gtk_flow_box_child_get_index(child);
    /* 更新头像 */
    UpdataFace(Count,ua);
    Unbind(ua);
}

/******************************************************************************
* 函数:              NotifyEvents       
*        
* 说明:  用户点击空白处关闭头像选择窗口
*        
* 输入:  		
*       
*        
* 返回:  
*        
* 作者:  zhuyaliang  09/05/2018
******************************************************************************/ 
static gboolean NotifyEvents(GtkWidget *widget,
                             GdkEventButton *event, 
                             gpointer data)
{

    UserAdmin *ua = (UserAdmin *) data;

    if(ua->IconWindow  != NULL )
    {
        Unbind(ua);
    }        
    return TRUE;
}        
/******************************************************************************
* 函数:              GetFaceFile
*
* 说明:  获得照片并将照片缩小
*
* 输入:
*        照片路径
*
* 返回:
*
* 作者:  zhuyaliang  09/05/2018
******************************************************************************/
static GtkWidget *GetFaceFile (const char *FileName)
{
    char Path[300] = { 0 };
    GtkWidget *image;
    GdkPixbuf *src;
    GdkPixbuf *dst;
    sprintf(Path, "%s/%s",FACEDIR, FileName);
    src = gdk_pixbuf_new_from_file(Path, NULL);
    dst = gdk_pixbuf_scale_simple(src,70,70,GDK_INTERP_BILINEAR);
    image = gtk_image_new_from_pixbuf(dst);

    gtk_widget_show_all (image);

    return image;
}

/******************************************************************************
* 函数:              GetDirFace
*
* 说明:  得到目录下所有可用的照片
*
* 输入:
*        排列盒
*
* 返回:
*
* 作者:  zhuyaliang  09/05/2018
******************************************************************************/
static int GetDirFace(GtkWidget *flowbox)
{
    DIR *dp;
    struct stat statbuf;
    struct dirent *entry;
    int cnt = 0;

    if((dp = opendir(FACEDIR)) == NULL)
	    return -2;

    while((entry = readdir(dp)) != NULL)
    {
        lstat(entry->d_name,&statbuf);
        if(S_ISDIR(statbuf.st_mode))
        {
            if(strcmp(".",entry->d_name) == 0 ||strcmp("..",entry->d_name) == 0)
            continue;
        }
        /*将照片名字保存到数组*/
        memcpy(gcPicBuf[cnt],entry->d_name,strlen(entry->d_name));
        if(cnt++ > PICMAX)
            return 0;
        /*将照片添加到盒子中*/
        gtk_container_add (GTK_CONTAINER (flowbox), GetFaceFile(entry->d_name));
    }
    closedir(dp);
    return 0;
}
static int WindowOpenFlag;
/******************************************************************************
* 函数:              GetFaceLsit
*
* 说明:  列出所有可用的头像图片
*
* 输入:
*        左侧布局盒
*
* 返回:
*
* 作者:  zhuyaliang  09/05/2018
******************************************************************************/
static void GetFaceList(GtkWidget *button, gpointer data)
{
    UserAdmin *ua = (UserAdmin *) data;
    GtkWidget *window;
    GtkWidget *scrolled;
    GtkWidget *flowbox;
    int MouseId;
    int KeyId;
    int nRet;
  
    if(WindowOpenFlag == 1)
        Unbind(ua);        
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_decorated(GTK_WINDOW(window),FALSE);  //设置无边框
    gtk_window_set_default_size (GTK_WINDOW (window), 450, 270);
    gtk_widget_realize(window);
    gtk_window_set_position(GTK_WINDOW(window),GTK_WIN_POS_MOUSE);
    scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_set_border_width(GTK_CONTAINER(window),20);

    flowbox =  gtk_flow_box_new();
    gtk_widget_set_valign (flowbox, GTK_ALIGN_START);
    gtk_flow_box_set_max_children_per_line (GTK_FLOW_BOX (flowbox), 30);
    gtk_flow_box_set_selection_mode (GTK_FLOW_BOX (flowbox), GTK_SELECTION_NONE);
    gtk_flow_box_set_activate_on_single_click(GTK_FLOW_BOX (flowbox),TRUE);
    gtk_container_add (GTK_CONTAINER (scrolled), flowbox);
    gtk_container_add (GTK_CONTAINER (window), scrolled);
    /*得到目录下的所以照片*/
    nRet = GetDirFace(flowbox);
    if(nRet < 0)
    {
        MessageReport(_("Avatar list"),_("There is no address to store photos"),WARING);
    }   
    ua->IconWindow = window;
    gtk_flow_box_selected_foreach (GTK_FLOW_BOX (flowbox),
                                face_widget_activated,
                                ua);

    gtk_widget_add_events(ua->MainWindow,
                          GDK_BUTTON_PRESS_MASK |
                          GDK_KEY_PRESS_MASK);

    MouseId = g_signal_connect(G_OBJECT(ua->MainWindow),
                    "button-press-event", 
                    G_CALLBACK(NotifyEvents), 
                    ua);

    ua->MouseId = MouseId;
    KeyId = g_signal_connect(G_OBJECT(ua->MainWindow),
                    "key-press-event", 
                    G_CALLBACK(NotifyEvents), 
                    ua);
    ua->KeyId = KeyId;
    WindowOpenFlag = 1;
    g_signal_connect (flowbox, "child-activated",
                      G_CALLBACK (face_widget_activated),
                      ua);
    gtk_widget_show_all (scrolled);

    gtk_widget_show (window);

}
static gboolean RealNameValidCheck (const gchar *name)
{
        gboolean is_empty = TRUE;
        const gchar *c;
        for (c = name; *c; c++) 
        {
                gunichar unichar;
                
                unichar = g_utf8_get_char_validated (c, -1);
                if (unichar == (gunichar) -1 || unichar == (gunichar) -2)
                        break; 
                if (!g_unichar_isspace (unichar)) 
                {
                        is_empty = FALSE;
                        break;
                }       
        }

        return !is_empty;
}

/******************************************************************************
* 函数:              ModifyName        
*        
* 说明:  修改用户名，回车进入次函数
*        
* 输入:  		
*       
*        
* 返回:  
*        
* 作者:  zhuyaliang  09/05/2018
******************************************************************************/ 
static void ModifyName (GtkEntry *entry,gpointer  data)
{
    UserAdmin *ua = (UserAdmin *) data;
    const gchar *NewName;

    NewName = gtk_entry_get_text (entry);
    if(RealNameValidCheck(NewName))
    {        
        gtk_list_store_set(ua->ListSTore,&ua->ul[gnCurrentUserIndex].Iter,
                          LIST_TEXT, NewName,
                          LIST_COLOR,"black",
                          LIST_FRONT,1985,-1);
        memset(ua->ul[gnCurrentUserIndex].RealName,
              '\0',
            strlen(ua->ul[gnCurrentUserIndex].RealName));
        memcpy(ua->ul[gnCurrentUserIndex].RealName,NewName,strlen(NewName));
        act_user_set_real_name (ua->ul[gnCurrentUserIndex].User,NewName);
    }
    else
    {
        MessageReport(_("Change Name"),_("Please enter a valid character"),ERROR); 
    }        
}
/******************************************************************************
* Function:              DisplayUserSetFace  
*        
* Explain: Display the user's head image and name
*        
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  09/05/2018
******************************************************************************/
void DisplayUserSetFace(GtkWidget *Hbox,UserAdmin *ua)
{
    GtkWidget *image;
    GdkPixbuf *pb, *pb2;
    GtkWidget *EntryName;
    GtkWidget *table;
    GtkWidget *fixed;
    GtkWidget *ButtonFace;
     
    fixed = gtk_fixed_new();
    gtk_box_pack_start(GTK_BOX(Hbox),fixed,TRUE,TRUE,0);  
   
    table = gtk_grid_new();
    gtk_fixed_put(GTK_FIXED(fixed),table, 0, 0);
   
    /*set user icon 96 *96 */
    pb = gdk_pixbuf_new_from_file(ua->ul[0].UserIcon,NULL);
    pb2 = gdk_pixbuf_scale_simple (pb,96,96, GDK_INTERP_BILINEAR);
    image = gtk_image_new_from_pixbuf(pb2);

    ButtonFace = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(ButtonFace),image);
    ua->ButtonFace = ButtonFace;
    gtk_grid_attach(GTK_GRID(table) , ButtonFace , 0 , 0 , 8 , 8);
    g_signal_connect(G_OBJECT(ButtonFace), 
                    "clicked",
                     G_CALLBACK(GetFaceList),
                     ua);

    /*设置用户名*/
    EntryName = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(EntryName),48);
    gtk_entry_set_text(GTK_ENTRY(EntryName),ua->ul[0].UserName);
    ua->EntryName = EntryName;
    gtk_grid_attach(GTK_GRID(table) ,EntryName , 8 , 4 , 1 , 1);
    g_signal_connect (EntryName, "activate",G_CALLBACK (ModifyName),ua);
    gtk_grid_set_row_spacing(GTK_GRID(table), 10);
    gtk_grid_set_column_spacing(GTK_GRID(table), 10);

}     
