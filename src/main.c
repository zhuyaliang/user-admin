/*   mate-user-admin 
*   Copyright (C) 2018  zhuyaliang https://github.com/zhuyaliang/
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.

*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.

*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "user-info.h"
#include "user.h"
#include "user-base.h"
#include "user-share.h"
#include "user-face.h"
#include "user-list.h"

#define  LOCKFILE    "/tmp/user-admin.pid"

static gboolean on_window_quit (GtkWidget *widget, 
                                GdkEvent  *event, 
                                gpointer   user_data)
{
    UserAdmin *ua = (UserAdmin *)user_data;
    
    g_slist_free_full (ua->UsersList,g_object_unref);
    g_strfreev(all_languages);
    g_hash_table_destroy(LocaleHash);
    g_slist_free (LangList);
    gtk_main_quit();
    return TRUE;
}
static GdkPixbuf * GetAppIcon(void)
{
    GdkPixbuf *Pixbuf;
    GError    *Error = NULL;

    Pixbuf = gdk_pixbuf_new_from_file("/usr/share/mate-user-admin/icon/user-admin.png",&Error);
    if(!Pixbuf)
    {
        MessageReport(_("Get Icon Fail"),Error->message,ERROR);
        g_error_free(Error);
    }   
    
    return Pixbuf;
}    
static void InitMainWindow(UserAdmin *ua)
{
    GtkWidget *Window;
    GdkPixbuf *AppIcon;

    Window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_position(GTK_WINDOW(Window), GTK_WIN_POS_CENTER);
    gtk_window_set_title(GTK_WINDOW(Window), _("Mate User Manage"));
    gtk_container_set_border_width(GTK_CONTAINER(Window),10);
    g_signal_connect(G_OBJECT(Window), 
                    "delete-event",
                     G_CALLBACK(on_window_quit),
                     ua);
    
    AppIcon = GetAppIcon();
    if(AppIcon)
    {
        gtk_window_set_icon(GTK_WINDOW(Window),AppIcon);
        g_object_unref(AppIcon);
    }    
    ua->MainWindow = Window;
}

static void CreateInterface(GtkWidget *Vbox,UserAdmin *ua)
{
    GtkWidget *Hbox;
    GtkWidget *Hbox1;
    GtkWidget *Hbox2;

    Hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);  
    gtk_box_pack_start(GTK_BOX(Vbox),Hbox,FALSE,FALSE,0); 
    
    Hbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);  
    gtk_box_pack_start(GTK_BOX(Hbox),Hbox2,TRUE,TRUE,10); 
    gtk_widget_set_size_request (Hbox2, 200,-1);

    /*Display user list on the left side*/   
    DisplayUserList(Hbox2,ua);

    Hbox1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);  
    gtk_box_pack_start(GTK_BOX(Hbox),Hbox1,TRUE,TRUE,10); 
   
    /* Display the user's head image and name */
    DisplayUserSetFace(Hbox1,ua);
    
    /*user type and user password and user langusge and auto login and login time */    
    DisplayUserSetOther(Hbox1,ua);  

    /*Adding new users or remove users*/
    AddRemoveUser(Vbox,ua);

}
static int RecordPid(void)
{
    int pid = 0;
    int fd;
    int Length = 0; 
    char WriteBuf[30] = { 0 };
    fd = open(LOCKFILE,O_WRONLY|O_CREAT|O_TRUNC,0777);
    if(fd < 0)
    {
         MessageReport(_("open file"),_("Create pid file failed"),ERROR);
         return -1;      
    }       
    chmod(LOCKFILE,0777); 
    pid = getpid();
    sprintf(WriteBuf,"%d",pid);
    Length = write(fd,WriteBuf,strlen(WriteBuf));
    if(Length <= 0 )
    {
        MessageReport(_("write file"),_("write pid file failed"),ERROR);
        return -1;      
    }			
    close(fd);

    return 0;
}        
/******************************************************************************
* Function:              ProcessRuning      
*        
* Explain: Check whether the process has been started,If the process is not started, 
*          record the current process ID =====>"/tmp/user-admin.pid"
*        
* Input:         
*        
*        
* Output:  start        :TRUE
*          not start    :FALSE
*        
* Author:  zhuyaliang  31/07/2018
******************************************************************************/
static gboolean ProcessRuning(void)
{
    int fd = 0;
    int pid = 0;
    gboolean Run = FALSE;
    char ReadBuf[30] = { 0 };

    if(access(LOCKFILE,F_OK) == 0)
    {
        fd = open(LOCKFILE,O_RDONLY);
        if(fd < 0)
        {
             MessageReport(_("open file"),_("open pid file failed"),ERROR);
             return TRUE;
        }        
        if(read(fd,ReadBuf,sizeof(ReadBuf)) <= 0)
        {
             MessageReport(_("read file"),_("read pid file failed"),ERROR);
             goto ERROREXIT;
        }        
        pid = atoi(ReadBuf);
        if(kill(pid,0) == 0)
        {        
             goto ERROREXIT;
        }
    }
    
    if(RecordPid() < 0)
        Run = TRUE;
    
    return Run;
ERROREXIT:
    close(fd);
    return TRUE;

}        

static int LangSort (const void *a,const void *b)
{
    gchar *name1, *name2;
    gint result;

    name1 = g_utf8_collate_key (a, -1);
    name2 = g_utf8_collate_key (b, -1);
    result = strcmp (name1, name2);
    g_free (name1);
    g_free (name2);

    return result;
}

static void GetLocaleLang (void)
{
    guint i,len;
    char *lang;

    all_languages = mate_get_all_locales ();
    LocaleHash = g_hash_table_new(g_str_hash, g_str_equal);
    len = g_strv_length (all_languages); 
    for(i = 0; i < len; i++)
    {        
        lang = mate_get_language_from_locale (all_languages[i], NULL);    
        g_hash_table_insert(LocaleHash,lang,all_languages[i]);
        LangList = g_slist_insert_sorted(LangList, lang,LangSort);
    }
}        
int main(int argc, char **argv)
{
    GtkWidget *fixed;
    GtkWidget *Vbox;
    UserAdmin ua;

    bindtextdomain (PACKAGE, LOCALEDIR);   
    textdomain (PACKAGE); 
    
    gtk_init(&argc, &argv);
    
    /* Create the main window */
    InitMainWindow(&ua);

    /* Check whether the process has been started */
    if(ProcessRuning() == TRUE)
        exit(0);        

    WindowLogin = ua.MainWindow;
    /* Get local support language */ 
    GetLocaleLang();
    /* Get local user info */
    ua.UserCount = GetUserInfo(&ua);
    if(ua.UserCount < 0)
    {
        exit(0);
    }			
    fixed = gtk_fixed_new();
    gtk_container_add(GTK_CONTAINER(ua.MainWindow), fixed); 
    
    Vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);  
    gtk_fixed_put(GTK_FIXED(fixed),Vbox, 0, 0);
   
    /* Create an interface */ 
    CreateInterface(Vbox,&ua);

    gtk_widget_show_all(ua.MainWindow);
    gtk_main();

}
