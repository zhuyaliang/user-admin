/*************************************************************************
  File Name: user-window.c
  
  Copyright (C) 2020  zhuyaliang https://github.com/zhuyaliang/
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
                                      
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
                                               
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
                                               
  Created Time: 2021年03月12日 星期五 17时16分18秒
 ************************************************************************/

#include "user-window.h"
#include "user-info.h"
#include "user-base.h"
#include "user-admin.h"
#include "user-share.h"
#include "user-face.h"
#include "user-list.h"
#include "user-group-window.h"

#define  USER_ADMIN_PERMISSION     "org.mate.user.admin.administration"
#define  APPICON                   "user-admin.png"

struct _UserWindowPrivate
{
    ActUserManager *manager;
    ActUser        *user;
    GPermission    *permission;
    GSList         *user_list;
    GtkWidget      *list_box;
    UserFace       *face;
    UserBase       *base;
    GtkWidget      *list_image;
    GtkWidget      *list_label;
    GtkWidget      *button_remove;
    GtkWidget      *button_lock;
    GtkWidget      *button_add;
    GtkWidget      *popover;

    gint            update_user_id;
};

G_DEFINE_TYPE_WITH_PRIVATE (UserWindow, user_window, GTK_TYPE_WINDOW)

static void user_window_update (UserWindow *win,
                                ActUser    *user)
{
    gboolean   is_authorized;
    gboolean   self_selected;

    is_authorized = g_permission_get_allowed (G_PERMISSION (win->priv->permission));
    self_selected = act_user_get_uid (user) == geteuid ();

    /*Switching icon*/
    user_face_update (win->priv->face, GetUserIcon (user), GetRealName (user));
    user_base_update_user_info (win->priv->base, user);

    mate_uesr_admin_log("Info","mate-user-admin Current user name %s",GetRealName(user));

    if(self_selected == 0)
    {
        gtk_widget_set_sensitive(GTK_WIDGET (win->priv->face),is_authorized);
        user_base_set_private_sensitive (win->priv->base, is_authorized);
    }
    else if(is_authorized == 0 && self_selected == 1)
    {
        gtk_widget_set_sensitive(GTK_WIDGET (win->priv->face),self_selected);
        user_base_set_private_sensitive (win->priv->base, self_selected);
    }
}

static GtkWidget *set_unlock_button_tips (GtkWidget *button_lock)
{
    GtkWidget *popover;
    GtkWidget *box;
    GtkWidget *label;
    GtkWidget *image;

    popover = gtk_popover_new (button_lock);
    box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 10);
    image = gtk_image_new_from_icon_name ("system-lock-screen-symbolic", GTK_ICON_SIZE_LARGE_TOOLBAR);
    gtk_container_add(GTK_CONTAINER(box), image);

    label = gtk_label_new (NULL);
    SetLableFontType(label, "black", 11, _("Some settings must be unlocked before they can be changed"), FALSE);
    gtk_container_add(GTK_CONTAINER(box), label);

    gtk_popover_set_position (GTK_POPOVER (popover), GTK_POS_LEFT);
    gtk_container_add (GTK_CONTAINER (popover), box);
    gtk_container_set_border_width (GTK_CONTAINER (popover), 6);
    gtk_widget_show_all (popover);
    gtk_popover_popup (GTK_POPOVER(popover));

    return popover;
}

static void update_permission (UserWindow *win)
{
    gboolean  is_authorized;
    gboolean  self_selected;

    is_authorized = g_permission_get_allowed (G_PERMISSION (win->priv->permission));
    self_selected = act_user_get_uid (win->priv->user) == geteuid ();

    gtk_widget_set_sensitive (win->priv->button_add, is_authorized);
    gtk_widget_set_sensitive (win->priv->button_remove, is_authorized);
    gtk_widget_set_sensitive (GTK_WIDGET (win->priv->face), is_authorized);
    user_base_set_public_sensitive (win->priv->base, is_authorized);
    user_base_set_private_sensitive (win->priv->base, is_authorized);
    gtk_widget_set_visible (win->priv->popover,!is_authorized);

    if (is_authorized == 0 && self_selected == 1)
    {
        gtk_widget_set_sensitive (GTK_WIDGET (win->priv->face), self_selected);
        user_base_set_private_sensitive (win->priv->base, self_selected);
    }
}

