#include "user-info.h"
#include "user.h"
#include "user-base.h"
#include "user-share.h"
#include "user-face.h"
#include "user-list.h"

#define  LOCKFILE    "/tmp/user-admin.pid"

static gboolean on_window_quit (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	g_strfreev(all_languages);
	gtk_main_quit();
	return TRUE;
}
static void InitMainWindow(UserAdmin *ua)
{
    GtkWidget *Window;

    Window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_position(GTK_WINDOW(Window), GTK_WIN_POS_CENTER);
    gtk_window_set_title(GTK_WINDOW(Window), _("Mate User Manage"));
    gtk_container_set_border_width(GTK_CONTAINER(Window),10);
    g_signal_connect(G_OBJECT(Window), 
                    "delete-event",
                    G_CALLBACK(on_window_quit),
                    NULL);

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
    char WriteBuf[30] = { 0 };
    fd = open(LOCKFILE,O_WRONLY|O_CREAT,0777);
    if(fd < 0)
    {
         MessageReport(_("open file"),_("Create pid file failed"),ERROR);
         return -1;      
    }       
    chmod(LOCKFILE,0777); 
    pid = getpid();
    sprintf(WriteBuf,"%d",pid);
    write(fd,WriteBuf,strlen(WriteBuf));
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

    if(access(LOCKFILE,F_OK)==0)
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
             close(fd);
             return TRUE;
        }        
        pid = atoi(ReadBuf);
        if(kill(pid,0) == 0)
        {        
            Run = TRUE;
        }
        else
        {    
            if(remove(LOCKFILE) < 0)
            {
                MessageReport(_("remove file"),
                _("Please delete the /tmp/user-admin.pid file manually"),ERROR);
                return TRUE;
            }        
            if(RecordPid() < 0)
                Run = TRUE;
        }    
                
    }
    else
    {
        if(RecordPid() < 0)
            Run = TRUE;
    }        

    close(fd);
    return Run;

}        
static int LangSort (const void *a,const void *b)
{
    char *c = (char *)a;
    char *d = (char *)b;
    int i = 0;

    while(c[i] == d[i] && c[i] && d[i])
    {
        i++;
    } 
    return c[i] - d[i];    
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

    /* Get local support language */ 
    all_languages = mate_get_all_locales ();

    qsort(all_languages,
          g_strv_length (all_languages),
          sizeof(all_languages[0]),
          LangSort);

    WindowLogin = ua.MainWindow;

    /* Get local user info */
    ua.UserCount = GetUserInfo(&ua);

    fixed = gtk_fixed_new();
    gtk_container_add(GTK_CONTAINER(ua.MainWindow), fixed); 
    
    Vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);  
    gtk_fixed_put(GTK_FIXED(fixed),Vbox, 0, 0);
   
    /* Create an interface */ 
    CreateInterface(Vbox,&ua);

    gtk_widget_show_all(ua.MainWindow);
    gtk_main();

}
