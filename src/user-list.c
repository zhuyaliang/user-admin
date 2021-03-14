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
#include "user-admin.h"
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
    gtk_widget_set_halign(box, GTK_ALIGN_START);
    gtk_widget_set_valign(box, GTK_ALIGN_CENTER);
    gtk_container_add (GTK_CONTAINER (row), box);

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start (GTK_BOX(box), hbox, TRUE, TRUE, 0);

    row->priv->user_image = gtk_image_new();
    gtk_box_pack_start(GTK_BOX(hbox),row->priv->user_image ,TRUE, TRUE, 6);

    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request (vbox, 110, -1);
    gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 12);

    row->priv->real_name = create_user_list_row_label ();
    gtk_box_pack_start(GTK_BOX(vbox),row->priv->real_name ,TRUE, TRUE, 1);

    row->priv->user_name = create_user_list_row_label ();
    gtk_box_pack_start (GTK_BOX (vbox), row->priv->user_name ,TRUE, TRUE, 1);

    gtk_box_pack_start (GTK_BOX (box), gtk_separator_new (GTK_ORIENTATION_HORIZONTAL) ,TRUE, TRUE, 3);

}

static void
user_list_row_class_init (UserListRowClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    widget_class->destroy = user_list_row_destroy;
}

GtkWidget *
user_list_row_new (ActUser *Actuser)
{
    UserListRow *row;

    row = g_object_new (USER_LIST_TYPE_ROW, NULL);
    g_set_object (&row->priv->user, Actuser);
    user_list_row_set_data (row);

    return GTK_WIDGET (row);
}

static void remove_all_row (GtkWidget *row, gpointer data)
{
    gtk_container_remove (GTK_CONTAINER (data), row);
    g_object_unref (row);
}    

void user_list_box_update (GtkWidget *list_box, GSList *user_list)
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
}

static void SwitchUser (GtkListBox    *list_box,
                      	GtkListBoxRow *Row,
                        gpointer       data)
{

    UserAdmin *ua = (UserAdmin *)data;
    UserListRow *row = USER_LIST_ROW(Row);
    ua->CurrentUser  = row->priv->user;
    ua->CurrentImage = row->priv->user_image;
    ua->CurrentName  = row->priv->real_name;
    UpdateInterface(row->priv->user, ua);
}

GtkWidget *user_list_get_row_image_label (GtkWidget *list_box, int index)
{
    GtkListBoxRow *row;
    UserListRow   *user_row;

    row = gtk_list_box_get_row_at_index (GTK_LIST_BOX (list_box), index);
    user_row = USER_LIST_ROW (row);
    
    return user_row->priv->user_image;
}

GtkWidget *user_list_get_row_name_label (GtkWidget *list_box, int index)
{
    GtkListBoxRow *row;
    UserListRow   *user_row;

    row = gtk_list_box_get_row_at_index (GTK_LIST_BOX (list_box), index);
    user_row = USER_LIST_ROW (row);
    
    return user_row->priv->real_name;
}

/******************************************************************************
* Function:              DisplayUserList      
*        
* Explain: Display user list on the left side,icon and name 
*        
* Input:         
*        
* Output:  
*        
* Author:  zhuyaliang  09/05/2018
******************************************************************************/
GtkWidget *create_user_list_box (UserAdmin *ua)
{
    GtkWidget *list_box;

    list_box = gtk_list_box_new ();

    g_signal_connect (list_box,
                     "row-activated",
                      G_CALLBACK (SwitchUser),
                      ua);

    return list_box;
}

static void QuitApp(GtkWidget *widget, gpointer data)
{
    UserAdmin *ua = (UserAdmin *)data;
    g_slist_free_full(ua->UsersList,g_object_unref); 
    gtk_main_quit();
}
/******************************************************************************
* Function:              AddRemoveUser 
*        
* Explain: Adding new users or remove users 
*        
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  09/05/2018
******************************************************************************/
void AddRemoveUser(GtkWidget *Vbox,UserAdmin *ua)
{
    GtkWidget *ButtonClose;
    GtkWidget *LableSpace;
    GtkWidget *table;
    table = gtk_grid_new();
    gtk_grid_set_column_homogeneous(GTK_GRID(table),TRUE);
    gtk_box_pack_start(GTK_BOX(Vbox),table, TRUE, TRUE,0);

    LableSpace = gtk_label_new(NULL);
    gtk_grid_attach(GTK_GRID(table) , LableSpace , 0 , 0 , 4 , 1);
    ua->ButtonAdd    = SetButtonIcon(_("Add User"),"list-add");
    ua->ButtonRemove = SetButtonIcon(_("Remove User"),"list-remove");
    ButtonClose      = SetButtonIcon(_("Close"),"window-close");

    gtk_grid_attach(GTK_GRID(table) , ua->ButtonRemove , 0 , 1 , 1 , 1);
    gtk_grid_attach(GTK_GRID(table) , ua->ButtonAdd ,    1 , 1 , 1 , 1);
    if(GetUseHeader() == 0)
    {
        gtk_grid_attach(GTK_GRID(table) ,ua->ButtonLock, 3 , 1 , 1 , 1);
    }    
    gtk_grid_attach(GTK_GRID(table) , ButtonClose ,      4 , 1 , 1 , 1);
    g_signal_connect (ua->ButtonRemove, 
                     "clicked",
                      G_CALLBACK (RemoveUser),
                      ua);
    g_signal_connect (ua->ButtonAdd, 
                     "clicked",
                      G_CALLBACK (AddNewUser),
                      ua);
    g_signal_connect (ButtonClose, 
                     "clicked",
                      G_CALLBACK (QuitApp),
                      ua);
    
    gtk_grid_set_row_spacing(GTK_GRID(table), 10);
    gtk_grid_set_column_spacing(GTK_GRID(table), 10);

}  
