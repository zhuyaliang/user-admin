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

#include "user-share.h"
#include "user-admin.h"
#include "user-info.h"
#include "user-list.h"
#include "user-password.h"
#include <pwd.h>
#include <libintl.h>
#include <locale.h>
#include <glib/gi18n.h>
#include <grp.h>
#include <sys/types.h>
#include <libgroupservice/gas-group.h>
#include <libgroupservice/gas-group-manager.h>

#define  NUCONFIG      "/etc/mate-user-admin/nuconfig"
#define  KEYGROUPNAME  "nudefault"
#define  LANGKEY       "nulanguage"
#define  TYPEKEY       "nutype"
#define  GROUPKEY      "nugroups"

#define CHECK_TIME_OUT 600
struct _UserManagerPrivate
{
    GtkWidget     *button;
    GtkWidget     *name_entry;
    GtkWidget     *real_entry;
    GtkWidget     *label_note;
    GtkWidget     *password_entry;
    GtkWidget     *check_entry;
    GtkWidget     *level_bar;
    GtkWidget     *label_pass_note;
    GtkWidget     *label_space;
    int            name_time_id;       //Check the password format timer
    int            password_time_id;       //Check the Realname format timer
    char          *user_lang;
    char         **user_groups;
    int            user_type;
    gboolean       sensitive;
    gboolean       success;
    GPermission   *permission;
    GCancellable  *cancellable;
    ActUserPasswordMode password_mode;
};

enum
{
    PROP_0,
    PROP_USER_TYPE,
};

G_DEFINE_TYPE_WITH_PRIVATE (UserManager, user_manager, GTK_TYPE_DIALOG)

static void user_manager_response (GtkDialog *dialog,
                                   gint       response_id);

static gboolean user_manager_get_new_user_config (UserManager *dialog)
{
    GKeyFile         *Kconfig = NULL;
    g_autoptr(GError) error = NULL;
    g_auto(GStrv)     ConfigGroups = NULL;
    g_auto(GStrv)     unGroups = NULL;
    gsize             Length = 0;
    g_autofree char  *Value = NULL;
    gboolean          Type;

    Kconfig = g_key_file_new ();
    if (Kconfig == NULL)
    {
        mate_uesr_admin_log ("Warning","g_key_file_new fail");
        return FALSE;
    }
    if (!g_key_file_load_from_file (Kconfig, NUCONFIG, G_KEY_FILE_NONE, &error))
    {
        mate_uesr_admin_log ("Warning","Error loading key file: %s", error->message);
        goto EXIT;
    }
    ConfigGroups = g_key_file_get_groups (Kconfig, &Length);
    if (g_strv_length (ConfigGroups) <= 0)
    {
        mate_uesr_admin_log ("Warning","key file format errors are not grouped");
        goto EXIT;
    }
    if (g_key_file_has_key (Kconfig, KEYGROUPNAME, LANGKEY,&error) == FALSE)
    {
        mate_uesr_admin_log ("Warning", "key file format errors %s", error->message);
        goto EXIT;
    }
    Value = g_key_file_get_string(Kconfig,KEYGROUPNAME,LANGKEY,&error);
    if(Value == NULL)
    {
        mate_uesr_admin_log ("Warning","key file format errors %s",error->message);
        goto EXIT;
    }
    if(mate_get_language_from_locale(Value,NULL) == NULL)
    {
        mate_uesr_admin_log ("Warning", "key file language format errors Language unavailability");
        goto EXIT;
    }
    dialog->priv->user_lang = g_strdup (Value);

    Type = g_key_file_get_boolean (Kconfig, KEYGROUPNAME, TYPEKEY, &error);
    if(Type == FALSE && error != NULL)
    {
        mate_uesr_admin_log ("Warning", "key file user type format errors %s", error->message);
        goto EXIT;
    }
    dialog->priv->user_type = Type;

    unGroups = g_key_file_get_string_list (Kconfig, KEYGROUPNAME, GROUPKEY, &Length, &error);
    if (unGroups == NULL)
    {
        mate_uesr_admin_log ("Info", "key file No default add group is set for new users");
        g_key_file_free (Kconfig);
        return TRUE;
    }
    dialog->priv->user_groups = g_strdupv (unGroups);
    g_key_file_free (Kconfig);
    return TRUE;

EXIT:
    dialog->priv->user_groups = NULL;
    dialog->priv->user_lang = NULL;
    g_key_file_free (Kconfig);
    return FALSE;
}

