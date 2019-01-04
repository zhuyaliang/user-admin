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

static GtkListStore *store;
static gboolean SelectUser = TRUE;
/******************************************************************************
* Function:              RefreshUserList 
*        
* Explain: Refresh User List Call after adding or deleting user 
*        
* Input:  @UserList User List on the left       
*         @List     Store a list of user classes
* Output: 
*        
* Author:  zhuyaliang  09/05/2018
******************************************************************************/
void RefreshUserList(GtkWidget *UserList,GSList *List)
{
    UserInfo *user;
    GSList *l;
    int i = 0;
    SelectUser = FALSE;
    if(store != NULL)
    {
        gtk_list_store_clear(store);
    }    
    for (l = List ; l; l = l->next)
    {
        user = (UserInfo *)l->data;
        UserListAppend(UserList,
                       GetUserIcon(user->ActUser),                     
                       GetRealName(user->ActUser),
                       "black",
                       i,
                       &user->Iter);
        i++;
    }
    gnCurrentUserIndex = 0;
    SelectUser = TRUE;
}    
/******************************************************************************
* Function:              on_changed     
*        
* Explain: Switching users to display user information
*        
* Input:         
*        
* Output:  
*        
* Author:  zhuyaliang  09/05/2018
******************************************************************************/
static void  on_changed(GtkWidget *widget,  gpointer data)
{
    UserAdmin *ua = (UserAdmin *) data;
    GtkTreeIter iter;
    UserInfo *user;
    gint count = 0;
    if(SelectUser == TRUE)
    {    
        if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(widget), &ua->Model, &iter))
        {
            /*Get the user line number*/
            gtk_tree_model_get (ua->Model, &iter,
                                INT_COLUMN, &count,
                                -1);
            gnCurrentUserIndex = count;
            user = GetIndexUser(ua,count);
            if(user == NULL)
            {
                g_error("No such user !!!\r\n");
            }    
            /*update display*/
            UpdateInterface(user->ActUser,ua);
        }
    }    
}

static GtkTreeModel  *ListModelCreate(UserAdmin *ua)
{   
    store = gtk_list_store_new(N_COLUMNS,
                               GDK_TYPE_PIXBUF,
                               G_TYPE_INT,
                               G_TYPE_STRING,
                               G_TYPE_STRING,
                               G_TYPE_INT);
    ua->ListSTore = store;
    return GTK_TREE_MODEL(store);
}

/******************************************************************************
* Function:              ListViewInit 
*        
* Explain:  Initialization list.@renderer_icon user icon;
*                               @renderer_text user real name; 
*                               include color and front;
* Input:         
*        
* Output:  
*        
* Author:  zhuyaliang  09/05/2018
******************************************************************************/
static void ListViewInit(GtkWidget *list)
{
    GtkCellRenderer *renderer_icon,*renderer_text;
    GtkTreeViewColumn *column;
    column=gtk_tree_view_column_new ();

    gtk_tree_view_column_set_title(column,_("User List"));
    
    renderer_icon = gtk_cell_renderer_pixbuf_new();   //user icon
    renderer_text = gtk_cell_renderer_text_new();     //user real name text

    gtk_tree_view_column_pack_start (column, renderer_icon, FALSE);
    gtk_tree_view_column_set_attributes (column, 
                                         renderer_icon,
                                         "pixbuf", 
                                         COL_USER_FACE,
                                         NULL);

    gtk_tree_view_column_pack_start(column,renderer_text,FALSE);
    gtk_tree_view_column_add_attribute(column,
                                       renderer_text,
                                       "text",
                                       LIST_TEXT);

    gtk_tree_view_column_add_attribute(column,
                                       renderer_text,
                                       "foreground",
                                       LIST_COLOR);

    gtk_tree_view_column_add_attribute(column,
                                       renderer_text,
                                       "weight",
                                       LIST_FRONT);
    gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);
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
void DisplayUserList(GtkWidget *Hbox,UserAdmin *ua)
{
    GtkWidget *table;
    GtkTreeSelection *selection;
    GtkWidget *UserList;
    GtkWidget *Scrolled;
    GtkTreeModel *model;
    
    Scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_box_pack_start(GTK_BOX(Hbox),Scrolled, TRUE, TRUE,0);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (Scrolled),
                                    GTK_POLICY_NEVER,
                                    GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (Scrolled),
                                         GTK_SHADOW_IN);
    
    table = gtk_grid_new();
    gtk_grid_set_column_homogeneous(GTK_GRID(table),TRUE);
    gtk_container_add (GTK_CONTAINER (Scrolled), table);
   /* create user list */ 
    UserList= gtk_tree_view_new_with_model(ListModelCreate(ua)); 

    /* init user list */
    ListViewInit(UserList);
    ua->UserList = UserList;

    /* Add the user to the lis */

    RefreshUserList(ua->UserList,ua->UsersList);
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(UserList));
    gtk_tree_selection_set_mode(selection,GTK_SELECTION_SINGLE);
    model=gtk_tree_view_get_model(GTK_TREE_VIEW(UserList));
    ua->Model = model;
    ua->UserSelect = selection;

    gtk_grid_attach(GTK_GRID(table) , UserList , 0 , 0 , 3 , 6);
    gtk_grid_set_row_spacing(GTK_GRID(table), 10);
    gtk_grid_set_column_spacing(GTK_GRID(table), 10);

    g_signal_connect(selection, "changed", G_CALLBACK(on_changed), ua);
}
static void QuitApp(GtkWidget *widget, gpointer data)
{
    UserAdmin *ua = (UserAdmin *)data;
    g_slist_free_full(ua->UsersList,g_object_unref); 
    g_strfreev(all_languages);
    g_hash_table_destroy(LocaleHash);
    g_slist_free (LangList);
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
    GtkWidget *ButtonRemove;
    GtkWidget *ButtonAdd;
    GtkWidget *ButtonClose;
    GtkWidget *LableSpace;
    GtkWidget *table;
    table = gtk_grid_new();
    gtk_grid_set_column_homogeneous(GTK_GRID(table),TRUE);
    gtk_box_pack_start(GTK_BOX(Vbox),table, TRUE, TRUE,0);

    LableSpace = gtk_label_new(NULL);
    gtk_grid_attach(GTK_GRID(table) , LableSpace , 0 , 0 , 4 , 1);
    ButtonAdd    =  gtk_button_new_with_label(_("Add User"));
    ButtonRemove =  gtk_button_new_with_label(_("Remove User"));
    ButtonClose  =  gtk_button_new_with_label(_("Close"));

    gtk_grid_attach(GTK_GRID(table) , ButtonRemove , 0 , 1 , 1 , 1);
    gtk_grid_attach(GTK_GRID(table) , ButtonAdd ,    1 , 1 , 1 , 1);
    gtk_grid_attach(GTK_GRID(table) , ButtonClose ,  4 , 1 , 1 , 1);
    g_signal_connect (ButtonRemove, 
                      "clicked",
                      G_CALLBACK (RemoveUser),
                      ua);
    g_signal_connect (ButtonAdd, 
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

