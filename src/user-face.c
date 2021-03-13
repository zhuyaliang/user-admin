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
#include "user-crop.h"
#include "user.h"
#include "user-face.h"
#include "user-share.h"
#include "user-info.h"
#include "user-list.h"
#include "user-avatar.h"

static void user_avatar_changed_cb (UserAvatar *avatar, UserAdmin *ua)
{
    GtkWidget *image;
    char      *file_name;
    g_autoptr(GdkPixbuf) pb = NULL;
    GdkPixbuf *pb2;
    GdkPixbuf *face;

    file_name = user_avatar_get_file_name (avatar);
    /*Update the home page picture*/
    pb = gdk_pixbuf_new_from_file (file_name, NULL);
    pb2 = gdk_pixbuf_scale_simple (pb, 96, 96, GDK_INTERP_BILINEAR);
    image = gtk_image_new_from_pixbuf(pb2);
    gtk_button_set_image(GTK_BUTTON(ua->ButtonFace), image);

    act_user_set_icon_file (ua->CurrentUser, file_name);
    face = SetUserFaceSize (file_name, 50);
    gtk_image_set_from_pixbuf(GTK_IMAGE(ua->CurrentImage), face);
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
static void ModifyName (GtkEntry *entry,gpointer data)
{
    UserAdmin   *ua = (UserAdmin *) data;
    const gchar *NewName;

    NewName  = gtk_entry_get_text (entry);

    if(RealNameValidCheck(NewName))
    {
        act_user_set_real_name (ua->CurrentUser,NewName);
        SetLableFontType(ua->CurrentName,"black",14,NewName,TRUE);
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
    UserAvatar *avatar;
    g_autoptr(GdkPixbuf) pb = NULL;
    GdkPixbuf *pb2;
    GtkWidget *EntryName;
    GtkWidget *table;
    GtkWidget *fixed;
    GError    *error = NULL;

    fixed = gtk_fixed_new();
    gtk_box_pack_start(GTK_BOX(Hbox),fixed,TRUE,TRUE,0);

    table = gtk_grid_new();
    gtk_fixed_put(GTK_FIXED(fixed),table, 0, 0);

    /*set user icon 96 *96 */
    pb = gdk_pixbuf_new_from_file(GetUserIcon(ua->CurrentUser),&error);
    if(pb == NULL)
    {
        mate_uesr_admin_log("Warning","mate-user-admin user icon %s",error->message);
        g_error_free(error);
    }
    pb2 = gdk_pixbuf_scale_simple (pb,96,96, GDK_INTERP_BILINEAR);
    image = gtk_image_new_from_pixbuf(pb2);

    ua->ButtonFace = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(ua->ButtonFace),image);
    gtk_grid_attach(GTK_GRID(table) , ua->ButtonFace , 0 , 0 , 8 , 8);
    avatar = user_avatar_new (ua->ButtonFace);
    g_signal_connect (avatar,
                     "avatar-changed",
                      G_CALLBACK (user_avatar_changed_cb),
                      ua);

    g_signal_connect (ua->ButtonFace,
                     "clicked",
                      G_CALLBACK (user_avatar_popup_button_clicked),
                      avatar);

    g_signal_connect (ua->ButtonFace,
                     "button-press-event",
                      G_CALLBACK (user_avatar_popup_button_pressed),
                      avatar);

    /*set real name*/
    EntryName = gtk_entry_new();
    gtk_widget_set_tooltip_text(EntryName,_("Use Enter Key to Save Modifications"));
    gtk_entry_set_max_length(GTK_ENTRY(EntryName),48);
    gtk_entry_set_text(GTK_ENTRY(EntryName),GetRealName(ua->CurrentUser));
    ua->EntryName = EntryName;
    gtk_grid_attach(GTK_GRID(table) ,EntryName , 8 , 4 , 1 , 1);
    g_signal_connect (EntryName, "activate",G_CALLBACK (ModifyName),ua);
    gtk_grid_set_row_spacing(GTK_GRID(table), 10);
    gtk_grid_set_column_spacing(GTK_GRID(table), 10);

}
