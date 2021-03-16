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

#include "user-base.h"
#include "user-password.h"
#include "user-share.h"
#include "user-info.h"
#include "user-group-window.h"
#include "user-history.h"
#include "user-language.h"


static void user_language_set_done (UserLanguage *chooser, UserAdmin *ua)
{
    gchar *name = NULL;
    gchar *lang;

    lang = user_language_get_language (chooser);
    name = mate_get_language_from_locale (lang, NULL);
    gtk_button_set_label(GTK_BUTTON(ua->ButtonLanguage),
                         name);

    g_free (name);
}
static void
change_language (GtkButton   *button,
                 UserAdmin   *self)
{
    const gchar *current_language;
    UserLanguage *user_language;

    current_language = GetUserLang(self->CurrentUser);

    user_language = user_language_new (self->CurrentUser);
    g_signal_connect (G_OBJECT(user_language),
                    "lang-changed",
                     G_CALLBACK(user_language_set_done),
                     self);
    
    if (current_language && *current_language != '\0')
        user_language_set_language (user_language, current_language);
    gtk_widget_show_all(GTK_WIDGET(user_language));
}

/******************************************************************************
* Function:            SwitchState 
*        
* Explain: Select auto login,Only one user can choose to log in automatically.
*        
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  09/05/2018
******************************************************************************/
static void SwitchState(GtkSwitch *widget,gboolean state,gpointer data)
{
    GSList    *list;
    GSList    *l;
    UserAdmin *ua = (UserAdmin *)data;
    ActUserManager *um;
    
    if(Change == 0)
    {        
        um =  act_user_manager_get_default ();
        if(state == TRUE)
        {
            list = act_user_manager_list_users (um);
            for (l = list; l != NULL; l = l->next)
            {
                ActUser *u = l->data;
                if (act_user_get_uid (u) != act_user_get_uid (ua->CurrentUser)) 
                {
                    act_user_set_automatic_login (u, FALSE);
                }
            }
            g_slist_free (list);
            act_user_set_automatic_login(ua->CurrentUser,TRUE);
        }
        else
            act_user_set_automatic_login(ua->CurrentUser,FALSE);
    }
}    

