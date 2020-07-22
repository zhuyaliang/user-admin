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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "user.h"
#include "user-history.h"
#include "user-share.h"
#include "user-info.h"
#include "user-password.h"

G_DEFINE_TYPE (LoginHistoryDialog, login_history_dialog, GTK_TYPE_DIALOG)

typedef struct 
{
    gint64 login_time;
    gint64 logout_time;
    const gchar *type;
}LoginHistory;
static void show_week_label (LoginHistoryDialog *self)
{
    g_autofree gchar *label = NULL;
    GTimeSpan span;

    span = g_date_time_difference (self->current_week, self->week);
    if (span == 0) 
    {
        label = g_strdup (_("This Week"));
    }
    else if (span == G_TIME_SPAN_DAY * 7) 
    {
        label = g_strdup (_("Last Week"));
    }
    else 
    {
        g_autofree gchar *from = NULL;
        g_autofree gchar *to = NULL;
        g_autoptr(GDateTime) date = NULL;

        date = g_date_time_add_days (self->week, 6);
        from = g_date_time_format (self->week, ("%b %e"));
        if (g_date_time_get_year (self->week) == g_date_time_get_year (self->current_week)) 
        {
            to = g_date_time_format (date, ("%b %e"));
        }
        else 
        {
            to = g_date_time_format (date, ("%b %e, %Y"));
        }

        label = g_strdup_printf(("%s — %s"), from, to);
    }
    if(self->is_header == 1)
        gtk_header_bar_set_subtitle (self->header_bar, label);
    else
        gtk_window_set_title(GTK_WINDOW(self),label);
}
static void clear_history (LoginHistoryDialog *self)
{
    g_autoptr(GList) list = NULL;
    GList           *it;

    list = gtk_container_get_children (GTK_CONTAINER (self->history_box));
    for (it = list; it != NULL; it = it->next) 
    {
        gtk_container_remove (GTK_CONTAINER (self->history_box), GTK_WIDGET (it->data));
    }
}
static GArray * get_login_history (ActUser *user)
{
    GArray         *login_history;
    GVariantIter   *iter, *iter2;
    GVariant       *variant;
    const GVariant *value;
    const gchar    *key;
    LoginHistory    history;

    login_history = NULL;
    value = act_user_get_login_history (user);
    g_variant_get ((GVariant *) value, "a(xxa{sv})", &iter);
    while (g_variant_iter_loop (iter, "(xxa{sv})", &history.login_time, &history.logout_time, &iter2))
    {
        while (g_variant_iter_loop (iter2, "{sv}", &key, &variant))
        {
            if (g_strcmp0 (key, "type") == 0) 
            {
                history.type = g_variant_get_string (variant, NULL);
            }
        }

        if (login_history == NULL) 
        {
            login_history = g_array_new (FALSE, TRUE, sizeof (LoginHistory));
        }

        g_array_append_val (login_history, history);
    }

    return login_history;
}

static void set_sensitivity (LoginHistoryDialog *self)
{
    g_autoptr(GArray) login_history = NULL;
    LoginHistory      history;
    gboolean          sensitive = FALSE;

    login_history = get_login_history (self->Actuser);
    if (login_history != NULL) 
    {
        history = g_array_index (login_history, LoginHistory, 0);
        sensitive = g_date_time_to_unix (self->week) > history.login_time;
    }
    gtk_widget_set_sensitive (GTK_WIDGET (self->previous_button), sensitive);

    sensitive = (g_date_time_compare (self->current_week, self->week) == 1);
    gtk_widget_set_sensitive (GTK_WIDGET (self->next_button), sensitive);
}
static char *
cc_util_get_smart_date (GDateTime *date)
{
    g_autoptr(GDateTime) today = NULL;
    g_autoptr(GDateTime) local = NULL;
    GTimeSpan span;

    local = g_date_time_new_now_local (); 
    today = g_date_time_new_local (g_date_time_get_year (local),
                                   g_date_time_get_month (local),
                                   g_date_time_get_day_of_month (local),
                                   0, 0, 0); 
    span = g_date_time_difference (today, date);
    if (span <= 0)
    {   
        return g_strdup (_("Today"));
    }
    else if (span <= G_TIME_SPAN_DAY)
    {
        return g_strdup (_("Yesterday"));
    }
    else
    {
        if (g_date_time_get_year (date) == g_date_time_get_year (today))
        {
            return g_date_time_format (date, _("%b %e"));
        }
        else
        {
            return g_date_time_format (date, _("%b %e, %Y"));
        }
    }
}

