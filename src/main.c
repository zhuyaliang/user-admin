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

#include "user-window.h"
#include "user-share.h"

#define  LOCKFILE              "/tmp/mate-user-admin.pid"

static void ExitHook (void)
{
    remove(LOCKFILE);
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
*          record the current process ID =====>"/tmp/mate-user-admin.pid"
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
    int fd;
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

static void users_loaded(ActUserManager  *manager,
                         GParamSpec      *pspec,
                         gpointer         data)
{
    UserWindow *window;

    window = user_window_new (manager);

    g_signal_connect (manager,
                     "user-added",
                     G_CALLBACK (user_window_add_user_cb),
                     window);

    g_signal_connect (manager,
                     "user-removed",
                     G_CALLBACK (user_window_remove_user_cb),
                     window);

    gtk_widget_show_all (GTK_WIDGET (window));
}

static void SetupUsersList (void)
{
    gboolean        loaded;
    ActUserManager *manager;

    manager = act_user_manager_get_default ();

    g_object_get (manager, "is-loaded", &loaded, NULL);
    if (loaded)
        users_loaded (manager,NULL, NULL);
    else
        g_signal_connect(manager,
                        "notify::is-loaded",
                         G_CALLBACK (users_loaded),
                         NULL);

}
int main(int argc, char **argv)
{
   bindtextdomain (GETTEXT_PACKAGE,LUNAR_CALENDAR_LOCALEDIR);
    textdomain (GETTEXT_PACKAGE); 

    gtk_init(&argc, &argv);

    /* Program exit processing */
    atexit(ExitHook);
    /* Check whether the process has been started */
    if(ProcessRuning() == TRUE)
    {
        mate_uesr_admin_log("Info","The mate-user-admin process already exists");
        exit(0);
    }

    SetupUsersList ();
    gtk_main();
}
