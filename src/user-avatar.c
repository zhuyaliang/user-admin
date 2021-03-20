/*************************************************************************
  File Name: user-avatar.c
  
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
                                               
  Created Time: 2021年03月13日 星期五 20时32分34秒
 ************************************************************************/

#define MATE_DESKTOP_USE_UNSTABLE_API
#include <libmate-desktop/mate-desktop-thumbnail.h>
#include <gio/gio.h>
#include <gio/gunixoutputstream.h>
#include <glib/gstdio.h>
#include "user-crop.h"
#include "config.h"
#include "user-avatar.h"
#include "user-share.h"

enum
{
    AVATAR_CHANGED,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };
struct _UserAvatarPrivate
{
    GtkWidget *button;
    GtkWidget *crop_area;
    GtkWidget *flowbox;
    char      *new_avatar_path;

    MateDesktopThumbnailFactory *thumbnail;
    GListStore *faces;
};

G_DEFINE_TYPE_WITH_PRIVATE (UserAvatar, user_avatar, GTK_TYPE_POPOVER)

static void thumbnail_preview (GtkFileChooser  *chooser,
                              MateDesktopThumbnailFactory *thumbnail)
{
    char      *uri;
    GdkPixbuf *pixbuf = NULL;
    char      *mime_type = NULL;
    GFile     *file;
    GFileInfo *file_info;
    GtkWidget *preview;

    uri = gtk_file_chooser_get_uri (chooser);
    thumbnail = mate_desktop_thumbnail_factory_new (MATE_DESKTOP_THUMBNAIL_SIZE_NORMAL);
    if (uri)
    {
        preview = gtk_file_chooser_get_preview_widget (chooser);
        file = g_file_new_for_uri (uri);
        file_info = g_file_query_info (file,
                                      "standard::*",
                                       G_FILE_QUERY_INFO_NONE,
                                       NULL, NULL);
        g_object_unref (file);
        if (file_info != NULL &&
            g_file_info_get_file_type (file_info) != G_FILE_TYPE_DIRECTORY)
        {
            mime_type = g_strdup (g_file_info_get_content_type (file_info));
            g_object_unref (file_info);
        }

        if (mime_type)
        {
            pixbuf = mate_desktop_thumbnail_factory_generate_thumbnail (thumbnail,
                                                                        uri,
                                                                        mime_type);
            g_free (mime_type);
        }
        gtk_dialog_set_response_sensitive (GTK_DIALOG (chooser),
                                           GTK_RESPONSE_ACCEPT,
                                           (pixbuf != NULL));

        if (pixbuf != NULL)
        {
            gtk_image_set_from_pixbuf (GTK_IMAGE (preview), pixbuf);
            g_object_unref (pixbuf);
        }
        else
        {
            gtk_image_set_from_icon_name (GTK_IMAGE (preview),
                                         "dialog-question",
                                          GTK_ICON_SIZE_DIALOG);
        }

        g_free (uri);
    }

    gtk_file_chooser_set_preview_widget_active (chooser, TRUE);
}

static void save_new_user_icon (UserAvatar *avatar, GdkPixbuf *pb)
{
    gchar         *FilePath;
    gint           fd;
    GOutputStream *stream;
    GError        *error =NULL;

    FilePath = g_build_filename (g_get_tmp_dir (), "mate-user-admin-icon-XXXXXX", NULL);
    fd = g_mkstemp (FilePath);

    if (fd == -1)
    {
        MessageReport(_("Create Crop File"),
                      _("failed to create temporary file for image data"),
                       ERROR);
        g_free (FilePath);
        return;
    }

    stream = g_unix_output_stream_new (fd, TRUE);

    if (!gdk_pixbuf_save_to_stream (pb, stream, "png", NULL, &error, NULL))
    {
        MessageReport(_("Create Crop File"),
                         error->message,
                         ERROR);
        g_error_free (error);
        g_object_unref (stream);
        return;
    }

    g_object_unref (stream);
    avatar->priv->new_avatar_path = g_strdup (FilePath);
    g_signal_emit (avatar, signals[AVATAR_CHANGED], 0);

    g_remove (FilePath);
    g_free (FilePath);
}