static void on_permission_changed (GPermission *permission,
                                   GParamSpec  *pspec,
                                   gpointer     data)
{
    UserWindow *userwin = USER_WINDOW (data);

    update_permission(userwin);
}

static void user_window_set_permission (UserWindow *win)
{
    GError    *error = NULL;

    win->priv->permission = polkit_permission_new_sync (USER_ADMIN_PERMISSION, NULL, NULL, &error);
    if (win->priv->permission != NULL)
    {
        win->priv->button_lock = gtk_lock_button_new (win->priv->permission);
        win->priv->popover = set_unlock_button_tips (win->priv->button_lock);

        gtk_lock_button_set_permission (GTK_LOCK_BUTTON (win->priv->button_lock), win->priv->permission);
        gtk_widget_grab_focus (win->priv->button_lock);

        g_signal_connect (win->priv->permission,
                        "notify",
                         G_CALLBACK (on_permission_changed),
                         win);
    }
    else
    {
        mate_uesr_admin_log ("Warning","Cannot create '%s' permission: %s", USER_ADMIN_PERMISSION, error->message);
        g_error_free (error);
    }
}
static GtkWidget *create_user_list_box (GtkWidget *box)
{
    GtkWidget *list_box;
    GtkWidget *scrolled;

    scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_box_pack_start (GTK_BOX(box), scrolled, TRUE, TRUE, 0);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                    GTK_POLICY_NEVER,
                                    GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled),
                                         GTK_SHADOW_IN);
    list_box = gtk_list_box_new ();
    gtk_container_add (GTK_CONTAINER (scrolled), list_box);

    return list_box;
}

static void user_window_list_select_user (GtkListBox    *list_box,
                                          GtkListBoxRow *row,
                                          UserWindow    *win)
{
    win->priv->user = user_list_row_get_user (USER_LIST_ROW (row));
    win->priv->list_image = user_list_row_get_image_label (USER_LIST_ROW (row));
    win->priv->list_label = user_list_row_get_name_label (USER_LIST_ROW (row));

    user_base_set_user (win->priv->base, win->priv->user);

    user_window_update (win, win->priv->user);
}

static void user_image_changed_cb (UserFace *face, UserWindow *win)
{
    char      *file_name;
    GdkPixbuf *pb;

    file_name = user_face_get_image_name (face);
    act_user_set_icon_file (win->priv->user, file_name);

    pb = SetUserFaceSize (file_name, 50);
    gtk_image_set_from_pixbuf (GTK_IMAGE (win->priv->list_image), pb);
}

static void show_main_window (UserGroupWindow *win, GtkWidget *window)
{
    gtk_widget_show (window);
}

static void user_group_manager_cb (UserBase *base, UserWindow *win)
{
    UserGroupWindow *group_window;
    const char      *name;

    name = GetUserName (win->priv->user);

    gtk_widget_hide (GTK_WIDGET (win));
    group_window = user_group_window_new (name, win->priv->user_list);
    g_signal_connect (group_window,
                      "window-closed",
                      G_CALLBACK (show_main_window),
                      win);

    gtk_widget_show_all (GTK_WIDGET (group_window));
}

static void user_name_changed_cb (UserFace *face, UserWindow *win)
{
    char *name;

    name = user_face_get_real_name (face);
    act_user_set_real_name (win->priv->user, name);
    SetLableFontType (win->priv->list_label, "black", 14, name, TRUE);
}

static void remove_user_cb (GtkWidget *widget, UserWindow *win)
{
    ActUser *user = win->priv->user;

    RemoveUser (user);
}

static void QuitApp (GtkWidget *widget, UserWindow *win)
{
    gtk_widget_destroy (GTK_WIDGET (win));
}