static void add_record (LoginHistoryDialog *self, GDateTime *datetime, gchar *record_string, gint line)
{
    g_autofree gchar *date = NULL;
    g_autofree gchar *time = NULL;
    g_autofree gchar *str = NULL;
    GtkWidget *label, *row;

    date = cc_util_get_smart_date (datetime);
    time = g_date_time_format (datetime, ("%k:%M"));
    str = g_strdup_printf(("%s, %s"), date, time);

    row = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_show (row);
    gtk_box_set_homogeneous (GTK_BOX (row), TRUE);
    gtk_container_set_border_width (GTK_CONTAINER (row), 6);

    label = gtk_label_new (record_string);
    gtk_widget_show (label);
    gtk_widget_set_halign (label, GTK_ALIGN_START);
    gtk_box_pack_start (GTK_BOX (row), label, TRUE, TRUE, 0);

    label = gtk_label_new (str);
    gtk_widget_show (label);
    gtk_widget_set_halign (label, GTK_ALIGN_START);
    gtk_box_pack_start (GTK_BOX (row), label, TRUE, TRUE, 0);

    gtk_list_box_insert (self->history_box, row, line);
}

static void
show_week (LoginHistoryDialog *self)
{
    g_autoptr(GArray) login_history = NULL;
    g_autoptr(GDateTime) datetime = NULL;
    g_autoptr(GDateTime) temp = NULL;
    gint64 from, to;
    gint i, line;
    LoginHistory history;

    show_week_label (self);
    clear_history (self);
    set_sensitivity (self);

    login_history = get_login_history (self->Actuser);
    if (login_history == NULL) 
    {
        return;
    }

    from = g_date_time_to_unix (self->week);
    temp = g_date_time_add_weeks (self->week, 1);
    to = g_date_time_to_unix (temp);
    for (i = login_history->len - 1; i >= 0; i--)
    {
        history = g_array_index (login_history, LoginHistory, i);
        if (history.login_time < to) 
        {
            break;
        }
    }
    line = 0;
    for (;i >= 0; i--) 
    {
        history = g_array_index (login_history, LoginHistory, i);

        if (!g_str_has_prefix (history.type, ":") &&
            !g_str_has_prefix (history.type, "tty")) 
        {
            continue;
        }

        if (history.logout_time > 0 && history.logout_time < from) 
        {
            break;
        }
        if (history.logout_time > 0 && history.logout_time < to) 
        {
            datetime = g_date_time_new_from_unix_local (history.logout_time);
            add_record (self, datetime, _("Session Ended"), line);
            line++;
        }

        if (history.login_time >= from) 
        {
            datetime = g_date_time_new_from_unix_local (history.login_time);
            add_record (self, datetime, _("Session Started"), line);
            line++;
        }
    }
}
static void
previous_button_clicked_cb (GtkWidget *widget, gpointer data)
{
    LoginHistoryDialog *self = (LoginHistoryDialog *)data;
    g_autoptr(GDateTime) temp = NULL;

    temp = self->week;
    self->week = g_date_time_add_weeks (self->week, -1);

    show_week (self);
}

static void
next_button_clicked_cb (GtkWidget *widget, gpointer data)
{
    LoginHistoryDialog *self = (LoginHistoryDialog *)data;
    g_autoptr(GDateTime) temp = NULL;

    temp = self->week;
    self->week = g_date_time_add_weeks (self->week, 1);

    show_week (self);
}
static LoginHistoryDialog *login_history_dialog_new (ActUser *user)
{
    LoginHistoryDialog *self;
    g_autoptr(GDateTime) temp = NULL;
    g_autoptr(GDateTime) local = NULL;
    g_autofree gchar *title = NULL;

    g_return_val_if_fail (ACT_IS_USER (user), NULL);
    
    title = g_strdup_printf (_("%s — Account Activity"),
                              act_user_get_real_name (user));
    if(GetUseHeader() == 1)
    {
        self = g_object_new (TYPE_LOGIN_HISTORY,
                            "use-header-bar", 1,
                             NULL);
        gtk_header_bar_set_title (self->header_bar, title);
        self->is_header = 1;
    }  
    else
    {
        self = g_object_new (TYPE_LOGIN_HISTORY,
                            "use-header-bar", 0,
                             NULL);
        self->is_header = 0;
        gtk_window_set_title(GTK_WINDOW(self),title);
    }    

    self->Actuser = g_object_ref (user);

    local = g_date_time_new_now_local ();
    temp = g_date_time_new_local (g_date_time_get_year (local),
                                  g_date_time_get_month (local),
                                  g_date_time_get_day_of_month (local),
                                  0, 0, 0);
    self->week = g_date_time_add_days (temp, 1 - g_date_time_get_day_of_week (temp));
    self->current_week = g_date_time_ref (self->week);


    show_week (self);

    return self;
}

