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

#include "user-list.h"
#include "user-share.h"
#include "user-info.h"

struct _UserListRowPrivate
{
    ActUser      *user;
    GtkWidget    *user_image;
    GtkWidget    *user_name;
    GtkWidget    *real_name;
};

G_DEFINE_TYPE_WITH_PRIVATE (UserListRow, user_list_row, GTK_TYPE_LIST_BOX_ROW)

void
user_list_row_set_data (UserListRow *row)
{
    GdkPixbuf   *face;
    const char  *user_image;
    const char  *real_name;
    const char  *user_name;

    user_image = GetUserIcon (row->priv->user);
    real_name  = GetRealName (row->priv->user),
    user_name  = GetUserName (row->priv->user),

    face = SetUserFaceSize (user_image, 50);
    gtk_image_set_from_pixbuf (GTK_IMAGE (row->priv->user_image),face);

    SetLableFontType (row->priv->real_name, "black", 14, real_name, TRUE);
    SetLableFontType (row->priv->user_name, "black", 11, user_name, FALSE);
}

static void
user_list_row_destroy (GtkWidget *widget)
{
    UserListRow *row = USER_LIST_ROW (widget);
    g_clear_object (&row->priv->user);
}

static GtkWidget *create_user_list_row_label (void)
{
    GtkWidget *label;

    label = gtk_label_new (NULL);
    gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
    gtk_label_set_max_width_chars (GTK_LABEL (label), 10);

    return label;
}

static void
user_list_row_init (UserListRow *row)
{
    GtkWidget *box;
    GtkWidget *hbox;
    GtkWidget *vbox;

    row->priv = user_list_row_get_instance_private (row);
    box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_size_request (box, 190, -1);
    gtk_widget_set_halign (box, GTK_ALIGN_START);
    gtk_widget_set_valign (box, GTK_ALIGN_CENTER);
    gtk_container_add (GTK_CONTAINER (row), box);

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start (GTK_BOX(box), hbox, TRUE, TRUE, 0);

    row->priv->user_image = gtk_image_new();
    gtk_box_pack_start (GTK_BOX (hbox), row->priv->user_image, TRUE, TRUE, 6);

    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request (vbox, 110, -1);
    gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 12);

    row->priv->real_name = create_user_list_row_label ();
    gtk_box_pack_start (GTK_BOX (vbox), row->priv->real_name, TRUE, TRUE, 1);

    row->priv->user_name = create_user_list_row_label ();
    gtk_box_pack_start (GTK_BOX (vbox), row->priv->user_name, TRUE, TRUE, 1);

    gtk_box_pack_start (GTK_BOX (box), gtk_separator_new (GTK_ORIENTATION_HORIZONTAL), TRUE, TRUE, 3);

}

static void
user_list_row_class_init (UserListRowClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    widget_class->destroy = user_list_row_destroy;
}

GtkWidget *
user_list_row_new (ActUser *user)
{
    UserListRow *row;

    row = g_object_new (USER_LIST_TYPE_ROW, NULL);
    g_set_object (&row->priv->user, user);
    user_list_row_set_data (row);

    return GTK_WIDGET (row);
}

static void remove_all_row (GtkWidget *row, gpointer data)
{
    gtk_container_remove (GTK_CONTAINER (data), row);
    g_object_unref (row);
}

static void user_list_set_select_user (GtkWidget *list_box, gint index)
{
    GtkListBoxRow *row;

    row = gtk_list_box_get_row_at_index (GTK_LIST_BOX (list_box), index);
    if (row == NULL)
    {
        g_warning ("The selected user does not exist \r\n");
        return;
    }
    gtk_list_box_select_row (GTK_LIST_BOX (list_box), row);
}

void user_list_box_update (GtkWidget *list_box, GSList *user_list, int index)
{
    GtkWidget *row;
    GSList    *l;
    int        i = 0;
    ActUser   *user;

    gtk_container_foreach (GTK_CONTAINER (list_box), remove_all_row, list_box);
    for (l = user_list ; l; l = l->next)
    {
        user  = l->data;
        row = user_list_row_new (user);
        gtk_list_box_row_set_activatable (GTK_LIST_BOX_ROW (row), TRUE);
        gtk_list_box_insert (GTK_LIST_BOX (list_box), g_object_ref (row), i);
        i++;
    }
    user_list_set_select_user (list_box, index);
}

GtkWidget *user_list_row_get_image_label (UserListRow *row)
{
    return row->priv->user_image;
}

GtkWidget *user_list_row_get_name_label (UserListRow *row)
{
    return row->priv->real_name;
}

ActUser *user_list_row_get_user (UserListRow *row)
{
    return row->priv->user;
}