static void
user_window_destroy (GtkWidget *data)
{
    UserWindow *userwin = USER_WINDOW (data);

    g_slist_free_full (userwin->priv->user_list, g_object_unref);
    close_log_file ();
    gtk_widget_destroy (GTK_WIDGET (userwin->priv->face));
    gtk_widget_destroy (GTK_WIDGET (userwin->priv->base));
    if (userwin->priv->update_user_id > 0)
    {
        g_source_remove (userwin->priv->update_user_id);
    }
    g_clear_object (&userwin->priv->manager);
    gtk_main_quit ();
}

static void create_button_box (GtkWidget *box, GtkWidget *button_lock, UserWindow *win)
{
    GtkWidget *button_close;
    GtkWidget *lable_space;
    GtkWidget *table;

    table = gtk_grid_new ();
    gtk_grid_set_column_homogeneous (GTK_GRID (table), TRUE);
    gtk_box_pack_start (GTK_BOX (box), table, TRUE, TRUE, 0);

    lable_space = gtk_label_new (NULL);
    gtk_grid_attach (GTK_GRID (table) , lable_space , 0, 0, 4, 1);

    win->priv->button_add = SetButtonIcon (_("Add User"), "list-add");
    gtk_grid_attach (GTK_GRID (table), win->priv->button_add, 1, 1, 1, 1);

    win->priv->button_remove = SetButtonIcon (_("Remove User"), "list-remove");
    gtk_grid_attach (GTK_GRID (table), win->priv->button_remove ,0 ,1 ,1 ,1);

    button_close = SetButtonIcon (_("Close"), "window-close");
    gtk_grid_attach (GTK_GRID (table) , button_close ,4, 1, 1, 1);

    gtk_grid_attach (GTK_GRID (table), button_lock, 3, 1, 1, 1);

    g_signal_connect (win->priv->button_remove,
                     "clicked",
                      G_CALLBACK (remove_user_cb),
                      win);

    g_signal_connect (win->priv->button_add,
                     "clicked",
                      G_CALLBACK (AddNewUser),
                      NULL);

    g_signal_connect (button_close,
                     "clicked",
                      G_CALLBACK (QuitApp),
                      win);

    gtk_grid_set_row_spacing (GTK_GRID (table), 10);
    gtk_grid_set_column_spacing (GTK_GRID (table), 10);
}

static void 
user_window_fill (UserWindow *userwin)
{
    GtkWidget *fixed;
    GtkWidget *box;
    GtkWidget *hbox;
    GtkWidget *hbox1;
    GtkWidget *hbox2;

    fixed = gtk_fixed_new ();
    gtk_container_add (GTK_CONTAINER (userwin), fixed);

    box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 10);
    gtk_fixed_put (GTK_FIXED (fixed), box, 0, 0);

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start (GTK_BOX(box), hbox, FALSE, FALSE, 0);

    hbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 10);
    gtk_box_pack_start (GTK_BOX (hbox), hbox2, FALSE, FALSE, 10);
    gtk_widget_set_size_request (hbox2, 180, -1);

    userwin->priv->list_box = create_user_list_box (hbox2);
    g_signal_connect (userwin->priv->list_box,
                     "row-activated",
                      G_CALLBACK (user_window_list_select_user),
                      userwin);

    hbox1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 10);
    gtk_box_pack_start (GTK_BOX (hbox), hbox1, TRUE, TRUE, 10); 

    userwin->priv->face = user_face_new ();
    g_signal_connect (userwin->priv->face,
                     "image-changed",
                      G_CALLBACK (user_image_changed_cb),
                      userwin);

    g_signal_connect (userwin->priv->face,
                     "name-changed",
                      G_CALLBACK (user_name_changed_cb),
                      userwin);

    gtk_box_pack_start (GTK_BOX (hbox1), GTK_WIDGET (userwin->priv->face), TRUE, TRUE, 0);

    userwin->priv->base = user_base_new ();
    g_signal_connect (userwin->priv->base,
                     "group-viewed",
                      G_CALLBACK (user_group_manager_cb),
                      userwin);

    gtk_box_pack_start (GTK_BOX (hbox1), GTK_WIDGET (userwin->priv->base), TRUE, TRUE, 0);

    /*Adding new users or remove users*/
    create_button_box (box, userwin->priv->button_lock, userwin);
}