static void DeleteOldUserDone (ActUserManager *manager,
                               GAsyncResult   *res,
                               gpointer        data)
{
    GError *error = NULL;

    if (!act_user_manager_delete_user_finish (manager, res, &error))
    {
        if (!g_error_matches (error, ACT_USER_MANAGER_ERROR, ACT_USER_MANAGER_ERROR_PERMISSION_DENIED))
        {
            MessageReport(_("Remove User"),
                          error->message,
                          ERROR);
            g_error_free (error);
        }
    }

}

/******************************************************************************
* Function:              RemoveUser 
*        
* Explain: delete user
*        
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  09/05/2018
******************************************************************************/
void RemoveUser (ActUser *user)
{
    int             nRet;
    gboolean        RemoveType = TRUE;
    ActUserManager *Manager;

    Manager = act_user_manager_get_default ();
    if (act_user_get_uid (user) == getuid ())
    {
        MessageReport (_("Remove User"),
                       _("You cannot delete your own account."),
                       ERROR);
        return;

    }
    else if (act_user_is_logged_in_anywhere (user))
    {
        MessageReport (_("Remove User"),
                       _("user is still logged in"),
                       ERROR);
        return;
    }
    nRet = MessageReport (_("Remove User"),
                          _("Whether to remove the user's home directory"),
                          QUESTION);
    if (nRet == GTK_RESPONSE_NO)
    {
         RemoveType = FALSE;
    }
    else if (nRet == GTK_RESPONSE_DELETE_EVENT ||
             nRet ==  GTK_RESPONSE_ACCEPT)
    {
         return;
    }
     /* remove autologin */
    if (act_user_get_automatic_login (user))
    {
        act_user_set_automatic_login (user, FALSE);
    }

    act_user_manager_delete_user_async (Manager,
                                        user,
                                        RemoveType,
                                        NULL,
                                        (GAsyncReadyCallback)DeleteOldUserDone,
                                        NULL);

}

static gboolean CheckUserNameUsed (const gchar *UserName)
{
    struct passwd *pwent;

    if (UserName == NULL || UserName[0] == '\0')
    {
        return FALSE;
    }

    pwent = getpwnam (UserName);

    return pwent != NULL;
}

/******************************************************************************
* Function:              UserNameValidCheck 
*        
* Explain: check the validity of a username.include Is it empty
*          Whether or not to use and character check
*        
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  09/05/2018
******************************************************************************/
static gboolean UserNameValidCheck (const gchar *UserName, gchar **Message)
{
    gboolean empty;
    gboolean home_use;
    gboolean in_use;
    gboolean valid;
    const gchar *c;
    char HomeName[32] = { 0 };

    if (UserName == NULL || UserName[0] == '\0')
    {
        empty = TRUE;
        in_use = FALSE;
        home_use = FALSE;
    }
    else
    {
        empty = FALSE;
        in_use = CheckUserNameUsed (UserName);
        sprintf (HomeName, "/home/%s", UserName);
        home_use = access (HomeName, F_OK ) == 0;
    }
    valid = TRUE;
    if (!in_use && !empty && !home_use)
    {
        for (c = UserName; *c; c++)
        {
            if (! ((*c >= 'a' && *c <= 'z') ||
                   (*c >= 'A' && *c <= 'Z') ||
                   (*c >= '0' && *c <= '9') ||
                   (*c == '_') || (*c == '.') ||
                   (*c == '-' && c != UserName)))
               valid = FALSE;
        }
    }

    valid = !empty && !in_use && !home_use && valid;
    if (!empty && (in_use || home_use || !valid))
    {
        if (in_use)
        {
            *Message = g_strdup (_("Repeat of user name.Please try another"));
        }
        else if(home_use)
        {
            *Message = g_strdup (_("Repeat of user home name.Please try another"));
        }
        else if (UserName[0] == '-')
        {
            *Message = g_strdup (_("The username cannot start with a - ."));
        }
        else
        {
            *Message = g_strdup (_("The username should only consist of upper and lower case \nletters from a-z,digits and the following characters: . - _"));
        }
    }

    return valid;
}

