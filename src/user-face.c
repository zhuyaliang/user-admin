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
#include <sys/statfs.h>
#include <dirent.h>
#include <sys/stat.h>
#include <gio/gio.h>
#include <gio/gunixoutputstream.h>
#include <glib/gstdio.h>
#include "user.h"
#include "user-face.h"
#include "user-share.h"
#include "user-crop.h"
static char gcPicBuf[PICMAX][50];   //照片
/******************************************************************************
* Function:              UpdataFace 
*        
* Explain: Update the user's head image
*        
* Input:   @nCount :Photo ordinal number
*        
* Output: 
*        
* Author:  zhuyaliang  09/05/2018
******************************************************************************/
static void UpdataFace(int nCount,const char *LocalFileName,UserAdmin *ua)
{
    GtkWidget *image;
    GdkPixbuf *face;

    GdkPixbuf *pb, *pb2;
    char BasePath[100] = { 0 };
    if(nCount >= 0)
    {    
        sprintf(BasePath, FACEDIR"/%s", gcPicBuf[nCount]);
    }
    else
    {
        memcpy(BasePath,LocalFileName,strlen(LocalFileName)); 
    }    
    /*Update the home page picture*/
    pb = gdk_pixbuf_new_from_file(BasePath,NULL);
    pb2 = gdk_pixbuf_scale_simple (pb,96,96, GDK_INTERP_BILINEAR);
    image = gtk_image_new_from_pixbuf(pb2);
    gtk_button_set_image(GTK_BUTTON(ua->ButtonFace),image);
    
    /*Update the left list picture*/
    face = SetUserFaceSize (BasePath, 50);

    gtk_list_store_set(ua->ListSTore,&ua->ul[gnCurrentUserIndex].Iter,
                      COL_USER_FACE, face,
                      LIST_COLOR,"black",
                      LIST_FRONT,1985,-1);
    act_user_set_icon_file (ua->ul[gnCurrentUserIndex].User,BasePath);
    memset(ua->ul[gnCurrentUserIndex].UserIcon,
          '\0',
          strlen(ua->ul[gnCurrentUserIndex].UserIcon));
    /**/
    memcpy(ua->ul[gnCurrentUserIndex].UserIcon,BasePath,strlen(BasePath));

}
/******************************************************************************
* Function:              Unbind 
*        
* Explain: Close the window and dissolve the signal
*        
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  09/05/2018
******************************************************************************/
static void Unbind(gpointer data)
{
    UserAdmin *ua = (UserAdmin *) data;
    if(ua->IconWindow != NULL)
        gtk_widget_destroy(ua->IconWindow);

    /*mouse click event*/
    if(ua->MouseId != 0)
    {    
        g_signal_handler_disconnect(ua->MainWindow,ua->MouseId);
    ua->MouseId = 0;
    }
    if(ua->KeyId != 0)
    {        
        g_signal_handler_disconnect(ua->MainWindow,ua->KeyId);
        ua->KeyId = 0;
    }    
    ua->IconWindow = NULL; 

}        
/******************************************************************************
* Function:              face_widget_activated 
*        
* Explain: Click the picture signal event
*        
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  09/05/2018
******************************************************************************/
static void face_widget_activated (GtkFlowBox *flowbox,
                                   GtkFlowBoxChild *child,
                                   gpointer data)
{
    UserAdmin *ua = (UserAdmin *) data;
    int Count;

   /* The lower mark of the selected photo*/
    Count = gtk_flow_box_child_get_index(child);
    /* updata user image */
    UpdataFace(Count,NULL,ua);
    Unbind(ua);
}

/******************************************************************************
* Function:              NotifyEvents 
*        
* Explain: When users do not operate, click the blank to close the window
*        
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  09/05/2018
******************************************************************************/
static gboolean NotifyEvents(GtkWidget *widget,
                             GdkEventButton *event, 
                             gpointer data)
{

    UserAdmin *ua = (UserAdmin *) data;

    if(ua->IconWindow  != NULL )
    {
        Unbind(ua);
    }        
    return TRUE;
}        
/******************************************************************************
* Function:              GetFaceFile 
*        
* Explain: Zoom the picture to the right size
*        
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  09/05/2018
******************************************************************************/
static GtkWidget *GetFaceFile (const char *FileName)
{
    char Path[300] = { 0 };
    GtkWidget *image;
    GdkPixbuf *src;
    GdkPixbuf *dst;
    sprintf(Path, "%s/%s",FACEDIR, FileName);
    src = gdk_pixbuf_new_from_file(Path, NULL);
    dst = gdk_pixbuf_scale_simple(src,70,70,GDK_INTERP_BILINEAR);
    image = gtk_image_new_from_pixbuf(dst);

    gtk_widget_show_all (image);

    return image;
}