static void crop_dialog_response_cb (GtkWidget *dialog,
                                    gint        response_id,
                                    UserAvatar *avatar)
{
    GdkPixbuf *pb, *pb2;

    if (response_id != GTK_RESPONSE_ACCEPT)
    {
        avatar->priv->crop_area = NULL;
        gtk_widget_destroy (dialog);
        return;
    }
    pb = user_crop_area_get_picture (USER_CROP_AREA (avatar->priv->crop_area));
    pb2 = gdk_pixbuf_scale_simple (pb, 512, 512, GDK_INTERP_BILINEAR);

    save_new_user_icon (avatar, pb2);

    g_object_unref (pb2);
    g_object_unref (pb);

    avatar->priv->crop_area = NULL;
    gtk_widget_destroy (dialog);
}

static void face_dialog_crop (UserAvatar *avatar, GdkPixbuf *pixbuf)
{
    GtkWidget *dialog;
    dialog = gtk_dialog_new_with_buttons ("",
                                          GTK_WINDOW (gtk_widget_get_toplevel (avatar->priv->button)),
                                          GTK_DIALOG_USE_HEADER_BAR,
                                          _("_Cancel"),
                                          GTK_RESPONSE_CANCEL,
                                          _("Select"),
                                          GTK_RESPONSE_ACCEPT,
                                          NULL);

    gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);

    gtk_window_set_icon_name (GTK_WINDOW (dialog), "system-users");

    g_signal_connect (G_OBJECT (dialog),
                     "response",
                      G_CALLBACK (crop_dialog_response_cb),
                      avatar);

    avatar->priv->crop_area = user_crop_area_new ();
    user_crop_area_set_min_size (USER_CROP_AREA (avatar->priv->crop_area), 50, 50);
    user_crop_area_set_constrain_aspect (USER_CROP_AREA (avatar->priv->crop_area), TRUE);
    user_crop_area_set_picture (USER_CROP_AREA (avatar->priv->crop_area), pixbuf);
    gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                        avatar->priv->crop_area,
                        TRUE, TRUE, 8);

    gtk_window_set_default_size (GTK_WINDOW (dialog), 400, 300);

    gtk_widget_show_all (dialog);
}

static void local_picture_chooser (GtkDialog *chooser,
                                   gint       response,
                                   gpointer   data)
{
    UserAvatar *avatar = USER_AVATAR (data);
    char       *FileName;
    GError     *error = NULL;
    GdkPixbuf  *pixbuf;
    GdkPixbuf  *cutpixbuf;

    if (response != GTK_RESPONSE_ACCEPT) 
    {
        gtk_widget_destroy (GTK_WIDGET (chooser));
        return;
    }

    FileName = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));

    pixbuf = gdk_pixbuf_new_from_file (FileName, &error);
    if (pixbuf == NULL)
    {
        MessageReport(_("Load Local Picture"),
                       error->message,
                       WARING);
        g_error_free (error);
    }

    g_free (FileName);
    cutpixbuf = gdk_pixbuf_apply_embedded_orientation (pixbuf);
    g_object_unref (pixbuf);
    gtk_widget_destroy (GTK_WIDGET (chooser));

    face_dialog_crop(avatar, cutpixbuf);
    g_object_unref (cutpixbuf);
}

static void user_select_local_picture (GtkWidget *button, gpointer data)
{
    UserAvatar    *avatar = USER_AVATAR (data);
    GtkWidget     *chooser;
    const char    *folder;
    GtkWidget     *preview;
    GtkFileFilter *filter;

    chooser = gtk_file_chooser_dialog_new (_("Browse for more pictures"),
                                           GTK_WINDOW (gtk_widget_get_toplevel (avatar->priv->button)),
                                           GTK_FILE_CHOOSER_ACTION_OPEN,
                                           _("_Cancel"), GTK_RESPONSE_CANCEL,
                                           _("_Open"), GTK_RESPONSE_ACCEPT,
                                           NULL);

    gtk_window_set_modal (GTK_WINDOW (chooser), TRUE);

    preview = gtk_image_new ();
    gtk_widget_set_size_request (preview, 128, -1);
    gtk_file_chooser_set_preview_widget (GTK_FILE_CHOOSER (chooser), preview);
    gtk_file_chooser_set_use_preview_label (GTK_FILE_CHOOSER (chooser), FALSE);
    gtk_widget_show (preview);
    g_signal_connect_after (chooser,
                           "selection-changed",
                            G_CALLBACK (thumbnail_preview),
                            avatar->priv->thumbnail);

    folder = g_get_user_special_dir (G_USER_DIRECTORY_PICTURES);
    if (folder)
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (chooser),
                                                 folder);

    filter = gtk_file_filter_new ();
    gtk_file_filter_add_pixbuf_formats (filter);
    gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (chooser), filter);

    g_signal_connect (chooser,
                     "response",
                      G_CALLBACK (local_picture_chooser),
                      avatar);

    gtk_window_present (GTK_WINDOW (chooser));
}