static void user_password_set_done (UserPassword *dialog, UserAdmin *ua)
{
    char *label;
    
    label = user_password_get_label (dialog);
    gtk_button_set_label (GTK_BUTTON (ua->ButtonPass), label);
}
/******************************************************************************
* Function:             ChangePass 
*        
* Explain: Modifying the cipher signal.The two state .Change the password 
*          .Set set the new password.
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  15/05/2018
******************************************************************************/
static void ChangePass(GtkWidget *widget,gpointer data)
{
    UserAdmin *ua = (UserAdmin *)data;
    UserPassword *dialog;

    dialog = user_password_new (ua->CurrentUser);
    g_signal_connect (G_OBJECT(dialog),
                    "changed",
                     G_CALLBACK(user_password_set_done),
                     ua);
    gtk_widget_show_all (GTK_WIDGET (dialog));
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (GTK_WIDGET (dialog));
}
/******************************************************************************
* Function:             ComboSelectUserType 
*        
* Explain: Select user type signal
*        
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  09/05/2018
******************************************************************************/
static void ComboSelectUserType(GtkWidget *widget,gpointer data)
{
    UserAdmin *ua = (UserAdmin *)data;
    gint       account_type;

    if( Change ==0 )
    {
        account_type =  gtk_combo_box_get_active (GTK_COMBO_BOX(widget)) ? 
                                                  ACT_USER_ACCOUNT_TYPE_ADMINISTRATOR:
                                                  ACT_USER_ACCOUNT_TYPE_STANDARD;
        act_user_set_account_type(ua->CurrentUser,account_type);
    }    
}
static void set_user_group (GtkWidget *widget, gpointer data)
{
    UserAdmin  *ua = (UserAdmin *)data;
    const char *name;

    name = GetUserName (ua->CurrentUser);
    UserGroupsManage (name, ua->UsersList);
}
/******************************************************************************
* Function:              DisplayUserSetOther 
*        
* Explain: Displays user type and user language and user password, 
*          and automatic login and logon time.
*        
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  09/05/2018
******************************************************************************/
void DisplayUserSetOther(GtkWidget *Hbox,UserAdmin *ua)
{
    GtkWidget *table;
    GtkWidget *fixed;
    GtkWidget *ButtonPass;
    GtkWidget *LabelUserType;
    GtkWidget *LabelAutoLogin;
    GtkWidget *LabelLanguage;
    GtkWidget *SwitchLogin;
    GtkWidget *LabelPass;
    GtkWidget *LabelTime;
    GtkWidget *ButtonTime;
    GtkWidget *ComboUser;
    GtkWidget *LabelGroup;
    GtkWidget *ButtonGroup;
    char      *lang;
    const char *lang_id;
    g_autofree const char *time = NULL;
	int PasswordType;
    fixed = gtk_fixed_new();
    gtk_box_pack_start(GTK_BOX(Hbox),fixed ,TRUE, TRUE, 0);
    table = gtk_grid_new();
    gtk_grid_set_column_homogeneous(GTK_GRID(table),TRUE);
    gtk_fixed_put(GTK_FIXED(fixed), table, 0, 0);

    /*user type*/
    LabelUserType = gtk_label_new(NULL);
    SetLableFontType(LabelUserType,"gray",11,_("Account Type"),TRUE);
    gtk_grid_attach(GTK_GRID(table) , LabelUserType , 0 , 0 , 1 , 1);

    /*drop-down select boxes*/
    ComboUser = SetComboUserType(_("Standard"),_("Administrators"));
    ua->ComUserType = ComboUser; 
    gtk_combo_box_set_active(GTK_COMBO_BOX(ComboUser),GetUserType(ua->CurrentUser));
    gtk_grid_attach(GTK_GRID(table) , ComboUser , 1 , 0 , 2 , 1);
    g_signal_connect(G_OBJECT(ComboUser),
                    "changed",
                     G_CALLBACK(ComboSelectUserType),
                     ua);

   /*select language*/ 
    LabelLanguage = gtk_label_new(NULL);
    SetLableFontType(LabelLanguage,"gray",11,_("Language"),TRUE);
    gtk_grid_attach(GTK_GRID(table) , LabelLanguage , 0 , 1 , 1 , 1);
    lang_id = GetUserLang(ua->CurrentUser);
    if(lang_id == NULL)
    {
        ua->ButtonLanguage = gtk_button_new_with_label(_("No Settings"));
    }    
    else
    {
        lang = mate_get_language_from_locale (lang_id, NULL);
        ua->ButtonLanguage = gtk_button_new_with_label(lang);
		g_free (lang);
    }
    g_signal_connect (ua->ButtonLanguage, 
                     "clicked",
                      G_CALLBACK (change_language),
                      ua);
    gtk_grid_attach(GTK_GRID(table) , ua->ButtonLanguage , 1 , 1 , 2 , 1);
    /*set password*/
    LabelPass = gtk_label_new(NULL);
    SetLableFontType(LabelPass,"gray",11,_("Password"),TRUE);
    gtk_grid_attach(GTK_GRID(table) , LabelPass , 0 , 2 , 1 , 1);
    ButtonPass = gtk_button_new_with_label(GetPasswordModeText(ua->CurrentUser,
                                           &PasswordType));
    ua->ButtonPass = ButtonPass;
    g_signal_connect (ButtonPass, 
                     "clicked",
                      G_CALLBACK (ChangePass),
                      ua);
    gtk_grid_attach(GTK_GRID(table) , ButtonPass , 1 , 2 , 2 , 1);
   
    /*auto login*/
    LabelAutoLogin = gtk_label_new(NULL);
    SetLableFontType(LabelAutoLogin,"gray",11,_("Automatic logon"),TRUE);
    gtk_grid_attach(GTK_GRID(table) , LabelAutoLogin , 0 , 3 , 1 , 1);
    SwitchLogin = gtk_switch_new();
    ua->SwitchAutoLogin = SwitchLogin;
    gtk_switch_set_state (GTK_SWITCH(SwitchLogin),
                          GetUserType(ua->CurrentUser));
    gtk_grid_attach(GTK_GRID(table) , SwitchLogin , 1 , 3 , 1 , 1);
    g_signal_connect(G_OBJECT(SwitchLogin),
                    "state-set",
                     G_CALLBACK(SwitchState),
                     ua);
    
    /*login time*/
    LabelTime = gtk_label_new(NULL);
    SetLableFontType(LabelTime,"gray",11,_("Login time"),TRUE);
    gtk_grid_attach(GTK_GRID(table) , LabelTime, 0 , 4 , 1 , 1);
    
    time = GetLoginTimeText(ua->CurrentUser);
    ButtonTime = gtk_button_new_with_label (time);
    ua->ButtonUserTime = ButtonTime;
    gtk_grid_attach(GTK_GRID(table) , ButtonTime, 1 , 4 , 2 , 1);
    g_signal_connect (ButtonTime, 
                     "clicked",
                      G_CALLBACK (ViewLoginHistory),
                      ua);

    /*Group Manage*/
    LabelGroup = gtk_label_new(NULL);
    SetLableFontType(LabelGroup,"gray",11,_("Group Manage"),TRUE);
    gtk_grid_attach(GTK_GRID(table) , LabelGroup, 0 , 5 , 1 , 1);
  
    ButtonGroup = gtk_button_new_with_label (_("Setting Groups"));
    ua->ButtonUserGroup = ButtonGroup;
    gtk_grid_attach(GTK_GRID(table) , ButtonGroup, 1 , 5 , 2 , 1);
    g_signal_connect (ButtonGroup, 
                     "clicked",
                      G_CALLBACK (set_user_group),
                      ua);

    gtk_grid_set_row_spacing(GTK_GRID(table), 10);
    gtk_grid_set_column_spacing(GTK_GRID(table), 10);
}