/******************************************************************************
* Function:             CheckName 
*        
* Explain: 800 milliseconds to check the validity of a username
*        
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  09/05/2018
******************************************************************************/
static gboolean check_user_name (UserManager *dialog)
{
    gboolean    Valid;
    char       *Message = NULL;
    const char *user_name_text;
    const char *real_name_text;
    const char *FixedNote = _("This will be used to name your home folder and can't be changed");

    user_name_text = gtk_entry_get_text (GTK_ENTRY (dialog->priv->name_entry));
    if(strlen(user_name_text) == 0)
    {
        gtk_widget_set_sensitive (dialog->priv->button, FALSE);
        gtk_entry_set_icon_from_icon_name (GTK_ENTRY (dialog->priv->name_entry),
                                      GTK_ENTRY_ICON_SECONDARY,
                                      NULL);
        return TRUE;
    }
    real_name_text = gtk_entry_get_text (GTK_ENTRY (dialog->priv->real_entry));
    if(strlen(real_name_text) > 0)
    {
        gtk_entry_set_icon_from_icon_name(GTK_ENTRY(dialog->priv->real_entry),
                                          GTK_ENTRY_ICON_SECONDARY,
                                         "emblem-ok-symbolic");
        dialog->priv->sensitive = TRUE;
    }
    else
    {
        gtk_entry_set_icon_from_icon_name(GTK_ENTRY(dialog->priv->real_entry),
                                          GTK_ENTRY_ICON_SECONDARY,
                                          NULL);
        gtk_widget_set_sensitive (dialog->priv->button, FALSE);
    }

    Valid = UserNameValidCheck (user_name_text, &Message);
    if(Message != NULL)
        SetLableFontType (dialog->priv->label_note, "red", 10, Message, FALSE);
    else
    {
        gtk_entry_set_icon_from_icon_name (GTK_ENTRY(dialog->priv->name_entry),
                                           GTK_ENTRY_ICON_SECONDARY,
                                          "emblem-ok-symbolic");

        SetLableFontType (dialog->priv->label_note,
                         "gray",
                          10,
                          FixedNote,
                          FALSE);
    }
    dialog->priv->sensitive = Valid & dialog->priv->sensitive & dialog->priv->success;
    gtk_widget_set_sensitive (dialog->priv->button, dialog->priv->sensitive);
    return TRUE;
}

static const gchar *GetNewUserPassword (GtkWidget *EntryPass1,GtkWidget *EntryPass2)
{
    const char *p1 = NULL;
    const char *p2 = NULL;

    p1 =  gtk_entry_get_text (GTK_ENTRY (EntryPass1));
    p2 =  gtk_entry_get_text (GTK_ENTRY (EntryPass2));

    if (strcmp (p1, p2) != 0 )
    {
        return NULL;
    }
    return p2;
}

static const gchar *GetNewUserLang (UserManager *dialog)
{
    if(dialog->priv->user_lang != NULL)
    {
        mate_uesr_admin_log ("Debug","nuLang = %s", dialog->priv->user_lang);
        return dialog->priv->user_lang;
    }
    return "en_US.utf8";
}

static void add_user_to_group (const char *name, char **groups)
{
    int groups_num = 0,i;
    GasGroup        *gas = NULL;
    GasGroupManager *manage;

    if(groups != NULL)
    {
        manage = gas_group_manager_get_default ();
        gas_group_manager_list_groups (manage);
        groups_num = g_strv_length (groups);
        for(i = 0; i < groups_num; i++)
        {
            if (getgrnam (groups[i]) == NULL)
            {
                if (g_utf8_strchr (groups[i],-1,' ') != NULL)
                {
                    mate_uesr_admin_log ("Warning","Configuration file error,Please delete the extra space keys");
                }
                mate_uesr_admin_log ("Warning","Configuration file error, no group %s",groups[i]);
                continue;
            }
            gas = gas_group_manager_get_group (manage,groups[i]);
            if(gas == NULL)
            {
                mate_uesr_admin_log ("Warning","Configuration file error, no group %s",groups[i]);
                continue;
            }
            mate_uesr_admin_log ("Debug","group name %s",groups[i]);
            gas_group_add_user_group (gas, name);
        }

    }
}

static void close_dialog (GtkWidget *widget)
{
    gtk_widget_destroy (widget);
}

