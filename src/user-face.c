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
#include "user-face.h"
#include "user-info.h"
#include "user-share.h"
#include "user-avatar.h"

struct _UserFacePrivate
{
    GtkWidget *button;
    GtkWidget *entry;

    char      *file_name;
    char      *real_name;
};

enum
{
    IMAGE_CHANGED,
    NAME_CHANGED,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };
G_DEFINE_TYPE_WITH_PRIVATE (UserFace, user_face, GTK_TYPE_GRID)

static void user_avatar_changed_cb (UserAvatar *avatar, UserFace *face)
{
    GtkWidget *image;
    char      *file_name;
    g_autoptr(GdkPixbuf) pb = NULL;
    GdkPixbuf *pb2;

    file_name = user_avatar_get_file_name (avatar);
    /*Update the home page picture*/
    pb = gdk_pixbuf_new_from_file (file_name, NULL);
    pb2 = gdk_pixbuf_scale_simple (pb, 96, 96, GDK_INTERP_BILINEAR);
    image = gtk_image_new_from_pixbuf(pb2);
    gtk_button_set_image(GTK_BUTTON(face->priv->button), image);

    face->priv->file_name = g_strdup (file_name);
    g_signal_emit (face, signals[IMAGE_CHANGED], 0);

}

/******************************************************************************
* Function:              RealNameValidCheck 
*        
* Explain: Check the validity of the input name
*        
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  09/05/2018
******************************************************************************/
static gboolean RealNameValidCheck (const gchar *name)
{
    gboolean is_empty = TRUE;
    const gchar *c;
    for (c = name; *c; c++)
    {
        gunichar unichar;
        unichar = g_utf8_get_char_validated (c, -1);
        if (unichar == (gunichar) -1 || unichar == (gunichar) -2)
            break;
        if (!g_unichar_isspace (unichar))
        {
            is_empty = FALSE;
            break;
        }
    }

    return !is_empty;
}

/******************************************************************************
* Function:              ModifyName 
*        
* Explain: Modify the user's real name,Use the Entry key to end
*        
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  09/05/2018
******************************************************************************/
static void ModifyName (GtkEntry *entry, gpointer data)
{
    UserFace    *face = USER_FACE (data);
    const gchar *NewName;

    NewName  = gtk_entry_get_text (entry);

    if(RealNameValidCheck (NewName))
    {
        face->priv->real_name = g_strdup (NewName);
        g_signal_emit (face, signals[NAME_CHANGED], 0);
    }
    else
    {
        MessageReport(_("Change Name"),_("Please enter a valid character"),ERROR);
    }
}

static void
user_face_dispose (GObject *object)
{
    UserFace *face = USER_FACE (object);

    if (face->priv->file_name != NULL)
    {
        g_free (face->priv->file_name);
        face->priv->file_name = NULL;
    }
    if (face->priv->real_name != NULL)
    {
        g_free (face->priv->real_name);
        face->priv->real_name = NULL;
    }
    G_OBJECT_CLASS (user_face_parent_class)->dispose (object);
}

static void
user_face_class_init (UserFaceClass *klass)
{
    GObjectClass   *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->dispose = user_face_dispose;

    signals [IMAGE_CHANGED] =
         g_signal_new ("image-changed",
                       G_TYPE_FROM_CLASS (klass),
                       G_SIGNAL_RUN_LAST,
                       0,
                       NULL, NULL,
                       g_cclosure_marshal_VOID__VOID,
                       G_TYPE_NONE, 0);

    signals [NAME_CHANGED] =
         g_signal_new ("name-changed",
                       G_TYPE_FROM_CLASS (klass),
                       G_SIGNAL_RUN_LAST,
                       0,
                       NULL, NULL,
                       g_cclosure_marshal_VOID__VOID,
                       G_TYPE_NONE, 0);
}

static void
user_face_init (UserFace *face)
{
    face->priv = user_face_get_instance_private (face);
    gtk_grid_set_row_spacing (GTK_GRID (face), 10);
    gtk_grid_set_column_spacing (GTK_GRID (face), 10);

    /* avatar button */
    face->priv->button = gtk_button_new ();
    gtk_grid_attach (GTK_GRID(face), face->priv->button, 0, 0, 8, 8);

    /*set real name*/
    face->priv->entry = gtk_entry_new ();
    gtk_widget_set_tooltip_text (face->priv->entry, _("Use Enter Key to Save Modifications"));
    gtk_entry_set_max_length (GTK_ENTRY (face->priv->entry), 48);
    gtk_grid_attach (GTK_GRID (face), face->priv->entry, 8, 4, 1, 1);
}

void user_face_fill (UserFace *face, ActUser *user)
{
    UserAvatar *avatar;
    GtkWidget  *image;
    GError     *error = NULL;
    g_autoptr(GdkPixbuf) pb = NULL;
    GdkPixbuf  *pb2;

    /*set user icon 96 *96 */
    pb = gdk_pixbuf_new_from_file (GetUserIcon (user), &error);
    if (pb == NULL)
    {
        mate_uesr_admin_log ("Warning","mate-user-admin user icon %s", error->message);
        g_error_free (error);
    }
    pb2 = gdk_pixbuf_scale_simple (pb, 96, 96, GDK_INTERP_BILINEAR);
    image = gtk_image_new_from_pixbuf (pb2);
    gtk_button_set_image (GTK_BUTTON(face->priv->button), image);

    gtk_entry_set_text (GTK_ENTRY (face->priv->entry), GetRealName (user));
    g_signal_connect (face->priv->entry,
                     "activate",
                      G_CALLBACK (ModifyName),
                      face);

    avatar = user_avatar_new (face->priv->button);
    g_signal_connect (avatar,
                     "avatar-changed",
                      G_CALLBACK (user_avatar_changed_cb),
                      face);

    g_signal_connect (face->priv->button,
                     "clicked",
                      G_CALLBACK (user_avatar_popup_button_clicked),
                      avatar);

    g_signal_connect (face->priv->button,
                     "button-press-event",
                      G_CALLBACK (user_avatar_popup_button_pressed),
                      avatar);
}

char *user_face_get_image_name (UserFace *face)
{
    return face->priv->file_name;
}

char *user_face_get_real_name (UserFace *face)
{
    return face->priv->real_name;
}

void user_face_update (UserFace  *face,
                      const char *image_name,
                      const char *real_name)
{
    GtkWidget *image;
    g_autoptr(GdkPixbuf) pb = NULL;
    GdkPixbuf *pb2;

    pb = gdk_pixbuf_new_from_file (image_name, NULL);
    pb2 = gdk_pixbuf_scale_simple (pb, 96, 96, GDK_INTERP_BILINEAR);
    image = gtk_image_new_from_pixbuf (pb2);

    gtk_button_set_image (GTK_BUTTON (face->priv->button),
                          image);

    gtk_entry_set_text (GTK_ENTRY (face->priv->entry),
                        real_name);
}

UserFace *
user_face_new (void)
{
    UserFace   *face;

    face = g_object_new (USER_TYPE_FACE, NULL);

    return face;
}