static void login_history_dialog_dispose (GObject *object)
{
    LoginHistoryDialog *self = LOGINHISTORY(object);

    g_clear_object (&self->Actuser);
    g_clear_pointer (&self->week, g_date_time_unref);
    g_clear_pointer (&self->current_week, g_date_time_unref);

    G_OBJECT_CLASS (login_history_dialog_parent_class)->dispose (object);
}

static void login_history_dialog_class_init (LoginHistoryDialogClass *class)
{
    GObjectClass   *gobject_class = G_OBJECT_CLASS (class);
    gobject_class->dispose = login_history_dialog_dispose; 
}

static void LoadHeader_bar(LoginHistoryDialog *dialog)
{
    GtkWidget *header;
    GtkWidget *box;
    GtkWidget *button;

    header = gtk_header_bar_new ();
    gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (header), TRUE);
    gtk_header_bar_set_has_subtitle (GTK_HEADER_BAR (header), TRUE);
    dialog->header_bar = GTK_HEADER_BAR(header);
    gtk_window_set_titlebar (GTK_WINDOW (dialog), header);
    

    box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_style_context_add_class (gtk_widget_get_style_context (box), "linked");
    
    button = gtk_button_new ();
    gtk_container_add (GTK_CONTAINER (button),
                       gtk_image_new_from_icon_name ("go-previous-symbolic",
                       GTK_ICON_SIZE_BUTTON));
    gtk_container_add (GTK_CONTAINER (box), button);
    dialog->previous_button = GTK_BUTTON(button); 

    button = gtk_button_new ();
    gtk_container_add (GTK_CONTAINER (button),
                       gtk_image_new_from_icon_name ("go-next-symbolic",
                       GTK_ICON_SIZE_BUTTON));
    gtk_container_add (GTK_CONTAINER (box), button);
    dialog->next_button = GTK_BUTTON(button);
    gtk_header_bar_pack_start (GTK_HEADER_BAR (header), box);

}    
void login_history_dialog_init (LoginHistoryDialog *dialog)
{
    GtkWidget *Scrolled;
    GtkWidget *listbox;
    GtkWidget *button;

    gtk_widget_set_size_request (GTK_WIDGET (dialog),580,350);
    Scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (Scrolled),
                                    GTK_POLICY_NEVER,
                                    GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (Scrolled),
                                         GTK_SHADOW_IN);

    listbox = gtk_list_box_new();
    gtk_container_add(GTK_CONTAINER(Scrolled),listbox);
    gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                        Scrolled,
                        TRUE, TRUE, 8);
    if(GetUseHeader() == 1)
    {
        LoadHeader_bar(dialog);
    }
    else
    {
        button = dialog_add_button_with_icon_name(GTK_DIALOG(dialog),
                                                 _("previous"),
                                                 "go-previous-symbolic",
                                                  GTK_RESPONSE_NONE);    
        dialog->previous_button = GTK_BUTTON(button); 
        button     = dialog_add_button_with_icon_name(GTK_DIALOG(dialog),
                                                     _("next"),
                                                     "go-next-symbolic",
                                                      GTK_RESPONSE_NONE);    
        dialog->next_button = GTK_BUTTON(button);
    }    
    dialog->history_box = GTK_LIST_BOX(listbox);

}
void ViewLoginHistory (GtkWidget *widget, gpointer data)
{
    LoginHistoryDialog *dialog;
    UserAdmin *ua = (UserAdmin *)data;

    dialog = login_history_dialog_new (ua->CurrentUser);
    g_signal_connect (dialog->previous_button, 
                     "clicked",
                      G_CALLBACK (previous_button_clicked_cb),
                      dialog);
    g_signal_connect (dialog->next_button, 
                     "clicked",
                      G_CALLBACK (next_button_clicked_cb),
                      dialog);
    gtk_widget_show_all(GTK_WIDGET(dialog));
//    gtk_dialog_run (GTK_DIALOG (dialog));
}    