static void set_new_user_base_info (ActUser         *user,
                                    GParamSpec      *pspec,
                                    UserManager     *dialog)
{
    const char *NewUserLang;
    const char *Password;
    const char *un;

    NewUserLang  = GetNewUserLang (dialog);
    act_user_set_account_type (user, dialog->priv->user_type);
    act_user_set_language (user, NewUserLang);

    if(dialog->priv->password_mode == ACT_USER_PASSWORD_MODE_SET_AT_LOGIN)
    {
        act_user_set_password_mode (user,ACT_USER_PASSWORD_MODE_SET_AT_LOGIN);
    }
    else
    {
        Password = GetNewUserPassword (dialog->priv->password_entry, dialog->priv->check_entry);
        act_user_set_password_mode (user, ACT_USER_PASSWORD_MODE_REGULAR);
        act_user_set_password (user,Password, "");
    }
    un = gtk_entry_get_text (GTK_ENTRY (dialog->priv->name_entry));
    mate_uesr_admin_log ("Debug","New user: %s lang %s", act_user_get_user_name (user), NewUserLang);
    add_user_to_group (un, dialog->priv->user_groups);

    close_dialog (GTK_WIDGET (dialog));
}

static void CreateUserDone (ActUserManager  *Manager,
                            GAsyncResult    *res,
                            UserManager     *dialog)
{
    GError  *error = NULL;
    ActUser *user;

    user = act_user_manager_create_user_finish (Manager, res, &error);
    if(user == NULL)
    {
        MessageReport (_("Creating User"), error->message,ERROR);
        g_error_free (error);
        close_dialog (GTK_WIDGET (dialog));
        return;
    }
    mate_uesr_admin_log ("Debug","Created user: %s success", act_user_get_user_name (user));
    if (act_user_is_loaded (user))
        set_new_user_base_info (user, NULL, dialog);
    else
        g_signal_connect (user,
                         "notify::is-loaded",
                          G_CALLBACK (set_new_user_base_info),
                          dialog);

}

/******************************************************************************
* Function:             CreateNewUser
*        
* Explain: Create new users
*          step 1 Check the password for the two input
*          step 2 Add the user to the list 
*          step 3 create user
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  09/05/2018
******************************************************************************/
static void CreateLocalNewUser(UserManager *dialog)
{
    ActUserManager *Manager;
    const char *rn;
    const char *un;

    rn = gtk_entry_get_text (GTK_ENTRY (dialog->priv->real_entry));
    un = gtk_entry_get_text (GTK_ENTRY (dialog->priv->name_entry));

    if (dialog->priv->name_time_id != 0)
    {
        g_source_remove (dialog->priv->name_time_id);
        dialog->priv->name_time_id = 0;
    }
    Manager = act_user_manager_get_default ();
    mate_uesr_admin_log ("Debug","username %s realname %s",
                         un, rn);
    act_user_manager_create_user_async (Manager,
                                        un,
                                        rn,
                                        ACT_USER_ACCOUNT_TYPE_STANDARD,
                                        dialog->priv->cancellable,
                                        (GAsyncReadyCallback)CreateUserDone,
                                        dialog);

}
/******************************************************************************
* Function:             SetNewUserInfo 
*        
* Explain: New user name and type and language
*        
* Input:  @CanConfig 
*         
*         TERU    Use configuration files /etc/mate-user-admin/unconfig
*         FALSE   Use default values
*        
* Output: 
*        
* Author:  zhuyaliang  09/05/2018
******************************************************************************/
static void SetNewUserInfo (GtkWidget   *Vbox,
                            UserManager *dialog,
                            gboolean     CanConfig)
{
    GtkWidget *table;
    GtkWidget *LabelUserName;
    GtkWidget *LabelRealName;
    GtkWidget *LabelUserType;
    GtkWidget *combox;
    const char *FixedNote = _("This will be used to name your home folder and can't be changed");

    table = gtk_grid_new ();
    gtk_box_pack_start (GTK_BOX (Vbox), table, TRUE, TRUE, 0);
    gtk_grid_set_column_homogeneous (GTK_GRID (table), TRUE);

    LabelUserName = gtk_label_new (NULL);
    SetLableFontType (LabelUserName, "gray", 11, _("User Name"), TRUE);
    gtk_grid_attach (GTK_GRID (table), LabelUserName , 0, 0, 1, 1);

    dialog->priv->name_entry = gtk_entry_new ();
    dialog->priv->name_time_id = g_timeout_add (CHECK_TIME_OUT, (GSourceFunc)check_user_name, dialog);
    gtk_entry_set_max_length (GTK_ENTRY (dialog->priv->name_entry),24);
    gtk_grid_attach (GTK_GRID (table), dialog->priv->name_entry,1, 0, 3, 1);

    dialog->priv->label_note = gtk_label_new (NULL);
    SetLableFontType (dialog->priv->label_note, "gray", 10, FixedNote, TRUE);
    gtk_grid_attach (GTK_GRID (table) ,dialog->priv->label_note, 0, 1, 4, 1);

    LabelRealName = gtk_label_new (NULL);
    SetLableFontType (LabelRealName, "gray", 11, _("Full Name"), TRUE);
    gtk_grid_attach (GTK_GRID (table), LabelRealName, 0, 2, 1, 1);

    dialog->priv->real_entry = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY (dialog->priv->real_entry), 24);
    gtk_grid_attach (GTK_GRID (table), dialog->priv->real_entry, 1 ,2 ,3 ,1);

    LabelUserType = gtk_label_new (NULL);
    SetLableFontType (LabelUserType, "gray", 11, _("Account Type"), TRUE);
    gtk_grid_attach(GTK_GRID(table), LabelUserType, 0, 3, 1, 1);

    combox = SetComboUserType (_("Standard"), _("Administrators"));
    g_object_bind_property (combox, "active", dialog, "user-type", 0);
    gtk_combo_box_set_active (GTK_COMBO_BOX (combox), dialog->priv->user_type);
    gtk_grid_attach (GTK_GRID (table), combox, 1, 3, 3, 1);

    gtk_grid_set_row_spacing (GTK_GRID (table), 10);
    gtk_grid_set_column_spacing (GTK_GRID (table), 10);
}

