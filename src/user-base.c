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
#include "user-history.h"
#include "user-language.h"

enum
{
    GROUP_VIEWED,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };
struct _UserBasePrivate
{
    ActUser    *user;
    GtkWidget  *combox;
    GtkWidget  *button_lang;
    GtkWidget  *button_password;
    GtkWidget  *button_time;
    GtkWidget  *switch_login;
    GtkWidget  *button_group;
};

G_DEFINE_TYPE_WITH_PRIVATE (UserBase, user_base, GTK_TYPE_GRID)
static void user_language_set_done (UserLanguage *chooser, GtkButton *button)
{
    gchar *name = NULL;
    gchar *lang;

    lang = user_language_get_language (chooser);
    name = mate_get_language_from_locale (lang, NULL);
    gtk_button_set_label (button, name);

    g_free (name);
}
static void
change_language (GtkButton   *button,
                 UserBase    *base)
{
    const gchar  *current_language;
    UserLanguage *user_language;

    current_language = GetUserLang (base->priv->user);

    user_language = user_language_new (base->priv->user);
    g_signal_connect (G_OBJECT(user_language),
                    "lang-changed",
                     G_CALLBACK(user_language_set_done),
                     button);

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
static void SwitchState (GtkSwitch *switch_login,
                         gboolean   state,
                         UserBase  *base)
{
    GSList    *list;
    GSList    *l;
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
                if (act_user_get_uid (u) != act_user_get_uid (base->priv->user)) 
                {
                    act_user_set_automatic_login (u, FALSE);
                }
            }
            g_slist_free (list);
            act_user_set_automatic_login (base->priv->user, TRUE);
        }
        else
            act_user_set_automatic_login (base->priv->user,FALSE);
    }
}

