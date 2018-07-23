
#include "user-info.h"
#include "user.h"
#include "user-base.h"
#include "user-share.h"
#include "user-face.h"
#include "user-list.h"

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
    g_signal_connect(G_OBJECT(Window), "delete-event", G_CALLBACK(on_window_quit), NULL);

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
    /*显示用户列表*/   
    DisplayUserList(Hbox2,ua);

    Hbox1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);  
    gtk_box_pack_start(GTK_BOX(Hbox),Hbox1,TRUE,TRUE,10); 
   /* 显示用户头像与用户名 */
    DisplayUserSetFace(Hbox1,ua);
    
    /* 显示用户其他界面 */    
    DisplayUserSetOther(Hbox1,ua);  

    AddRemoveUser(Vbox,ua);

}
int main(int argc, char **argv)
{
    GtkWidget *fixed;
    GtkWidget *Vbox;
    UserAdmin ua;

    bindtextdomain (PACKAGE, LOCALEDIR);   
    textdomain (PACKAGE); 
    
    gtk_init(&argc, &argv);
    
    all_languages = mate_get_all_locales ();
    InitMainWindow(&ua);

    WindowLogin = ua.MainWindow;
    ua.UserCount = GetUserInfo(&ua);

    fixed = gtk_fixed_new();
    gtk_container_add(GTK_CONTAINER(ua.MainWindow), fixed); 
    
    Vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);  
    gtk_fixed_put(GTK_FIXED(fixed),Vbox, 0, 0);
    
    CreateInterface(Vbox,&ua);

    gtk_widget_show_all(ua.MainWindow);
    gtk_main();

}