/******************************************************************************
* Function:              GetFaceList 
*        
* Explain: Get all the available pictures
*        
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  09/05/2018
******************************************************************************/
static int GetDirFace(GtkWidget *flowbox)
{
    DIR *dp;
    struct stat statbuf;
    struct dirent *entry;
    int cnt = 0;

    if((dp = opendir(FACEDIR)) == NULL)
	    return -2;

    while((entry = readdir(dp)) != NULL)
    {
        lstat(entry->d_name,&statbuf);
        if(S_ISDIR(statbuf.st_mode))
        {
            if(strcmp(".",entry->d_name) == 0 ||strcmp("..",entry->d_name) == 0)
            continue;
        }
        /*Save the name of the photo to the array*/
        memcpy(gcPicBuf[cnt],entry->d_name,strlen(entry->d_name));
        if(cnt++ > PICMAX)
            return 0;
        gtk_container_add (GTK_CONTAINER (flowbox), GetFaceFile(entry->d_name));
    }
    closedir(dp);
    return 0;
}
static int WindowOpenFlag;
/******************************************************************************
* Function:              ThumbnailPreview
*        
* Explain: Local picture thumbnails
*        
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  14/08/2018
******************************************************************************/
static void ThumbnailPreview (GtkFileChooser  *chooser,gpointer data)
{
    char *uri;
    GdkPixbuf *pixbuf = NULL;
    char *mime_type = NULL;
    GFile *file;
    GFileInfo *file_info;
    GtkWidget *preview;
    MateDesktopThumbnailFactory *thumbnail;

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
static void SaveFaceFile(UserAdmin *ua,GdkPixbuf *pb)
{
    gchar *FilePath;
    gint fd;
    GOutputStream *stream;
    GError *error =NULL;

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
    UpdataFace(-1,FilePath,ua);

     /* if we ever make the dbus call async, the g_remove call needs
      * to wait for its completion
      */
     g_remove (FilePath);
     g_free (FilePath);
}    
static void CropDialogResponse (GtkWidget *dialog,
                                gint      response_id,
                                UserAdmin *ua)
{
    GdkPixbuf *pb, *pb2;

    if (response_id != GTK_RESPONSE_ACCEPT) {
            ua->CropArea = NULL;
            gtk_widget_destroy (dialog);
            return;
    }
    pb = user_crop_area_get_picture (USER_CROP_AREA (ua->CropArea));
    pb2 = gdk_pixbuf_scale_simple (pb, 96, 96, GDK_INTERP_BILINEAR);

    SaveFaceFile (ua, pb2);

    g_object_unref (pb2);
    g_object_unref (pb);

    ua->CropArea = NULL;
    gtk_widget_destroy (dialog);

}
/******************************************************************************
* Function:              FaceDialogCrop
*        
* Explain: Tailoring local photos
*        
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  13/11/2018
******************************************************************************/
static void FaceDialogCrop (UserAdmin *ua,GdkPixbuf *pixbuf)
{
        GtkWidget *dialog;
        dialog = gtk_dialog_new_with_buttons ("",
                                              GTK_WINDOW (gtk_widget_get_toplevel (ua->IconWindow)),
                                              GTK_DIALOG_USE_HEADER_BAR,
                                              _("_Cancel"),
                                              GTK_RESPONSE_CANCEL,
                                              _("Select"),
                                              GTK_RESPONSE_ACCEPT,
                                              NULL);

     //   gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);

        gtk_window_set_icon_name (GTK_WINDOW (dialog), "system-users");

        g_signal_connect (G_OBJECT (dialog), "response",
                          G_CALLBACK (CropDialogResponse), ua);

        ua->CropArea           = user_crop_area_new (); 
        user_crop_area_set_min_size (USER_CROP_AREA (ua->CropArea), 50, 50);
        user_crop_area_set_constrain_aspect (USER_CROP_AREA (ua->CropArea), TRUE);
        user_crop_area_set_picture (USER_CROP_AREA (ua->CropArea), pixbuf);
        gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                            ua->CropArea,
                            TRUE, TRUE, 8);

        gtk_window_set_default_size (GTK_WINDOW (dialog), 400, 300);

        gtk_widget_show_all (dialog);
}