static void user_password_set_done (UserPassword *dialog, GtkButton *button)
{
    char *label;

    label = user_password_get_label (dialog);
    gtk_button_set_label (button, label);
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
static void ChangePass (GtkButton *button, UserBase *base)
{
    UserPassword *dialog;

    dialog = user_password_new (base->priv->user);
    g_signal_connect (G_OBJECT(dialog),
                    "changed",
                     G_CALLBACK(user_password_set_done),
                     button);

    gtk_widget_show_all (GTK_WIDGET (dialog));
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void ViewLoginHistory (GtkWidget *widget, UserBase *base)
{
    LoginHistoryDialog *dialog;

    dialog = login_history_dialog_new (base->priv->user);
    gtk_widget_show_all(GTK_WIDGET(dialog));
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
static void ComboSelectUserType(GtkWidget *widget, UserBase *base)
{
    gint account_type;

    if( Change ==0 )
    {
        account_type =  gtk_combo_box_get_active (GTK_COMBO_BOX(widget)) ? 
                                                  ACT_USER_ACCOUNT_TYPE_ADMINISTRATOR:
                                                  ACT_USER_ACCOUNT_TYPE_STANDARD;
        act_user_set_account_type(base->priv->user, account_type);
    }
}

static void set_user_group (GtkWidget *widget, UserBase *base)
{

    g_signal_emit (base, signals[GROUP_VIEWED], 0);
}

void user_base_update_user_info (UserBase *base, ActUser *user)
{
    char       *lang;
    const char *lang_id;
    const char *time;
    const char *label;
    int         PasswordType;

    gtk_combo_box_set_active (GTK_COMBO_BOX (base->priv->combox), GetUserType (user));

    lang_id = GetUserLang (user);
    if(lang_id == NULL)
    {
        gtk_button_set_label (GTK_BUTTON (base->priv->button_lang), _("No Settings"));
    }
    else
    {
        lang = mate_get_language_from_locale (lang_id, NULL);
        gtk_button_set_label (GTK_BUTTON (base->priv->button_lang), lang);
        g_free (lang);
    }

    label = GetPasswordModeText(user, &PasswordType);
    gtk_button_set_label (GTK_BUTTON (base->priv->button_password), label);

    gtk_switch_set_state (GTK_SWITCH (base->priv->switch_login), GetUserType (user));

    time = GetLoginTimeText (user);
    gtk_button_set_label (GTK_BUTTON (base->priv->button_time), time);
}

static void
user_base_fill (UserBase *base)
{
    GtkWidget  *label;

    /*user type*/
    label = gtk_label_new (NULL);
    SetLableFontType (label, "gray", 11, _("Account Type"), TRUE);
    gtk_grid_attach (GTK_GRID(base), label, 0 , 0 , 1 , 1);

    /*drop-down select boxes*/
    base->priv->combox = SetComboUserType (_("Standard"),_("Administrators"));
    gtk_grid_attach(GTK_GRID(base) , base->priv->combox , 1 , 0 , 2 , 1);
    g_signal_connect (G_OBJECT (base->priv->combox),
                    "changed",
                     G_CALLBACK(ComboSelectUserType),
                     base);

   /*select language*/
    label = gtk_label_new (NULL);
    SetLableFontType (label, "gray", 11, _("Language"), TRUE);
    gtk_grid_attach (GTK_GRID(base) , label, 0 , 1 , 1 , 1);

    base->priv->button_lang = gtk_button_new ();
    gtk_grid_attach(GTK_GRID(base) ,base->priv->button_lang, 1 , 1 , 2 , 1);
    g_signal_connect (base->priv->button_lang, 
                     "clicked",
                      G_CALLBACK (change_language),
                      base);

    /*set password*/
    label = gtk_label_new (NULL);
    SetLableFontType (label, "gray", 11, _("Password"), TRUE);
    gtk_grid_attach (GTK_GRID(base), label, 0, 2, 1, 1);

    base->priv->button_password = gtk_button_new ();
    g_signal_connect (base->priv->button_password, 
                     "clicked",
                      G_CALLBACK (ChangePass),
                      base);
    gtk_grid_attach (GTK_GRID (base), base->priv->button_password, 1, 2, 2, 1);

    /*auto login*/
    label = gtk_label_new (NULL);
    SetLableFontType (label, "gray", 11, _("Automatic logon"), TRUE);
    gtk_grid_attach (GTK_GRID (base) ,label, 0 , 3 , 1 , 1);

    base->priv->switch_login = gtk_switch_new ();
    gtk_grid_attach(GTK_GRID(base), base->priv->switch_login, 1, 3 , 1 , 1);
    g_signal_connect(base->priv->switch_login,
                    "state-set",
                     G_CALLBACK(SwitchState),
                     base);

    /*login time*/
    label = gtk_label_new (NULL);
    SetLableFontType (label, "gray", 11, _("Login time"), TRUE);
    gtk_grid_attach (GTK_GRID (base) ,label, 0 , 4 , 1 , 1);

    base->priv->button_time = gtk_button_new ();
    gtk_grid_attach(GTK_GRID(base) ,base->priv->button_time, 1 , 4 , 2 , 1);
    g_signal_connect (base->priv->button_time,
                     "clicked",
                      G_CALLBACK (ViewLoginHistory),
                      base);

    /*Group Manage*/
    label = gtk_label_new (NULL);
    SetLableFontType (label, "gray", 11, _("Group Manage"), TRUE);
    gtk_grid_attach(GTK_GRID(base) ,label, 0 , 5 , 1 , 1);

    base->priv->button_group = gtk_button_new_with_label (_("Setting Groups"));
    gtk_grid_attach (GTK_GRID (base), base->priv->button_group, 1, 5, 2, 1);

    g_signal_connect (base->priv->button_group,
                     "clicked",
                      G_CALLBACK (set_user_group),
                      base);

}

static GObject *
user_base_constructor (GType                  type,
                       guint                  n_construct_properties,
                       GObjectConstructParam *construct_properties)
{
    GObject    *obj;
    UserBase   *base;

    obj = G_OBJECT_CLASS (user_base_parent_class)->constructor (type,
                                      n_construct_properties,
                                      construct_properties);

    base = USER_BASE (obj);

    user_base_fill (base);

    return obj;
}

static void
user_base_dispose (GObject *object)
{
    UserBase *base = USER_BASE (object);

    g_clear_object (&base->priv->user);
    G_OBJECT_CLASS (user_base_parent_class)->dispose (object);
}

static void
user_base_class_init (UserBaseClass *klass)
{
    GObjectClass   *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->constructor = user_base_constructor;
    gobject_class->dispose = user_base_dispose;

    signals [GROUP_VIEWED] =
         g_signal_new ("group-viewed",
                       G_TYPE_FROM_CLASS (klass),
                       G_SIGNAL_RUN_LAST,
                       0,
                       NULL, NULL,
                       g_cclosure_marshal_VOID__VOID,
                       G_TYPE_NONE, 0);
}

static void
user_base_init (UserBase *base)
{
    base->priv = user_base_get_instance_private (base);
}

UserBase *
user_base_new (ActUser *user)
{
    UserBase *base;

    base = g_object_new (USER_TYPE_BASE, 
                        "column-homogeneous", TRUE,
                        "row-spacing", 10,
                        "column-spacing", 10,
                        NULL);

    base->priv->user = g_object_ref (user);

    return base;
}

void user_base_set_user (UserBase *base, ActUser *user)
{
    g_clear_object (&base->priv->user);
    base->priv->user = g_object_ref (user);
}

void user_base_set_public_sensitive (UserBase *base,
                                     gboolean  sensitive)
{

    gtk_widget_set_sensitive (base->priv->combox, sensitive);
    gtk_widget_set_sensitive (base->priv->button_lang, sensitive);
    gtk_widget_set_sensitive (base->priv->button_password, sensitive);
    gtk_widget_set_sensitive (base->priv->switch_login, sensitive);
}

void user_base_set_private_sensitive (UserBase *base,
                                      gboolean  sensitive)
{
    gtk_widget_set_sensitive (base->priv->button_time, sensitive);
    gtk_widget_set_sensitive (base->priv->button_group, sensitive);
}