static void ComparePassword (UserManager *dialog)
{
    const gchar *password;
    const gchar *NoteMessage = _("The passwords entered twice are different");

    password = gtk_entry_get_text (GTK_ENTRY (dialog->priv->check_entry));
    if (strlen (password) <=0)
        return;
    if (GetNewUserPassword (dialog->priv->password_entry, dialog->priv->check_entry) == NULL)
    {
        gtk_entry_set_icon_from_icon_name (GTK_ENTRY (dialog->priv->check_entry),
                                           GTK_ENTRY_ICON_SECONDARY,
                                           NULL);
        SetLableFontType (dialog->priv->label_space, "red", 10, NoteMessage, FALSE);

        dialog->priv->success = FALSE;
        return;
    }
    gtk_entry_set_icon_from_icon_name (GTK_ENTRY (dialog->priv->check_entry),
                                       GTK_ENTRY_ICON_SECONDARY,
                                      "emblem-ok-symbolic");
    gtk_label_set_markup (GTK_LABEL (dialog->priv->label_space), NULL);
    dialog->priv->success = TRUE;
}
/******************************************************************************
* Function:              TimeFun
*        
* Explain: Whether the timer is legitimate to check the password
*        
* Input:   Password valid    UnlockFlag = 0
*          Password invalid  UnlockFlag = 1
* Output: 
*        
* Author:  zhuyaliang  09/05/2018
******************************************************************************/
static gboolean start_check_entry (gpointer data)
{
    UserManager *dialog = USER_MANAGER (data);
    const char  *s;
    int          Level;
    const char  *Message;
    const char  *tip = _("Mixed passwords improve security");

    s = gtk_entry_get_text (GTK_ENTRY (dialog->priv->password_entry));
    if (strlen (s) == 0)
    {
        dialog->priv->success = FALSE;
        gtk_entry_set_text (GTK_ENTRY (dialog->priv->check_entry), "");
        SetLableFontType (dialog->priv->label_pass_note, "gray", 10, tip, FALSE);
        return TRUE;
    }
    Level = GetPassStrength (s, NULL,NULL,&Message);
    gtk_level_bar_set_value (GTK_LEVEL_BAR (dialog->priv->level_bar), Level);

    if(Message == NULL && Level > 1)
    {
        gtk_entry_set_icon_from_icon_name (GTK_ENTRY (dialog->priv->password_entry),
                                           GTK_ENTRY_ICON_SECONDARY,
                                           "emblem-ok-symbolic");
        gtk_widget_set_sensitive (dialog->priv->check_entry, TRUE);
        SetLableFontType (dialog->priv->label_pass_note, "gray", 10, tip, FALSE);
        ComparePassword (dialog);
        return TRUE;
    }
    gtk_entry_set_icon_from_icon_name (GTK_ENTRY (dialog->priv->password_entry),
                                      GTK_ENTRY_ICON_SECONDARY,
                                     "system-run");
    SetLableFontType (dialog->priv->label_pass_note, "red", 10, Message, FALSE);

    dialog->priv->success = FALSE;
    return TRUE;
}