/******************************************************************************
* Function:              PictureChooser
*        
* Explain: Local picture select
*        
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  14/08/2018
******************************************************************************/
static void PictureChooser(GtkDialog *chooser,
                           gint response,
                           gpointer data)
{
    UserAdmin *ua = (UserAdmin *) data;
    char *FileName;
    GError *error = NULL;
    GdkPixbuf *pixbuf,*cutpixbuf;

    if (response != GTK_RESPONSE_ACCEPT) {
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
    FaceDialogCrop(ua, cutpixbuf);
    g_object_unref (cutpixbuf);
    Unbind(ua);
}

static void LoadLocalPicture(GtkWidget *button, gpointer data)
{
    UserAdmin *ua = (UserAdmin *) data;
    GtkWidget *chooser;
    const char *folder;
    GtkWidget *preview;
    GtkFileFilter *filter;

    chooser = gtk_file_chooser_dialog_new (_("Browse for more pictures"),
                                           GTK_WINDOW(ua->IconWindow),
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
    g_signal_connect_after (chooser, "selection-changed",
                            G_CALLBACK (ThumbnailPreview), NULL);

    folder = g_get_user_special_dir (G_USER_DIRECTORY_PICTURES);
    if (folder)
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (chooser),
                                                 folder);

    filter = gtk_file_filter_new ();
    gtk_file_filter_add_pixbuf_formats (filter);
    gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (chooser), filter);

    g_signal_connect (chooser, "response",
                      G_CALLBACK (PictureChooser), ua);

    gtk_window_present (GTK_WINDOW (chooser));
}    
/******************************************************************************
* Function:              GetFaceList 
*        
* Explain: Get user head image,This window can only open one at the same time.
*        
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  09/05/2018
******************************************************************************/
static void GetFaceList(GtkWidget *button, gpointer data)
{
    UserAdmin *ua = (UserAdmin *) data;
    GtkWidget *window;
    GtkWidget *Vbox;
    GtkWidget *Frame;
    GtkWidget *Scrolled;
    GtkWidget *Flowbox;
    GtkWidget *LocalButton;

    int MouseId;
    int KeyId;
    int nRet;
  
    /*WindowOpenFlag Whether to open a window sign*/
    if(WindowOpenFlag == 1)
        Unbind(ua);        
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_decorated(GTK_WINDOW(window),FALSE);  //Setting a frameless frame
    gtk_window_set_default_size (GTK_WINDOW (window), 450, 300);
    gtk_widget_realize(window);
    gtk_window_set_position(GTK_WINDOW(window),GTK_WIN_POS_MOUSE);
    gtk_container_set_border_width(GTK_CONTAINER(window),20);
    
    Vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,8);
    gtk_widget_set_size_request(Vbox,300,-1);
    gtk_container_add(GTK_CONTAINER(window),Vbox);

    Frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(Frame),GTK_SHADOW_IN);
    gtk_box_pack_start(GTK_BOX(Vbox),Frame,TRUE,TRUE,0);
    
    Scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (Scrolled),
                                    GTK_POLICY_NEVER, 
                                    GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (Scrolled),
                                         GTK_SHADOW_IN);
    gtk_container_add(GTK_CONTAINER(Frame),Scrolled);

    Flowbox =  gtk_flow_box_new();
    gtk_widget_set_valign (Flowbox, GTK_ALIGN_START);
    gtk_flow_box_set_max_children_per_line (GTK_FLOW_BOX (Flowbox), 30);
    gtk_flow_box_set_selection_mode (GTK_FLOW_BOX (Flowbox), GTK_SELECTION_NONE);
    gtk_flow_box_set_activate_on_single_click(GTK_FLOW_BOX (Flowbox),TRUE);
    gtk_container_add (GTK_CONTAINER (Scrolled), Flowbox);
    
    LocalButton = gtk_button_new_with_label(_("Local Picture"));
    gtk_widget_set_halign (LocalButton, GTK_ALIGN_CENTER);
    gtk_widget_set_valign (LocalButton, GTK_ALIGN_CENTER);
    gtk_box_pack_start (GTK_BOX (Vbox), LocalButton, FALSE, FALSE, 0);

    g_signal_connect(LocalButton,
                     "clicked",
                     G_CALLBACK(LoadLocalPicture),
                     ua);

    /* Get all the available pictures */
    nRet = GetDirFace(Flowbox);
    if(nRet < 0)
    {
        MessageReport(_("Avatar list"),
                      _("There is no address to store photos"),
                      WARING);
    }   
    ua->IconWindow = window;
    gtk_flow_box_selected_foreach (GTK_FLOW_BOX (Flowbox),
                                face_widget_activated,
                                ua);

    gtk_widget_add_events(ua->MainWindow,
                          GDK_BUTTON_PRESS_MASK |
                          GDK_KEY_PRESS_MASK);

    MouseId = g_signal_connect(G_OBJECT(ua->MainWindow),
                    "button-press-event", 
                    G_CALLBACK(NotifyEvents), 
                    ua);

    ua->MouseId = MouseId;
    KeyId = g_signal_connect(G_OBJECT(ua->MainWindow),
                    "key-press-event", 
                    G_CALLBACK(NotifyEvents), 
                    ua);
    ua->KeyId = KeyId;
    WindowOpenFlag = 1;
    g_signal_connect (Flowbox, "child-activated",
                      G_CALLBACK (face_widget_activated),
                      ua);

    gtk_widget_show_all (window);

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
static void ModifyName (GtkEntry *entry,gpointer  data)
{
    UserAdmin *ua = (UserAdmin *) data;
    const gchar *NewName;

    NewName = gtk_entry_get_text (entry);
    if(RealNameValidCheck(NewName))
    {        
        gtk_list_store_set(ua->ListSTore,&ua->ul[gnCurrentUserIndex].Iter,
                          LIST_TEXT, NewName,
                          LIST_COLOR,"black",
                          LIST_FRONT,1985,-1);
        memset(ua->ul[gnCurrentUserIndex].RealName,
              '\0',
            strlen(ua->ul[gnCurrentUserIndex].RealName));
        memcpy(ua->ul[gnCurrentUserIndex].RealName,NewName,strlen(NewName));
        act_user_set_real_name (ua->ul[gnCurrentUserIndex].User,NewName);
    }
    else
    {
        MessageReport(_("Change Name"),_("Please enter a valid character"),ERROR); 
    }        
}
/******************************************************************************
* Function:              DisplayUserSetFace  
*        
* Explain: Display the user's head image and name
*        
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  09/05/2018
******************************************************************************/
void DisplayUserSetFace(GtkWidget *Hbox,UserAdmin *ua)
{
    GtkWidget *image;
    GdkPixbuf *pb, *pb2;
    GtkWidget *EntryName;
    GtkWidget *table;
    GtkWidget *fixed;
    GtkWidget *ButtonFace;
     
    fixed = gtk_fixed_new();
    gtk_box_pack_start(GTK_BOX(Hbox),fixed,TRUE,TRUE,0);  
   
    table = gtk_grid_new();
    gtk_fixed_put(GTK_FIXED(fixed),table, 0, 0);
   
    /*set user icon 96 *96 */
    pb = gdk_pixbuf_new_from_file(ua->ul[0].UserIcon,NULL);
    pb2 = gdk_pixbuf_scale_simple (pb,96,96, GDK_INTERP_BILINEAR);
    image = gtk_image_new_from_pixbuf(pb2);

    ButtonFace = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(ButtonFace),image);
    ua->ButtonFace = ButtonFace;
    gtk_grid_attach(GTK_GRID(table) , ButtonFace , 0 , 0 , 8 , 8);
    g_signal_connect(G_OBJECT(ButtonFace), 
                    "clicked",
                     G_CALLBACK(GetFaceList),
                     ua);

    /*set real name*/
    EntryName = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(EntryName),48);
    gtk_entry_set_text(GTK_ENTRY(EntryName),ua->ul[0].UserName);
    ua->EntryName = EntryName;
    gtk_grid_attach(GTK_GRID(table) ,EntryName , 8 , 4 , 1 , 1);
    g_signal_connect (EntryName, "activate",G_CALLBACK (ModifyName),ua);
    gtk_grid_set_row_spacing(GTK_GRID(table), 10);
    gtk_grid_set_column_spacing(GTK_GRID(table), 10);

}     