static GtkWidget *create_flow_box (void)
{
    GtkWidget *flowbox;

    flowbox =  gtk_flow_box_new();
    gtk_widget_set_valign (flowbox, GTK_ALIGN_START);
    gtk_flow_box_set_max_children_per_line (GTK_FLOW_BOX (flowbox), 5);
    gtk_container_set_border_width (GTK_CONTAINER (flowbox), 10);
    gtk_flow_box_set_selection_mode (GTK_FLOW_BOX (flowbox), GTK_SELECTION_NONE);
    gtk_flow_box_set_homogeneous (GTK_FLOW_BOX (flowbox), TRUE);
    gtk_flow_box_set_activate_on_single_click (GTK_FLOW_BOX (flowbox), TRUE);
    gtk_flow_box_set_column_spacing (GTK_FLOW_BOX (flowbox), 10);

    return flowbox;
}

static void 
user_avatar_fill (UserAvatar *avatar)
{
    GtkWidget *box;
    GtkWidget *scrolled;
    GtkWidget *button;

    box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_size_request (box, 500, 300);
    gtk_container_add (GTK_CONTAINER (avatar), box);

    scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_box_pack_start(GTK_BOX(box), scrolled, TRUE, TRUE, 0);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                    GTK_POLICY_NEVER,
                                    GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled),
                                         GTK_SHADOW_IN);

    avatar->priv->flowbox = create_flow_box ();
    gtk_container_add (GTK_CONTAINER (scrolled), avatar->priv->flowbox);

    button = gtk_button_new_with_label (_("Local Picture"));
    gtk_widget_set_halign (button, GTK_ALIGN_CENTER);
    gtk_widget_set_valign (button, GTK_ALIGN_CENTER);
    gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);

    g_signal_connect(button,
                    "clicked",
                     G_CALLBACK(user_select_local_picture),
                     avatar);
}

static GObject *
user_avatar_constructor (GType                  type,
                         guint                  n_construct_properties,
                         GObjectConstructParam *construct_properties)
{
    GObject      *obj;
    UserAvatar   *avatar;

    obj = G_OBJECT_CLASS (user_avatar_parent_class)->constructor (type,
                                      n_construct_properties,
                                      construct_properties);

    avatar = USER_AVATAR (obj);

    user_avatar_fill (avatar);

    return obj;
}

static void
user_avatar_dispose (GObject *object)
{
    UserAvatar *avatar = USER_AVATAR (object);

    g_clear_object (&avatar->priv->thumbnail);
    if (avatar->priv->new_avatar_path != NULL)
    {
        g_free (avatar->priv->new_avatar_path);
    }
    G_OBJECT_CLASS (user_avatar_parent_class)->dispose (object);
}

static void
user_avatar_class_init (UserAvatarClass *klass)
{
    GObjectClass   *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->constructor = user_avatar_constructor;
    gobject_class->dispose = user_avatar_dispose;
    signals [AVATAR_CHANGED] =
         g_signal_new ("avatar-changed",
                       G_TYPE_FROM_CLASS (klass),
                       G_SIGNAL_RUN_LAST,
                       0,
                       NULL, NULL,
                       g_cclosure_marshal_VOID__VOID,
                       G_TYPE_NONE, 0);
}

static void
user_avatar_init (UserAvatar *avatar)
{

    avatar->priv = user_avatar_get_instance_private (avatar);

    avatar->priv->thumbnail = mate_desktop_thumbnail_factory_new (MATE_DESKTOP_THUMBNAIL_SIZE_NORMAL);
}

