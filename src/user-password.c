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

#define PASSWORD_CHECK_TIMEOUT 600

enum
{
    CHANGED,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };
struct _UserPassword
{
    GtkDialog     parent_instance;
    ActUser      *user;

    GtkWidget    *now_button;
    GtkWidget    *login_button;
    GtkWidget    *button_ok;
    GtkWidget    *password_entry;
    GtkWidget    *verify_entry;
    GtkWidget    *level_bar;
    GtkWidget    *label_note;

    char         *label;
    gint          check_password_time_id;
    gboolean      sensitive;

    ActUserPasswordMode password_mode;
};

G_DEFINE_TYPE (UserPassword, user_password, GTK_TYPE_DIALOG)

static void next_login_set_password (GtkRadioButton *button, gpointer data)
{
    UserPassword *dialog = USER_PASSWORD (data);

    gtk_widget_set_sensitive (dialog->password_entry, FALSE);  //lock widget
    gtk_widget_set_sensitive (dialog->verify_entry, FALSE);
    gtk_widget_set_sensitive (dialog->level_bar, FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET (dialog->button_ok), TRUE);
    gtk_entry_set_text (GTK_ENTRY (dialog->password_entry), "");
    gtk_entry_set_text (GTK_ENTRY (dialog->verify_entry), "");
    gtk_label_set_markup (GTK_LABEL (dialog->label_note), NULL);
    gtk_entry_set_icon_from_icon_name (GTK_ENTRY (dialog->password_entry),
                                       GTK_ENTRY_ICON_SECONDARY,
                                      "system-run");
    gtk_level_bar_set_value (GTK_LEVEL_BAR (dialog->level_bar), 0);

    if (dialog->check_password_time_id > 0)                //因为不需要检查密码所以移除定时器
    {
        g_source_remove (dialog->check_password_time_id);
        dialog->check_password_time_id = 0;
    }

    dialog->password_mode = ACT_USER_PASSWORD_MODE_SET_AT_LOGIN;
}

static void check_password_strength (UserPassword *dialog)
{
    const char *s;
    int         Level;
    const char *Message;

    s = gtk_entry_get_text (GTK_ENTRY (dialog->password_entry));
    if (strlen(s) == 0)
    {
        dialog->sensitive = FALSE;
        return;
    }
    Level = GetPassStrength (s, NULL, NULL, &Message);
    gtk_level_bar_set_value (GTK_LEVEL_BAR (dialog->level_bar), Level);
     
    /*The new password meets the requirements*/
    if (Message == NULL && Level > 1)
    {
        gtk_entry_set_icon_from_icon_name (GTK_ENTRY (dialog->password_entry),
                                           GTK_ENTRY_ICON_SECONDARY,
                                          "emblem-ok-symbolic");
        gtk_label_set_markup (GTK_LABEL (dialog->label_note), NULL);
        dialog->sensitive = TRUE;

        return;
    }
    dialog->sensitive = FALSE;
    SetLableFontType (dialog->label_note, "red", 10, Message, FALSE);
    gtk_entry_set_icon_from_icon_name (GTK_ENTRY (dialog->password_entry),
                                       GTK_ENTRY_ICON_SECONDARY,
                                      "system-run");
}

static void
update_sensitivity (UserPassword *dialog)
{
    if (!dialog->sensitive)
        gtk_entry_set_text (GTK_ENTRY (dialog->verify_entry), "");
    gtk_widget_set_sensitive (dialog->verify_entry, dialog->sensitive);
}

static void
update_password_match (UserPassword *dialog)
{
        const gchar *password;
        const gchar *verify;
        const gchar *message = _("The passwords do not match.");

        password = gtk_entry_get_text (GTK_ENTRY (dialog->password_entry));
        verify = gtk_entry_get_text (GTK_ENTRY (dialog->verify_entry));

        if (strlen (verify) > 0)
        {
            if (strcmp (password, verify) != 0)
            {
                gtk_widget_set_sensitive (dialog->button_ok, FALSE);
                SetLableFontType (dialog->label_note, "red", 10, message, FALSE);
            }
            else
            {
                gtk_widget_set_sensitive (dialog->button_ok, TRUE);
                gtk_label_set_markup (GTK_LABEL (dialog->label_note), NULL);
            }
        }
}

static gboolean
setup_check_password (UserPassword *dialog)
{
    check_password_strength (dialog);
    update_sensitivity (dialog);
    update_password_match (dialog);

    return TRUE;
}