static GObject *
user_window_constructor (GType                  type,
                         guint                  n_construct_properties,
                         GObjectConstructParam *construct_properties)
{
    GObject      *obj;
    UserWindow   *userwin;

    obj = G_OBJECT_CLASS (user_window_parent_class)->constructor (type,
                                      n_construct_properties,
                                      construct_properties);

    userwin = USER_WINDOW (obj);

    user_window_fill (userwin);

    return obj;
}

static void
user_window_class_init (UserWindowClass *klass)
{
    GObjectClass   *gobject_class = G_OBJECT_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    widget_class->destroy = user_window_destroy;
    gobject_class->constructor = user_window_constructor;
}

static void
user_window_init (UserWindow *userwin)
{
    GtkWindow  *window;

    userwin->priv = user_window_get_instance_private (userwin);

    window = GTK_WINDOW (userwin);

    gtk_container_set_border_width (GTK_CONTAINER (window), 10);

    user_window_set_permission (userwin);
}

static void user_window_set_list_data (UserWindow *win, int index)
{
    GtkListBoxRow  *row;

    row = gtk_list_box_get_row_at_index (GTK_LIST_BOX (win->priv->list_box), index);

    win->priv->user  = user_list_row_get_user (USER_LIST_ROW (row));
    win->priv->list_label = user_list_row_get_name_label (USER_LIST_ROW (row));
    win->priv->list_image = user_list_row_get_image_label (USER_LIST_ROW (row));
}

UserWindow *
user_window_new (ActUserManager *manager)
{
    UserWindow *userwin;
    g_autoptr(GdkPixbuf) icon = NULL;


    userwin = g_object_new (USER_TYPE_WINDOW,
                           "type", GTK_WINDOW_TOPLEVEL,
                           "window-position", GTK_WIN_POS_CENTER,
                           "title", _("Mate User Manager"),
                            NULL);

    icon = gdk_pixbuf_new_from_file(ICONDIR APPICON, NULL);
    if(icon)
    {
        gtk_window_set_icon (GTK_WINDOW (userwin), icon);
    }

    userwin->priv->user_list = get_user_info_list (manager);
    user_list_box_update (userwin->priv->list_box, userwin->priv->user_list, 0);
    user_window_set_list_data (userwin, 0);
    user_face_fill (userwin->priv->face, userwin->priv->user);
    user_base_set_user (userwin->priv->base, userwin->priv->user);
    user_window_update (userwin, userwin->priv->user);

    update_permission (userwin);
    userwin->priv->manager = g_object_ref (manager);

    return userwin;
}

void user_window_remove_user_cb (ActUserManager *um,
                                 ActUser        *user,
                                 UserWindow     *win)
{
    win->priv->user_list = g_slist_remove (win->priv->user_list, user);

    user_list_box_update (win->priv->list_box, win->priv->user_list, 0);

    user_window_set_list_data (win, 0);
    user_window_update (win, win->priv->user);

    gtk_widget_show_all (win->priv->list_box);
}

static gboolean update_new_user_info (UserWindow *win)
{
    user_window_update (win, win->priv->user);
    
    g_source_remove (win->priv->update_user_id);
    win->priv->update_user_id = 0;

    return FALSE;
}

void user_window_add_user_cb (ActUserManager *um,
                              ActUser        *user,
                              UserWindow     *win)
{
    int index;

    if (act_user_get_uid (user) == 0)
        return;

    win->priv->user_list  = g_slist_append (win->priv->user_list, user);

    index = g_slist_length (win->priv->user_list) - 1;
    user_list_box_update (win->priv->list_box, win->priv->user_list, index);

    user_window_set_list_data (win, index);

    win->priv->update_user_id = g_timeout_add (800, (GSourceFunc) update_new_user_info, win);

    gtk_widget_show_all (win->priv->list_box);
}