static GtkWidget *
create_face_widget (gpointer item,
                    gpointer user_data)
{
    g_autofree gchar *image_path = NULL;
    g_autoptr(GdkPixbuf) source_pixbuf = NULL;
    g_autoptr(GdkPixbuf) pixbuf = NULL;
    GtkWidget *image;

    image_path = g_file_get_path (G_FILE (item));
    source_pixbuf = gdk_pixbuf_new_from_file_at_size (image_path,
                                                      80,
                                                      80,
                                                      NULL);
    if (source_pixbuf == NULL)
        return NULL;

    pixbuf = GetRoundPixbuf (source_pixbuf);
    image = gtk_image_new_from_pixbuf (pixbuf);
    gtk_image_set_pixel_size (GTK_IMAGE (image), 80);
    gtk_widget_show (image);

    g_object_set_data_full (G_OBJECT (image),
                           "filename", g_steal_pointer (&image_path), g_free);

    return image;
}

static void face_widget_activated (GtkFlowBox      *flowbox,
                                   GtkFlowBoxChild *child,
                                   gpointer         data)
{
    UserAvatar  *avatar = USER_AVATAR (data);
    const gchar *filename;
    GtkWidget   *image;

    image = gtk_bin_get_child (GTK_BIN (child));
    filename = g_object_get_data (G_OBJECT (image), "filename");

    avatar->priv->new_avatar_path = NULL;
    avatar->priv->new_avatar_path = g_strdup (filename);

    g_signal_emit (avatar, signals[AVATAR_CHANGED], 0);
    gtk_popover_popdown (GTK_POPOVER (avatar));
}

static gboolean add_list_faces_file (GListStore *faces)
{
    DIR    *dp;
    struct  stat statbuf;
    struct  dirent *entry;
    GFile  *file;
    char   *file_path;

    if((dp = opendir(FACEDIR)) == NULL)
        return FALSE;

    while((entry = readdir(dp)) != NULL)
    {
        lstat(entry->d_name,&statbuf);
        if(S_ISDIR(statbuf.st_mode))
        {
            if(strcmp(".",entry->d_name) == 0 ||strcmp("..",entry->d_name) == 0)
            continue;
        }
        file_path = g_strdup_printf ("%s%s", FACEDIR, entry->d_name);
        file = g_file_new_for_path (file_path);
        g_list_store_append (faces, file);
        g_free (file_path);
    }
    closedir(dp);
    return TRUE;
}

static void
create_avatar_menus (UserAvatar *avatar)
{
    gboolean ret;
    avatar->priv->faces = g_list_store_new (G_TYPE_FILE);
    gtk_flow_box_bind_model (GTK_FLOW_BOX (avatar->priv->flowbox),
                             G_LIST_MODEL (avatar->priv->faces),
                             create_face_widget,
                             avatar,
                             NULL);

    gtk_flow_box_selected_foreach (GTK_FLOW_BOX (avatar->priv->flowbox),
                                   face_widget_activated,
                                   avatar);
    g_signal_connect (avatar->priv->flowbox,
                     "child-activated",
                      G_CALLBACK (face_widget_activated),
                      avatar);
    ret = add_list_faces_file (avatar->priv->faces);
    if(!ret)
    {
        mate_uesr_admin_log("Warning","mate-user-admin There is no address to store photos");
        MessageReport(_("Avatar list"),
                      _("There is no address to store photos"),
                      WARING);
    }
}

void
user_avatar_popup_button_clicked (GtkWidget  *button,
                                  UserAvatar *avatar)
{
    gtk_popover_popup (GTK_POPOVER (avatar));
    gtk_widget_show_all (GTK_WIDGET (avatar));
}

gboolean
user_avatar_popup_button_pressed (GtkWidget      *button,
                                  GdkEventButton *event,
                                  UserAvatar     *avatar)
{
    if (event->button == 1)
    {
        if (!gtk_widget_get_visible (GTK_WIDGET (avatar)))
        {
            gtk_popover_popup (GTK_POPOVER (avatar));
            gtk_widget_show_all (GTK_WIDGET (avatar));
        }
        else
        {
            gtk_popover_popdown (GTK_POPOVER (avatar));
        }

        return TRUE;
    }

    return FALSE;
}

char *
user_avatar_get_file_name (UserAvatar *avatar)
{
    if (avatar->priv->new_avatar_path != NULL)
        return avatar->priv->new_avatar_path;
    return DEFAULT; 
}

UserAvatar *
user_avatar_new (GtkWidget *button)
{
    UserAvatar *avatar;

    avatar = g_object_new (USER_TYPE_AVATAR,
                         "relative-to", button,
                          NULL);

    avatar->priv->button = button;
    create_avatar_menus (avatar);

    return avatar;
}