static void now_set_password (GtkRadioButton *button, gpointer data)
{
    UserPassword *dialog = USER_PASSWORD (data);

    gtk_widget_set_sensitive (dialog->password_entry, TRUE);
    gtk_widget_set_sensitive (dialog->verify_entry, FALSE);
    gtk_widget_set_sensitive (dialog->level_bar, TRUE);
    gtk_widget_set_sensitive (dialog->button_ok, FALSE);

    if (dialog->check_password_time_id > 0)
    {
        g_source_remove (dialog->check_password_time_id);
        dialog->check_password_time_id = 0;
    }

    dialog->check_password_time_id = g_timeout_add (PASSWORD_CHECK_TIMEOUT,
                                                   (GSourceFunc)setup_check_password,
                                                    dialog);
    dialog->password_mode =  ACT_USER_PASSWORD_MODE_REGULAR;
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
static void SetNewPass(UserPassword *dialog)
{
    const char *password;
    
    if (dialog->password_mode == ACT_USER_PASSWORD_MODE_SET_AT_LOGIN)
    {
        act_user_set_password_mode (dialog->user, dialog->password_mode);
        dialog->label = g_strdup (_("Set up next time"));
    }
    else
    {
        password =  gtk_entry_get_text (GTK_ENTRY (dialog->password_entry));
        act_user_set_password_mode (dialog->user, dialog->password_mode);
        act_user_set_password (dialog->user, password, "");
        dialog->label = g_strdup ("●●●●●●");
    }
    g_signal_emit (dialog, signals[CHANGED], 0);
}

static void user_password_set_mode (UserPassword *dialog, ActUser *user)
{
    int passtype;

    passtype = act_user_get_password_mode (user);

    if(passtype == ACT_USER_PASSWORD_MODE_REGULAR)
    {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->now_button), FALSE);
    }
    else if (passtype == ACT_USER_PASSWORD_MODE_SET_AT_LOGIN)
    {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->now_button), TRUE);
    }
}

static void CloseNewPassWindow(UserPassword *dialog)
{
    gtk_widget_destroy(GTK_WIDGET(dialog));
}