/******************************************************************************
* Function:             LoginSetPass
*        
* Explain: Set up next time
*        
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  09/05/2018
******************************************************************************/
static void next_login_set_password (GtkRadioButton *button, gpointer data)
{
    UserManager *dialog = USER_MANAGER (data);

    gtk_widget_set_sensitive (dialog->priv->password_entry, FALSE);  //lock widget
    gtk_widget_set_sensitive (dialog->priv->check_entry, FALSE);
    gtk_entry_set_text (GTK_ENTRY (dialog->priv->check_entry), "");
    gtk_entry_set_text (GTK_ENTRY (dialog->priv->password_entry), "");
    gtk_level_bar_set_value (GTK_LEVEL_BAR (dialog->priv->level_bar), 0);

    if(dialog->priv->password_time_id > 0)                //因为不需要检查密码所以移除定时器
    {
        g_source_remove (dialog->priv->password_time_id);
        dialog->priv->password_time_id = 0;
    }
    dialog->priv->password_mode = ACT_USER_PASSWORD_MODE_SET_AT_LOGIN;
    dialog->priv->success = TRUE;
}
/******************************************************************************
* Function:            NowSetNewUserPass 
*        
* Explain: Now set the password,Whether the timer is legitimate to check the password
*        
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  09/05/2018
******************************************************************************/
static void now_set_password (GtkRadioButton *button, gpointer data)
{
    UserManager *dialog = USER_MANAGER (data);

    gtk_widget_set_sensitive (dialog->priv->password_entry, TRUE);
    gtk_widget_set_sensitive (dialog->priv->button, FALSE);
    dialog->priv->success = FALSE;
    gtk_widget_set_sensitive(dialog->priv->check_entry, TRUE);

    dialog->priv->password_time_id = g_timeout_add (CHECK_TIME_OUT, (GSourceFunc) start_check_entry, dialog);
    dialog->priv->password_mode = ACT_USER_PASSWORD_MODE_REGULAR;
}
/******************************************************************************
* Function:              GetNewUserPass 
*        
* Explain: Set new user password
*        
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  09/05/2018
******************************************************************************/
static void SetNewUserPass (GtkWidget *Vbox, UserManager *dialog)
{
    GtkWidget *Table;
    GtkWidget *LabelTitle;
    GtkWidget *radio_button;
    GtkWidget *radio_button1;
    GSList    *RadioGroup;
    GtkWidget *LabelPass;
    GtkWidget *LabelConfirm;

    Table = gtk_grid_new ();
    gtk_box_pack_start (GTK_BOX (Vbox), Table, TRUE, TRUE, 20);
    gtk_grid_set_column_homogeneous (GTK_GRID (Table), TRUE);
    LabelTitle = gtk_label_new (_("Password"));
    gtk_grid_attach (GTK_GRID (Table), LabelTitle, 0, 0, 1, 1);

    //新建两个单选按钮
    radio_button = gtk_radio_button_new_with_label (NULL, _("Set up next time"));
    RadioGroup = gtk_radio_button_get_group (GTK_RADIO_BUTTON (radio_button));
    gtk_grid_attach (GTK_GRID (Table), radio_button, 0, 1, 5, 1);
    g_signal_connect (radio_button,
                     "released",
                      G_CALLBACK (next_login_set_password),
                      dialog);

    radio_button1 = gtk_radio_button_new_with_label (RadioGroup, _("Now set the password"));
    gtk_grid_attach (GTK_GRID (Table), radio_button1, 0, 2, 5, 1);
    g_signal_connect (radio_button1,
                     "released",
                      G_CALLBACK (now_set_password),
                      dialog);

    LabelPass = gtk_label_new (NULL);
    SetLableFontType (LabelPass, "gray", 11, _("Password"), TRUE);
    gtk_grid_attach(GTK_GRID(Table), LabelPass, 0, 3, 1, 1);

    dialog->priv->password_entry = gtk_entry_new();
    gtk_entry_set_visibility (GTK_ENTRY (dialog->priv->password_entry), FALSE);
    gtk_entry_set_icon_from_icon_name(GTK_ENTRY (dialog->priv->password_entry),
                                      GTK_ENTRY_ICON_SECONDARY,
                                     "system-run");
    gtk_entry_set_max_length (GTK_ENTRY (dialog->priv->password_entry), 24);
    gtk_grid_attach (GTK_GRID(Table), dialog->priv->password_entry, 1, 3, 4, 1);

    gtk_entry_set_icon_tooltip_text (GTK_ENTRY (dialog->priv->password_entry),
                                     GTK_ENTRY_ICON_SECONDARY,
                                    _("generation password"));

    dialog->priv->level_bar = gtk_level_bar_new ();
    gtk_level_bar_set_min_value (GTK_LEVEL_BAR (dialog->priv->level_bar), 0.0);
    gtk_level_bar_set_max_value (GTK_LEVEL_BAR (dialog->priv->level_bar), 5.0);
    gtk_level_bar_set_mode (GTK_LEVEL_BAR (dialog->priv->level_bar), GTK_LEVEL_BAR_MODE_DISCRETE);
    gtk_grid_attach (GTK_GRID (Table), dialog->priv->level_bar, 1, 4, 4, 1);

    dialog->priv->label_pass_note = gtk_label_new (NULL);
    gtk_grid_attach (GTK_GRID (Table) ,dialog->priv->label_pass_note, 0, 5, 4, 1);

    LabelConfirm = gtk_label_new (NULL);
    SetLableFontType (LabelConfirm, "gray", 11, _("Confirm"), TRUE);
    gtk_grid_attach(GTK_GRID(Table) ,LabelConfirm , 0, 6, 1, 1);

    dialog->priv->check_entry = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY (dialog->priv->check_entry),24);
    gtk_entry_set_visibility (GTK_ENTRY (dialog->priv->check_entry), FALSE);
    gtk_grid_attach (GTK_GRID (Table) ,dialog->priv->check_entry, 1, 6, 4, 1);
    g_signal_connect (G_OBJECT (dialog->priv->password_entry),
                     "icon-press",
                      G_CALLBACK (AutoGenera),
                      dialog->priv->check_entry);

    dialog->priv->label_space = gtk_label_new ("");
    gtk_grid_attach (GTK_GRID (Table) , dialog->priv->label_space , 0 , 7 , 4 , 1);

    gtk_widget_set_sensitive (dialog->priv->password_entry, FALSE);  //lock widget
    gtk_widget_set_sensitive (dialog->priv->check_entry, FALSE);
    gtk_widget_set_sensitive (dialog->priv->level_bar, FALSE);

    gtk_grid_set_row_spacing (GTK_GRID (Table), 10);
    gtk_grid_set_column_spacing (GTK_GRID (Table), 10);
}

