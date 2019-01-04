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
#include "user-group.h"

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
static void SwitchState(GtkSwitch *widget,gboolean   state,gpointer  data)
{
    GSList *list;
    GSList *l;
    UserAdmin *ua = (UserAdmin *)data;
    ActUserManager *um;
    UserInfo *user;
    
    if(Change == 0)
    {        
        user = GetIndexUser(ua->UsersList,gnCurrentUserIndex);
        um =  act_user_manager_get_default ();
        if(state == TRUE)
        {
            list = act_user_manager_list_users (um);
            for (l = list; l != NULL; l = l->next)
            {
                ActUser *u = l->data;
                if (act_user_get_uid (u) != act_user_get_uid (user->ActUser)) 
                {
                    act_user_set_automatic_login (u, FALSE);
                }
            }
            g_slist_free (list);
            act_user_set_automatic_login(user->ActUser,TRUE);
        }
        else
            act_user_set_automatic_login(user->ActUser,FALSE);

    }
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
    gtk_widget_set_sensitive(ua->MainWindow,FALSE);
    CreateNewPass(ua);      //There is no password for the user

}
/******************************************************************************
* Function:              ComboSelectLanguage 
*        
* Explain: Switch language signal
*        
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  09/05/2018
******************************************************************************/
static void ComboSelectLanguage(GtkWidget *widget,gpointer data)
{
    UserAdmin *ua = (UserAdmin *)data;
    char *text;
    GtkTreeIter   iter; 
    GtkTreeModel *model;
    UserInfo *user;
    char *LangName;

    if(Change == 0) 
    {       
        if( gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter ) )
        {
            model = gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
            gtk_tree_model_get( model, &iter, 0, &text, -1 );
        }
        user = GetIndexUser(ua->UsersList,gnCurrentUserIndex);
        LangName = g_hash_table_lookup(LocaleHash,text);
        act_user_set_language(user->ActUser,LangName);
        g_free(text);
    }     
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
    UserInfo *user;
    gint account_type;

    if( Change ==0 )
    {
        account_type =  gtk_combo_box_get_active (GTK_COMBO_BOX(widget)) ? 
                                                  ACT_USER_ACCOUNT_TYPE_ADMINISTRATOR:
                                                  ACT_USER_ACCOUNT_TYPE_STANDARD;
        user = GetIndexUser(ua->UsersList,gnCurrentUserIndex);
        act_user_set_account_type(user->ActUser,account_type);
    }    
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
    GtkWidget *ComboLanguage;
    GtkWidget *LabelTime;
    GtkWidget *ButtonTime;
    GtkWidget *ComboUser;
    GtkWidget *LabelGroup;
    GtkWidget *ButtonGroup;
    UserInfo  *user;
    int index;

    user = GetIndexUser(ua->UsersList,0);
    fixed = gtk_fixed_new();
    gtk_box_pack_start(GTK_BOX(Hbox),fixed ,TRUE, TRUE, 0);
    table = gtk_grid_new();
    gtk_grid_set_column_homogeneous(GTK_GRID(table),TRUE);
    gtk_fixed_put(GTK_FIXED(fixed), table, 0, 0);

    /*user type*/
    LabelUserType = gtk_label_new(NULL);
    SetLableFontType(LabelUserType,"gray",11,_("Account Type"));
    gtk_grid_attach(GTK_GRID(table) , LabelUserType , 0 , 0 , 1 , 1);

    /*drop-down select boxes*/
    ComboUser = SetComboUserType(_("Standard"),_("Administrators"));
    ua->ComUserType = ComboUser; 
    gtk_combo_box_set_active(GTK_COMBO_BOX(ComboUser),GetUserType(user->ActUser));
    gtk_grid_attach(GTK_GRID(table) , ComboUser , 1 , 0 , 2 , 1);
    g_signal_connect(G_OBJECT(ComboUser),
                    "changed",
                     G_CALLBACK(ComboSelectUserType),
                     ua);

   /*select langusge*/ 
    LabelLanguage = gtk_label_new(NULL);
    SetLableFontType(LabelLanguage,"gray",11,_("Language"));
    gtk_grid_attach(GTK_GRID(table) , LabelLanguage , 0 , 1 , 1 , 1);

    ComboLanguage = SetComboLanguageType();
    ua->ComUserLanguage = ComboLanguage;
    index = GetCurrentLangIndex(GetUserLang(user->ActUser));
    if(index < 0)
        MessageReport(_("Get user language"),_("get user language failed"),ERROR);
    gtk_combo_box_set_active(GTK_COMBO_BOX(ComboLanguage),index);
    gtk_grid_attach(GTK_GRID(table) , ComboLanguage , 1 , 1 , 2 , 1);
    g_signal_connect(G_OBJECT(ComboLanguage),
                    "changed",
                     G_CALLBACK(ComboSelectLanguage),
                     ua);
    
    /*set password*/
    LabelPass = gtk_label_new(NULL);
    SetLableFontType(LabelPass,"gray",11,_("Password"));
    gtk_grid_attach(GTK_GRID(table) , LabelPass , 0 , 2 , 1 , 1);
    ButtonPass = gtk_button_new_with_label(GetPasswordModeText(user->ActUser,
                                           &user->PasswordType));
    ua->ButtonPass = ButtonPass;
    g_signal_connect (ButtonPass, 
                     "clicked",
                      G_CALLBACK (ChangePass),
                      ua);
    gtk_grid_attach(GTK_GRID(table) , ButtonPass , 1 , 2 , 2 , 1);
   
    /*auto login*/
    LabelAutoLogin = gtk_label_new(NULL);
    SetLableFontType(LabelAutoLogin,"gray",11,_("Automatic logon"));
    gtk_grid_attach(GTK_GRID(table) , LabelAutoLogin , 0 , 3 , 1 , 1);
    
    SwitchLogin = gtk_switch_new();
    ua->SwitchAutoLogin = SwitchLogin;
    gtk_switch_set_state (GTK_SWITCH(SwitchLogin),
                          GetUserType(user->ActUser));
    gtk_grid_attach(GTK_GRID(table) , SwitchLogin , 1 , 3 , 1 , 1);
    g_signal_connect(G_OBJECT(SwitchLogin),
                    "state-set",
                     G_CALLBACK(SwitchState),
                     ua);
    
    /*login time*/
    LabelTime = gtk_label_new(NULL);
    SetLableFontType(LabelTime,"gray",11,_("Login time"));
    gtk_grid_attach(GTK_GRID(table) , LabelTime, 0 , 4 , 1 , 1);
  
    ButtonTime = gtk_button_new_with_label (GetLoginTimeText(user->ActUser));
    ua->ButtonUserTime = ButtonTime;
    gtk_grid_attach(GTK_GRID(table) , ButtonTime, 1 , 4 , 2 , 1);
    /*Group Manage*/
    LabelGroup = gtk_label_new(NULL);
    SetLableFontType(LabelGroup,"gray",11,_("Group Manage"));
    gtk_grid_attach(GTK_GRID(table) , LabelGroup, 0 , 5 , 1 , 1);
  
    ButtonGroup = gtk_button_new_with_label (_("Setting Groups"));
    ua->ButtonUserGroup = ButtonGroup;
    gtk_grid_attach(GTK_GRID(table) , ButtonGroup, 1 , 5 , 2 , 1);
    g_signal_connect (ButtonGroup, 
                     "clicked",
                      G_CALLBACK (UserGroupsManage),
                      ua);

    gtk_grid_set_row_spacing(GTK_GRID(table), 10);
    gtk_grid_set_column_spacing(GTK_GRID(table), 10);
}