static void
user_password_init (UserPassword *dialog)
{
    GtkWidget *box;
    GtkWidget *dialog_area;
    GtkWidget *table;
    GtkWidget *label;
    GtkWidget *now_button;
    GSList    *radio_group;
    GtkWidget *login_button;
    GtkWidget *hseparator;

    dialog->check_password_time_id = 0;
    dialog->password_mode = ACT_USER_PASSWORD_MODE_SET_AT_LOGIN;

    gtk_container_set_border_width (GTK_CONTAINER (dialog), 20);
    gtk_window_set_deletable (GTK_WINDOW (dialog), FALSE);
    gtk_window_set_default_size (GTK_WINDOW (dialog), 450, 200);

    dialog_add_button_with_icon_name ( GTK_DIALOG (dialog),
                                      _("Close"), 
                                      "window-close", 
                                       GTK_RESPONSE_CLOSE);

    dialog->button_ok = dialog_add_button_with_icon_name (GTK_DIALOG (dialog),
                                                         _("Confirm"),
                                                         "emblem-default",
                                                          GTK_RESPONSE_OK);

    dialog_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
    box =  gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start (GTK_BOX (dialog_area), box, TRUE, TRUE, 8);

    table = gtk_grid_new ();
    gtk_grid_set_row_spacing(GTK_GRID(table), 10);
    gtk_grid_set_column_spacing(GTK_GRID(table), 10);
    gtk_box_pack_start (GTK_BOX (box), table, TRUE, TRUE, 0);
    gtk_grid_set_column_homogeneous (GTK_GRID (table), TRUE);

    label = gtk_label_new (_("Password"));
    gtk_grid_attach (GTK_GRID (table), label, 0, 0, 1, 1);

    login_button = gtk_radio_button_new_with_label (NULL, _("Set up next time"));
    radio_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (login_button));
    gtk_grid_attach (GTK_GRID (table), login_button, 0, 1, 5, 1);
    g_signal_connect (login_button,
                    "toggled",
                     G_CALLBACK (next_login_set_password),
                     dialog);

    now_button = gtk_radio_button_new_with_label (radio_group, _("Now set up"));
    gtk_grid_attach (GTK_GRID (table), now_button, 0, 2, 5, 1);
    g_signal_connect (now_button,
                     "toggled",
                      G_CALLBACK (now_set_password),
                      dialog);
    dialog->login_button = login_button;
    dialog->now_button = now_button;

    label = gtk_label_new (NULL);
    SetLableFontType (label, "gray", 11, _("User Password"), TRUE);
    gtk_grid_attach (GTK_GRID (table), label, 0, 3, 1, 1);

    dialog->password_entry = gtk_entry_new ();
    gtk_entry_set_icon_from_icon_name (GTK_ENTRY (dialog->password_entry),
                                       GTK_ENTRY_ICON_SECONDARY,
                                      "system-run");

    gtk_entry_set_icon_tooltip_text (GTK_ENTRY (dialog->password_entry),
                                     GTK_ENTRY_ICON_SECONDARY,
                                    _("generation password"));
    gtk_entry_set_max_length (GTK_ENTRY (dialog->password_entry), 20);
    gtk_grid_attach (GTK_GRID (table), dialog->password_entry, 1, 3, 4, 1);

    dialog->level_bar = gtk_level_bar_new ();
    gtk_level_bar_set_min_value (GTK_LEVEL_BAR (dialog->level_bar), 0.0);
    gtk_level_bar_set_max_value (GTK_LEVEL_BAR (dialog->level_bar), 5.0);
    gtk_level_bar_set_mode (GTK_LEVEL_BAR (dialog->level_bar), GTK_LEVEL_BAR_MODE_DISCRETE);
    gtk_grid_attach (GTK_GRID (table), dialog->level_bar, 1, 4, 4, 1);

    dialog->label_note = gtk_label_new (NULL);
    gtk_grid_attach (GTK_GRID (table), dialog->label_note, 0, 5, 4, 1);

    label = gtk_label_new (NULL);
    SetLableFontType (label, "gray", 11, _("Confirm"), TRUE);
    gtk_grid_attach (GTK_GRID (table), label, 0, 6, 1, 1);

    dialog->verify_entry = gtk_entry_new ();
    gtk_entry_set_visibility (GTK_ENTRY (dialog->verify_entry), FALSE);
    gtk_entry_set_max_length (GTK_ENTRY(dialog->verify_entry),20);
    gtk_entry_set_visibility (GTK_ENTRY(dialog->verify_entry),FALSE);
    gtk_grid_attach(GTK_GRID(table) ,dialog->verify_entry, 1, 6, 4, 1);
    g_signal_connect (G_OBJECT (dialog->password_entry),
                     "icon-press",
                      G_CALLBACK(AutoGenera),
                      dialog->verify_entry);

    label = gtk_label_new(NULL);
    gtk_grid_attach (GTK_GRID (table), label, 0, 7, 4, 1);

    gtk_widget_set_sensitive (dialog->password_entry, FALSE);
    gtk_widget_set_sensitive (dialog->verify_entry, FALSE);
    hseparator = gtk_separator_new (GTK_ORIENTATION_VERTICAL);
    gtk_grid_attach(GTK_GRID(table), hseparator, 0, 8, 5, 1);
}

static void
user_password_destroy (GtkWidget *obj)
{
    UserPassword *dialog = USER_PASSWORD (obj);

    if (dialog->check_password_time_id != 0)
    {
        g_source_remove (dialog->check_password_time_id);
        dialog->check_password_time_id = 0;
    }
    g_clear_object (&dialog->user);

    if (dialog->label != NULL)
    {
        g_free (dialog->label);
        dialog->label = NULL;
    }
}

static void
user_password_response (GtkDialog *dia,
                        int        response_id)
{

    UserPassword *dialog = USER_PASSWORD (dia);

    switch (response_id) 
    {
    case GTK_RESPONSE_CLOSE:
        CloseNewPassWindow (dialog);
        break;
    case GTK_RESPONSE_OK:
        SetNewPass(dialog);
        CloseNewPassWindow (dialog);
        break;
    default:
        break;
    }
}

static void
user_password_class_init (UserPasswordClass *klass)
{
    GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    widget_class->destroy = user_password_destroy;
    dialog_class->response = user_password_response;
    signals [CHANGED] =
         g_signal_new ("changed",
                       G_TYPE_FROM_CLASS (klass),
                       G_SIGNAL_RUN_LAST,
                       0,
                       NULL, NULL,
                       g_cclosure_marshal_VOID__VOID,
                       G_TYPE_NONE, 0);
}

char *user_password_get_label (UserPassword *dialog)
{
    return dialog->label;
}

UserPassword *
user_password_new (ActUser *user)
{
    UserPassword  *dialog;
    int            password_type;
    char          *label;

    g_return_val_if_fail (ACT_IS_USER (user), NULL);

    dialog = g_object_new (USER_TYPE_PASSWORD,
                          "use-header-bar", 1,
                           NULL);
    dialog->user = g_object_ref (user);
    label = GetPasswordModeText (user, &password_type);

    dialog->label = g_strdup (label);
    user_password_set_mode (dialog, user);

    return dialog;
}