static void GetPermission (GObject      *source_object,
                           GAsyncResult *res,
                           gpointer      data)
{
    UserManager *dialog = USER_MANAGER (data);
    GError *error = NULL;

    if (g_permission_acquire_finish (dialog->priv->permission, res, &error))
    {
        g_return_if_fail (g_permission_get_allowed (dialog->priv->permission));
        user_manager_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
    }
    else if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
    {
        mate_uesr_admin_log ("Warning","Failed to acquire permission: %s", error->message);
    }

    g_clear_error (&error);
    g_object_unref (dialog);
}

static void
user_manager_response (GtkDialog *dia,
                       gint       response_id)
{
    UserManager *dialog = USER_MANAGER (dia);

    switch (response_id)
    {
        case GTK_RESPONSE_OK:
            if (dialog->priv->permission && !g_permission_get_allowed (dialog->priv->permission))
            {
                g_permission_acquire_async (dialog->priv->permission,
                                            dialog->priv->cancellable,
                                            GetPermission,
                                            g_object_ref (dialog));
                return;
            }

            CreateLocalNewUser (dialog);
            break;
        case GTK_RESPONSE_CANCEL:
        case GTK_RESPONSE_DELETE_EVENT:
            g_cancellable_cancel (dialog->priv->cancellable);
            close_dialog (GTK_WIDGET(dia));
            break;
        default:
            break;
    }

}

