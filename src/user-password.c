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

#define _XOPEN_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <shadow.h>
#include <pwquality.h>
#include "user-password.h"
#include "user-share.h"
#include "user-info.h"

/******************************************************************************
* Function:              NextSetPass 
*        
* Explain: Set the password at the next login
*        
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  15/05/2018
******************************************************************************/
static void NextSetPass (GtkRadioButton *button,gpointer data)
{
    UserAdmin *ua = (UserAdmin *)data;
    UserInfo *user;

    gtk_widget_set_sensitive(ua->NewPassEntry, FALSE);  //lock widget
    gtk_widget_set_sensitive(ua->CheckPassEntry, FALSE);
    gtk_widget_set_sensitive(ua->LevelBar, FALSE);
    gtk_widget_set_sensitive(ua->ButtonConfirm,TRUE);

    if(ua->CheckPassTimeId > 0)                //因为不需要检查密码所以移除定时器
    {
        g_source_remove(ua->CheckPassTimeId);
        ua->CheckPassTimeId = 0;
    }
    user = GetIndexUser(ua->UsersList,gnCurrentUserIndex);
    act_user_set_password_mode(user->ActUser,ACT_USER_PASSWORD_MODE_SET_AT_LOGIN);
    
}        

/******************************************************************************
* Function:              NowSetPass 
*        
* Explain: Now set the password,Create a timer to check the password
*        
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  15/05/2018
******************************************************************************/
static void NowSetPass (GtkRadioButton *button,gpointer data)
{
    int CheckPassTimeId;
    UserAdmin *ua = (UserAdmin *)data;
    UserInfo *user;
    gtk_widget_set_sensitive(ua->CheckPassEntry, FALSE);  //Unlocking Widget
    gtk_widget_set_sensitive(ua->NewPassEntry, TRUE);
    gtk_widget_set_sensitive(ua->LevelBar, TRUE);
    gtk_widget_set_sensitive(ua->ButtonConfirm, FALSE);

    CheckPassTimeId = g_timeout_add(800,(GSourceFunc)CheckPassword,ua);
    ua->CheckPassTimeId = CheckPassTimeId;
    user = GetIndexUser(ua->UsersList,gnCurrentUserIndex);
    act_user_set_password_mode(user->ActUser,ACT_USER_PASSWORD_MODE_REGULAR); 
}        
/******************************************************************************
* Function:              SetNewPass 
*        
* Explain: Confirm the change of the password
*        
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  15/05/2018
******************************************************************************/
static void SetNewPass(UserAdmin *ua)
{
    int         passtype;
    UserInfo   *user;
    const char *password;

    password =  gtk_entry_get_text(GTK_ENTRY(ua->CheckPassEntry));
    user = GetIndexUser(ua->UsersList,gnCurrentUserIndex);
    GetPasswordModeText(user->ActUser,&passtype);
    /*choose now set password*/
    if(passtype == OLDPASS)
    {        
        act_user_set_password (user->ActUser,password, "");
    }
    UpdateInterface(user->ActUser,ua);
}
static void SetButtonMode(UserAdmin *ua)
{
    int passtype;
    UserInfo *user;
    
    user = GetIndexUser(ua->UsersList,gnCurrentUserIndex);
    GetPasswordModeText(user->ActUser,&passtype);
    if(passtype == OLDPASS)
    {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ua->RadioButton2), TRUE);
        NowSetPass(GTK_RADIO_BUTTON(ua->RadioButton2),ua);
    }
    else
    {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ua->RadioButton1), TRUE);
        NextSetPass(GTK_RADIO_BUTTON(ua->RadioButton1),ua);
    }    
}   
static GtkWidget*
dialog_add_button_with_icon_name (GtkDialog   *dialog,
                                  const gchar *button_text,
                                  const gchar *icon_name,
                                  gint         response_id)
{
	GtkWidget *button;

	button = gtk_button_new_with_mnemonic (button_text);
	gtk_button_set_image (GTK_BUTTON (button), gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_BUTTON));

	gtk_button_set_use_underline (GTK_BUTTON (button), TRUE);
	gtk_style_context_add_class (gtk_widget_get_style_context (button), "text-button");
	gtk_widget_set_can_default (button, TRUE);
	gtk_widget_show (button);
	gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button, response_id);

	return button;
}
static void CloseNewPassWindow(UserAdmin *ua)
{
    gtk_widget_destroy(GTK_WIDGET(ua->PasswordDialog));
    if(ua->CheckPassTimeId > 0)
    {
        g_source_remove(ua->CheckPassTimeId);
        ua->CheckPassTimeId = 0;
    }
}        
static void
passwod_dialog_response (GtkDialog *dialog,
			             int        response_id,
			             UserAdmin *ua)
{

	switch (response_id) 
    {
	case GTK_RESPONSE_CLOSE:
        CloseNewPassWindow(ua);
		break;
	case GTK_RESPONSE_OK:
        SetNewPass(ua);
        CloseNewPassWindow(ua);
		break;
	default:
		break;
	}
}
/******************************************************************************
* Function:              CreateNewPass 
*        
* Explain: Set the password for the first time
*        
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  15/05/2018
******************************************************************************/
void CreateNewPass(UserAdmin *ua)
{
    GtkWidget *PasswordDialog;
    GtkWidget *Vbox;
    GtkWidget *Table;
    GtkWidget *LabelTitle;
    GSList    *RadioGroup;
    GtkWidget *RadioButton1;
    GtkWidget *RadioButton2;
    GtkWidget *LabelPass;
    GtkWidget *NewPassEntry;
    GtkWidget *LevelBar;
    GtkWidget *LabelPassNote;
    GtkWidget *LabelConfirm;
    GtkWidget *CheckPassEntry;


    GtkWidget *Hseparator;
    GtkWidget *LabelSpace;

    //create chnaged passwod dialog

    PasswordDialog = gtk_dialog_new_with_buttons (_("Set Password"),
                                                   GTK_WINDOW (WindowLogin),
                                                   GTK_DIALOG_MODAL| GTK_DIALOG_DESTROY_WITH_PARENT,
                                                   NULL,
                                                   NULL);
    gtk_container_set_border_width(GTK_CONTAINER(PasswordDialog),20);
    gtk_window_set_deletable(GTK_WINDOW (PasswordDialog), FALSE);
    gtk_window_set_default_size (GTK_WINDOW (PasswordDialog), 450, 200);
    dialog_add_button_with_icon_name ( GTK_DIALOG (PasswordDialog), 
                                      _("Close"), 
                                      "window-close", 
                                       GTK_RESPONSE_CLOSE);

    ua->ButtonConfirm = dialog_add_button_with_icon_name (GTK_DIALOG (PasswordDialog), 
                                                         _("Confirm"), 
                                                         "emblem-default", 
                                                          GTK_RESPONSE_OK);
    Vbox =  gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
    gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (PasswordDialog))),
                        Vbox,
                        TRUE,
                        TRUE,
                        8);
    ua->PasswordDialog = PasswordDialog;
    Table = gtk_grid_new();
    gtk_box_pack_start(GTK_BOX(Vbox), Table,TRUE, TRUE, 0);
    gtk_grid_set_column_homogeneous(GTK_GRID(Table),TRUE);
    LabelTitle = gtk_label_new(_("Password"));
    gtk_grid_attach(GTK_GRID(Table) , LabelTitle , 0 , 0 , 1 , 1);

    //Select button
    ua->CheckPassTimeId = 0;
    RadioButton1 = gtk_radio_button_new_with_label(NULL,_("Set up next time"));
    RadioGroup = gtk_radio_button_get_group(GTK_RADIO_BUTTON(RadioButton1));
    gtk_grid_attach(GTK_GRID(Table) , RadioButton1 , 0 , 1 , 5 , 1);
    g_signal_connect(RadioButton1,
                    "released",
                     G_CALLBACK(NextSetPass),
                     ua);
    ua->RadioButton1 = RadioButton1;
   
    RadioButton2 = gtk_radio_button_new_with_label(RadioGroup,_("Now set up"));
    gtk_grid_attach(GTK_GRID(Table) , RadioButton2 , 0 , 2 , 5 , 1);
    g_signal_connect(RadioButton2,
                    "released",
                     G_CALLBACK(NowSetPass),
                     ua);
    ua->RadioButton2 = RadioButton2;
    
    LabelPass = gtk_label_new(NULL);
    SetLableFontType(LabelPass,"gray",11,_("User Password"));
    gtk_grid_attach(GTK_GRID(Table) ,LabelPass , 0 , 3 , 1 , 1);

    NewPassEntry = gtk_entry_new();
    ua->NewPassEntry = NewPassEntry;
    gtk_entry_set_icon_from_icon_name(GTK_ENTRY(NewPassEntry), 
                                      GTK_ENTRY_ICON_SECONDARY,
                                      "system-run");
    gtk_entry_set_icon_tooltip_text (GTK_ENTRY(NewPassEntry),
                                     GTK_ENTRY_ICON_SECONDARY,
                                    _("generation password"));
    gtk_entry_set_max_length(GTK_ENTRY(NewPassEntry),20);
    gtk_grid_attach(GTK_GRID(Table) , NewPassEntry , 1 , 3 , 4 , 1);


    LevelBar = gtk_level_bar_new ();
    ua->LevelBar = LevelBar;
    gtk_level_bar_set_min_value(GTK_LEVEL_BAR(LevelBar),0.0);
    gtk_level_bar_set_max_value(GTK_LEVEL_BAR(LevelBar),5.0);
    gtk_level_bar_set_mode(GTK_LEVEL_BAR(LevelBar),GTK_LEVEL_BAR_MODE_DISCRETE);
    gtk_grid_attach(GTK_GRID(Table) ,LevelBar , 1 , 4 , 4 , 1);


    LabelPassNote = gtk_label_new (NULL);
    ua->LabelPassNote = LabelPassNote;
    gtk_grid_attach(GTK_GRID(Table) ,LabelPassNote , 0 , 5 , 4 , 1);

    LabelConfirm = gtk_label_new (NULL);
    SetLableFontType(LabelConfirm,"gray",11,_("Confirm"));
    gtk_grid_attach(GTK_GRID(Table) ,LabelConfirm , 0 , 6 , 1 , 1);

    CheckPassEntry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(NewPassEntry),FALSE);
    ua->CheckPassEntry = CheckPassEntry;
    gtk_entry_set_max_length(GTK_ENTRY(CheckPassEntry),20);
    gtk_entry_set_visibility(GTK_ENTRY(CheckPassEntry),FALSE);
    gtk_grid_attach(GTK_GRID(Table) , CheckPassEntry , 1 , 6 , 4 , 1);
    g_signal_connect (G_OBJECT(NewPassEntry), 
                     "icon-press", 
                      G_CALLBACK(AutoGenera), 
                      CheckPassEntry);

    LabelSpace = gtk_label_new(NULL);
    ua->LabelSpace = LabelSpace;
    gtk_grid_attach(GTK_GRID(Table) , LabelSpace , 0 , 7 , 4 , 1);

    Hseparator = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_grid_attach(GTK_GRID(Table) , Hseparator , 0 , 8 , 5 , 1);

    SetButtonMode(ua);

    gtk_grid_set_row_spacing(GTK_GRID(Table), 10);
    gtk_grid_set_column_spacing(GTK_GRID(Table), 10);
    g_signal_connect (G_OBJECT (PasswordDialog),
			         "response",
			          G_CALLBACK (passwod_dialog_response),
			          ua);
    gtk_widget_show_all(PasswordDialog);
}