static void
user_manager_destroy (GtkWidget *obj)
{
    UserManager *dialog = USER_MANAGER (obj);

    if (dialog->priv->name_time_id != 0)
    {
        g_source_remove (dialog->priv->name_time_id);
        dialog->priv->name_time_id = 0;
    }

    if (dialog->priv->password_time_id != 0)
    {
        g_source_remove (dialog->priv->password_time_id);
        dialog->priv->password_time_id = 0;
    }

    if (dialog->priv->user_groups != NULL)
    {
        g_strfreev (dialog->priv->user_groups);
    }

    if (dialog->priv->user_lang != NULL)
    {
        g_free (dialog->priv->user_lang);
    }
}

static void
user_manager_init (UserManager *dialog)
{
    GtkWidget *vbox;
    GtkWidget *Vbox1;
    GtkWidget *Vbox2;
    gboolean   ret;

    dialog->priv = user_manager_get_instance_private (dialog);
    dialog->priv->name_time_id = 0;
    dialog->priv->password_time_id = 0;
    dialog->priv->sensitive = FALSE;
    dialog->priv->success = TRUE;
    dialog->priv->user_type = ACT_USER_ACCOUNT_TYPE_STANDARD;
    dialog->priv->password_mode = ACT_USER_PASSWORD_MODE_SET_AT_LOGIN;

    gtk_widget_set_size_request (GTK_WIDGET (dialog), 500, 450);

    dialog_add_button_with_icon_name (GTK_DIALOG (dialog),
                                      _("Close"), 
                                     "window-close",
                                      GTK_RESPONSE_CANCEL);

    dialog->priv->button = dialog_add_button_with_icon_name (GTK_DIALOG (dialog),
                                                           _("Confirm"),
                                                           "emblem-default",
                                                            GTK_RESPONSE_OK);

    gtk_widget_set_sensitive (dialog->priv->button, dialog->priv->sensitive);
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
    gtk_widget_grab_default (dialog->priv->button);
    gtk_window_set_title (GTK_WINDOW (dialog), _("Create New User"));

    vbox =  gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                        vbox,
                        TRUE,
                        TRUE,
                        8);

    Vbox1 =  gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start (GTK_BOX (vbox), Vbox1, TRUE, TRUE, 0);

    Vbox2 =  gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start (GTK_BOX (vbox), Vbox2, TRUE, TRUE, 0);

    ret = user_manager_get_new_user_config (dialog);
    SetNewUserInfo (Vbox1, dialog, ret);
    SetNewUserPass (Vbox2, dialog);
}

static void
user_manager_get_property (GObject    *object,
                           guint       param_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
    UserManager *dialog = USER_MANAGER (object);

    switch (param_id)
    {
        case PROP_USER_TYPE:
            g_value_set_int (value, dialog->priv->user_type);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
            break;
    }
}

static void
user_manager_set_property (GObject      *object,
                           guint         param_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
    UserManager *dialog = USER_MANAGER (object);
    switch (param_id)
    {
        case PROP_USER_TYPE:
            dialog->priv->user_type = g_value_get_int (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
            break;
    }
}

static void
user_manager_class_init (UserManagerClass *klass)
{
    GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->set_property = user_manager_set_property;
    gobject_class->get_property = user_manager_get_property;

    widget_class->destroy = user_manager_destroy;
    dialog_class->response = user_manager_response;

    g_object_class_install_property (
            gobject_class,
            PROP_USER_TYPE,
            g_param_spec_int (
                    "user-type",
                    "UserType",
                    "User type of new user",
                    0,1,0,
                    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

UserManager *user_manager_new (void)
{
    UserManager *dialog;

    dialog = g_object_new (USER_TYPE_MANAGER, NULL);

    return dialog;
}

void AddNewUser(GtkWidget *widget, gpointer data)
{
    UserManager *dialog;

    dialog = user_manager_new ();
    gtk_widget_show_all (GTK_WIDGET (dialog));
}
