/*  group-service 
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
#include "user-group-window.h"
#include "user-group.h"
#include "user-info.h"
#include "user-share.h"
#include <pwd.h>
#include <sys/types.h>
#include <grp.h>

#define  USER_GROUP_PERMISSION  "org.group.admin.group-administration"

typedef struct
{
    GSList           *GroupsList;
    GtkWidget        *GroupsWindow;
    GtkWidget        *NoteBook;
    GtkWidget        *SwitchBox;
    GtkWidget        *CreateBox;
    GtkWidget        *RemoveBox;
    GtkWidget        *ButtonConfirm;
    GtkWidget        *ButtonRemove;
    GtkWidget        *TreeSwitch;
    GtkWidget        *TreeCreate;
    GtkWidget        *TreeRemove;
    GtkListStore     *SwitchStore;
    GtkListStore     *RemoveStore;
    GtkWidget        *EntryGroupName;
    GtkWidget        *ButtonLock;
    GPermission      *Permission;
    int               GroupNum;
    const gchar      *username;
    const gchar      *remove_group_name;
    GasGroupManager  *g_manager;
    GSList           *NewGroupUsers;
}GroupsManage;
enum
{
    COLUMN_FIXED,
    COLUMN_GROUPNAME,
    COLUMN_GROUPID,
    COLUMN_DATA,
    NUM_COLUMNS
};
enum
{
  COLUMN_SELECT,
  COLUMN_USERNAME,
  COLUMN_USERID,
  NUM_USER
};
enum
{
  COLUMN_ICON,
  COLUMN_REMOVENAME,
  COLUMN_REMOVEID,
  COLUMN_GROUP,
  NUM_REMOVE
};
static void user_group_window_update_list_store (GroupsManage *gm);
static void addswitchlistdata    (GtkWidget    *tree,
                                  GtkListStore *store,
                                  UserGroup    *group,
                                  const gchar  *name);

static void addremovelistdata    (GtkWidget    *tree,
                                  GtkListStore *store,
                                  UserGroup    *group);

static gboolean CheckGroupNameUsed (const gchar *name)
{
    struct group *grent;
    grent = getgrnam (name);

    return grent != NULL;
}    
static gboolean CheckGroupName (const gchar *name, gchar **Message)
{
    gboolean empty;
    gboolean in_use;
    gboolean character;
    gboolean valid;
    const gchar *c;

    if (name == NULL || name[0] == '\0') 
    {
        empty = TRUE;
        in_use = FALSE;
        character = FALSE;
    } 
    else 
    {
        empty     = FALSE;
        character = !g_ascii_isalpha(name[0]);
        in_use    = CheckGroupNameUsed (name);
    }
    valid = TRUE;
    if (!in_use && !empty && !character) 
    {
        for (c = name; *c; c++) 
        {
            if (! ((*c >= 'a' && *c <= 'z') ||
                   (*c >= 'A' && *c <= 'Z') ||
                   (*c >= '0' && *c <= '9') ||
                   (*c == '_') || (*c == '.') ||
                   (*c == '-' && c != name)))
               valid = FALSE;
        }
    }

    valid = !empty && !in_use && !character &&valid;
    if (!empty && (in_use || character || !valid))
    {
        if (in_use) 
        {
            *Message = g_strdup (_("Repeat of group name.Please try another"));
        }
        else if (name[0] == '-') 
        {
            *Message = g_strdup (_("The groupname cannot start with a - ."));
        }
        else if(character)
        {
            *Message = g_strdup (_("The first character of the group name needs use letter"));
        }    
        else 
        {
            *Message = g_strdup (_("The groupname should only consist of upper and lower case \nletters from a-z,digits and the following characters: . - _"));
        }
    }

    return valid;
}

static gboolean QuitGroupWindow (GtkWidget *widget,
                                 GdkEvent  *event,
                                 gpointer   data)
{
    GroupsManage *gm = (GroupsManage *)data;
    g_free((gpointer)gm->username);
    if(gm->GroupsList != NULL)
        g_slist_free_full (gm->GroupsList,g_object_unref);
    if(gm->NewGroupUsers != NULL)
        g_slist_free(gm->NewGroupUsers);
    g_free (gm);
    gtk_widget_destroy(gm->GroupsWindow);
	gtk_widget_show(WindowLogin);
    return TRUE;
}    

static void CloseGroupWindow (GtkWidget *widget, gpointer data)
{
    GroupsManage *gm = (GroupsManage *)data;
    g_free((gpointer)gm->username);
    gm->Permission = NULL;
    g_slist_free_full (gm->GroupsList,g_object_unref);
    if(gm->NewGroupUsers != NULL)
        g_slist_free(gm->NewGroupUsers);   
    gtk_widget_destroy(gm->GroupsWindow);
	gtk_widget_show(WindowLogin);
}    
static gboolean restartlist  (GtkTreeModel *model,
                              GtkTreePath  *path,
                              GtkTreeIter  *iter,
                              gpointer      data)
{    
    gtk_list_store_set (GTK_LIST_STORE (model), 
                        iter, 
                        COLUMN_FIXED, 
                        FALSE, 
                        -1);
    return FALSE;
}
static void clearconfigdata(GroupsManage *gm)
{
    GtkTreeModel *model;
        
    gtk_entry_set_text(GTK_ENTRY(gm->EntryGroupName),""); 
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(gm->TreeCreate));
    gtk_tree_model_foreach (model,
                            restartlist,
                            NULL);
    if(gm->NewGroupUsers != NULL)
    {
        g_slist_free(gm->NewGroupUsers);
        gm->NewGroupUsers = NULL;
    }    
}   

static void CreateNewGroup(GtkWidget *widget, gpointer data)
{
    GroupsManage *gm = (GroupsManage *)data;
    gboolean      Valid;
    char         *Message = NULL;
    const char   *s;
    GasGroup     *gas = NULL;
    GError       *error = NULL;
    GasGroupManager *manage;
    
    s = gtk_entry_get_text(GTK_ENTRY(gm->EntryGroupName));
    Valid = CheckGroupName(s,&Message);  
    if(Valid == FALSE)
    {
        MessageReport(_("Create New Group"),Message,ERROR);
        return;
    }
    manage = gas_group_manager_get_default();
    gas    = gas_group_manager_create_group(manage,s,&error);
    if(gas == NULL)
    {
        MessageReport(_("Create New Group Faild"),error->message,ERROR);
        if(error != NULL)
        {
            g_error_free(error);
            return;
        }			
    }
}   

static UserGroup *user_group_tree_get_group (GtkWidget *widget)
{
    GtkTreeView  *treeview = GTK_TREE_VIEW(widget);
    GtkTreeModel *model;
    GtkTreeIter   iter;
    UserGroup    *group = NULL;

    GtkTreeSelection *selection = gtk_tree_view_get_selection (treeview);
    model = gtk_tree_view_get_model (treeview);
    if(gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
        gtk_tree_model_get (model, &iter, COLUMN_GROUP, &group, -1);  
    }
	
    return group;
}		

static void RemoveGroup(GtkWidget *widget, gpointer data)
{
    GroupsManage *gm = (GroupsManage *)data;
    UserGroup    *group;
    int           ret;
    GasGroupManager *GroupManager;
    GError       *error = NULL;

    group = user_group_tree_get_group (gm->TreeRemove);
    if(group != NULL)
    {
        ret = MessageReport(_("Remove Group"),
                            _("Whether to remove the selected group"),
                             QUESTIONNORMAL);
        if(ret == GTK_RESPONSE_NO)
        {
            return;
        }
        GroupManager = gas_group_manager_get_default ();
        if(!user_group_remove_group(GroupManager,
                                    group,
                                    &error))
        {
            MessageReport(_("Remove Group"),
                           error->message,
                           ERROR);
            if(error != NULL)
            {
                g_error_free(error);
            }
            return;
        }
    }
}
static int GetGroupNum(GSList *List)
{
    return g_slist_length(List);
}    

static void UserSelectGroup (GtkCellRendererToggle *cell,
                             gchar                 *path_str,
                             gpointer               data)
{
    GroupsManage *gm       = (GroupsManage *)data;
    GtkTreeView  *treeview = GTK_TREE_VIEW(gm->TreeSwitch);
    GtkTreeModel *model;
    GtkTreeIter   iter;
    GtkTreePath   *path;
    gboolean      fixed;
    UserGroup    *group; 

    model = gtk_tree_view_get_model (treeview);
    path  = gtk_tree_path_new_from_string (path_str); 
    gtk_tree_model_get_iter (model, &iter, path);
    gtk_tree_model_get (model, &iter, COLUMN_FIXED, &fixed, -1);
    gtk_tree_model_get (model, &iter, COLUMN_DATA, &group, -1); 
    
    if(fixed == TRUE)
    {
        user_group_remove_user_from_group (group, gm->username);
    }   
    else
    {
        user_group_add_user_to_group (group, gm->username);
    }
    fixed ^= 1;
    gtk_list_store_set (GTK_LIST_STORE (gm->SwitchStore), &iter, COLUMN_FIXED, fixed, -1);
    gtk_tree_path_free (path);
}
static void NewGroupSelectUsers (GtkCellRendererToggle *cell,
                                 gchar                 *path_str,
                                 gpointer               data)
{
    GroupsManage *gm       = (GroupsManage *)data;
    GtkTreeView  *treeview = GTK_TREE_VIEW(gm->TreeCreate);
    GtkTreeModel *model;
    GtkTreeIter   iter;
    GtkTreePath  *path;
    gboolean      fixed;
    const gchar  *name;  

    model = gtk_tree_view_get_model (treeview);
    path  = gtk_tree_path_new_from_string (path_str);
    /* get toggled iter */
    gtk_tree_model_get_iter (model, &iter, path);
    gtk_tree_model_get (model, &iter, COLUMN_SELECT,   &fixed, -1);
    gtk_tree_model_get (model, &iter, COLUMN_USERNAME, &name, -1); 
    if(fixed == FALSE)
    {
        gm->NewGroupUsers = g_slist_prepend(gm->NewGroupUsers,(gpointer)name);
    }   
    else
    {
        gm->NewGroupUsers = g_slist_remove(gm->NewGroupUsers,name);
    }    
    fixed ^= 1;
    gtk_list_store_set (GTK_LIST_STORE (model), &iter, COLUMN_SELECT, fixed, -1);
    gtk_tree_path_free (path);
}
static void addswitchlistdata(GtkWidget    *tree,
                              GtkListStore *store,
                              UserGroup    *group,
                              const gchar  *name)
{
    GtkTreeIter   iter;
    GtkTreeSelection *selection;
    
    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(tree));
    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter,
                        COLUMN_FIXED,     user_group_user_is_group (group, name),
                        COLUMN_GROUPNAME, user_group_get_group_name (group),
                        COLUMN_GROUPID,   user_group_get_group_id (group),
                        COLUMN_DATA,      group,
                        -1);
    gtk_tree_selection_select_iter (selection,&iter);
}		
static void addremovelistdata(GtkWidget    *tree,
                              GtkListStore *store,
                              UserGroup    *group)
{
    GtkTreeIter   iter;
    GtkTreeSelection *selection;
    
    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(tree));
    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter,
                        COLUMN_ICON,      "edit-delete",
                        COLUMN_REMOVENAME, user_group_get_group_name (group),
                        COLUMN_REMOVEID,   user_group_get_group_id (group),
                        COLUMN_GROUP, group,
                        -1);
    gtk_tree_selection_select_iter (selection,&iter);
}

static GtkTreeModel *GetSwicthModel(void)
{
    GtkListStore *SwitchStore = NULL;
    SwitchStore = gtk_list_store_new (NUM_COLUMNS,
                                      G_TYPE_BOOLEAN,
                                      G_TYPE_STRING,
                                      G_TYPE_UINT,
                                      G_TYPE_POINTER);
    return GTK_TREE_MODEL (SwitchStore);

}

static GtkTreeModel *GetRemoveModel (void)
{
    GtkListStore *RemoveStore = NULL;
    
    RemoveStore = gtk_list_store_new (NUM_REMOVE,
                                      G_TYPE_STRING,
                                      G_TYPE_STRING,
                                      G_TYPE_UINT,
                                      G_TYPE_POINTER);
    
    return GTK_TREE_MODEL (RemoveStore);
}

static GtkTreeModel * CreateAddUsersModel (GSList *List)
{
    gint          i = 0;
    GtkListStore *store;
    GtkTreeIter   iter;
    int           UserNum = 0;
	ActUser      *Actuser;

    UserNum = GetGroupNum(List);
    store = gtk_list_store_new (NUM_USER,
                                G_TYPE_BOOLEAN,
                                G_TYPE_STRING,
                                G_TYPE_UINT);

    
    for (i = 0; i < UserNum ; i++)
    {
        Actuser = g_slist_nth_data(List,i); 
        if(Actuser == NULL)
        {
            g_error("No such the User!!!\r\n");
            break;
        }    
        gtk_list_store_append (store, &iter);
        gtk_list_store_set (store, &iter,
                            COLUMN_SELECT,   FALSE,
                            COLUMN_USERNAME, GetUserName(Actuser),
                            COLUMN_USERID,   GetUserUid (Actuser),
                            -1);
    }

    return GTK_TREE_MODEL (store);
}
static void AddSwitchGroupColumns (GroupsManage *gm)
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeView  *treeview = GTK_TREE_VIEW(gm->TreeSwitch);
    
    renderer = gtk_cell_renderer_toggle_new (); 
    g_signal_connect (renderer, 
                     "toggled",
                      G_CALLBACK (UserSelectGroup), 
                      gm);

    column = gtk_tree_view_column_new_with_attributes (_("Select"),
                                                        renderer,
                                                       "active", COLUMN_FIXED,
                                                        NULL);

    gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                     GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_fixed_width (GTK_TREE_VIEW_COLUMN (column), 50);
    gtk_tree_view_column_set_sort_column_id (column, COLUMN_FIXED);
    gtk_tree_view_append_column (treeview, column);

    renderer = gtk_cell_renderer_text_new (); 
    column = gtk_tree_view_column_new_with_attributes (_("Group Name"),
                                                        renderer,
                                                       "text",
                                                        COLUMN_GROUPNAME,
                                                        NULL);
    gtk_tree_view_column_set_sort_column_id (column, COLUMN_GROUPNAME);
    gtk_tree_view_append_column (treeview, column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (_("Group ID"),
                                                        renderer,
                                                       "text",
                                                        COLUMN_GROUPID,
                                                        NULL);
    gtk_tree_view_column_set_sort_column_id (column, COLUMN_GROUPID);
    gtk_tree_view_append_column (treeview, column);

}
static void AddSelectUsersColumns (GroupsManage *gm)
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeView  *treeview = GTK_TREE_VIEW(gm->TreeCreate);
    
    renderer = gtk_cell_renderer_toggle_new (); 
    g_signal_connect (renderer, 
                     "toggled",
                      G_CALLBACK (NewGroupSelectUsers), 
                      gm);

    column = gtk_tree_view_column_new_with_attributes (_("Select"),
                                                        renderer,
                                                       "active", COLUMN_SELECT,
                                                        NULL);

    gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                     GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_fixed_width (GTK_TREE_VIEW_COLUMN (column), 50);
    gtk_tree_view_append_column (treeview, column);

    renderer = gtk_cell_renderer_text_new (); 
    column = gtk_tree_view_column_new_with_attributes (_("Username"),
                                                        renderer,
                                                       "text",
                                                        COLUMN_USERNAME,
                                                        NULL);
    gtk_tree_view_column_set_sort_column_id (column, COLUMN_USERNAME);
    gtk_tree_view_append_column (treeview, column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (_("User ID"),
                                                        renderer,
                                                       "text",
                                                        COLUMN_USERID,
                                                        NULL);
    gtk_tree_view_column_set_sort_column_id (column, COLUMN_USERID);
    gtk_tree_view_append_column (treeview, column);

}

static void AddRemoveGroupColumns (GroupsManage *gm)
{
    GtkCellRenderer   *renderer;
    GtkTreeViewColumn *column;
    GtkTreeView  *treeview = GTK_TREE_VIEW(gm->TreeRemove);
    
    renderer = gtk_cell_renderer_pixbuf_new ();
    column = gtk_tree_view_column_new_with_attributes (_("Remove"),
                                                        renderer,
                                                       "icon-name",
                                                        COLUMN_ICON,
                                                        NULL);
    gtk_tree_view_column_set_sort_column_id (column, COLUMN_ICON);
    gtk_tree_view_append_column (treeview, column);

    renderer = gtk_cell_renderer_text_new (); 
    column = gtk_tree_view_column_new_with_attributes (_("Group Name"),
                                                        renderer,
                                                       "text",
                                                        COLUMN_REMOVENAME,
                                                        NULL);
    gtk_tree_view_column_set_sort_column_id (column, COLUMN_REMOVENAME);
    gtk_tree_view_append_column (treeview, column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (_("Group ID"),
                                                        renderer,
                                                       "text",
                                                        COLUMN_REMOVEID,
                                                        NULL);
    gtk_tree_view_column_set_sort_column_id (column, COLUMN_REMOVEID);
    gtk_tree_view_append_column (treeview, column);

}

static GasGroupManager *user_group_start_group_manager (void)
{
    GasGroupManager *GroupManager = NULL;

    GroupManager = gas_group_manager_get_default ();
    if(GroupManager == NULL)
    {
        MessageReport(_("Initialization group management"),
                      _("Initialization failed, please see Group \n Management Service Interface function"),
                      ERROR);
        return NULL;
    }
    if( gas_group_manager_no_service(GroupManager) == TRUE)
    {
        MessageReport(_("Failed to contact the group service"),
                      _("Please make sure that the group-service is installed and enabled.\n url: https://github.com/zhuyaliang/group-service"),
                        ERROR);
        return  NULL;
    }

    return GroupManager;
}

static GSList *user_group_group_list (GasGroupManager *g_manager)
{
    GSList *list, *l;
    GSList *GroupsList = NULL;
    int i = 0;
    UserGroup *usergroup;

    list = gas_group_manager_list_groups (g_manager);
    for(l = list; l ; l = l->next,i++)
    {
        usergroup = user_group_new (l->data);
        if(usergroup != NULL)
        {
            GroupsList = g_slist_append(GroupsList,g_object_ref(usergroup));
        }
    }
    g_slist_free (list);

    return GroupsList;
}
static void AddUnlockTooltip(GroupsManage *gm,gboolean mode)
{
    if(!mode)
    {
        if(GetUseHeader() == 1)
            gtk_widget_set_tooltip_markup(gm->GroupsWindow,
                                     _("Please click the unlock sign in the upper left corner"));
        else
            gtk_widget_set_tooltip_markup(gm->GroupsWindow,
                                     _("Click the unlock button on the \"swith-group\" page"));
    }    
    else
    {
        gtk_widget_set_tooltip_markup(gm->GroupsWindow,NULL);
    }    
}    
static void UpdateState(GroupsManage *gm)
{
    gboolean Authorized;

    Authorized = g_permission_get_allowed (G_PERMISSION (gm->Permission));
    gtk_widget_set_sensitive(gm->TreeSwitch,Authorized);
    gtk_widget_set_sensitive(gm->TreeCreate,Authorized);
    gtk_widget_set_sensitive(gm->EntryGroupName,Authorized);
    gtk_widget_set_sensitive(gm->ButtonConfirm,Authorized);
    gtk_widget_set_sensitive(gm->TreeRemove,Authorized);
    gtk_widget_set_sensitive(gm->ButtonRemove,Authorized);
    AddUnlockTooltip(gm,Authorized);
}    
static void on_permission_changed (GPermission *permission,
                                   GParamSpec  *pspec,
                                   gpointer     data)
{
    GroupsManage *gm = (GroupsManage *)data;
    
    UpdateState(gm);
}    
static void LoadHeader_bar(GroupsManage *gm,const char *title)
{
    GtkWidget   *header;
    
    header = gtk_header_bar_new ();
    gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (header), TRUE);
    gtk_header_bar_set_title (GTK_HEADER_BAR (header), _("Groups Manage"));
    gtk_header_bar_set_has_subtitle (GTK_HEADER_BAR (header), TRUE);
    gtk_header_bar_set_subtitle(GTK_HEADER_BAR (header),title);

    gtk_header_bar_pack_start (GTK_HEADER_BAR (header), gm->ButtonLock);
    gtk_window_set_titlebar (GTK_WINDOW (gm->GroupsWindow), header);
}    
static void CreateManageWindow(GroupsManage *gm)
{
    GtkWidget   *Window;
    gchar       *title;
    GError      *error = NULL;

    title = g_strdup_printf(_("Current user -- %s"),gm->username);
    
    Window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_position(GTK_WINDOW(Window), GTK_WIN_POS_CENTER);
    gtk_container_set_border_width(GTK_CONTAINER(Window),10);
    gm->GroupsWindow = Window;
    
    gm->Permission = polkit_permission_new_sync (USER_GROUP_PERMISSION, NULL, NULL, &error);
    if (gm->Permission == NULL)
    {
        mate_uesr_admin_log ("Warning","Cannot create '%s' permission: %s", USER_GROUP_PERMISSION, error->message);
        g_error_free (error);
    }
    gm->ButtonLock = gtk_lock_button_new(gm->Permission);
    gtk_lock_button_set_permission(GTK_LOCK_BUTTON (gm->ButtonLock),gm->Permission);
    gtk_widget_grab_focus(gm->ButtonLock);    
    g_signal_connect(gm->Permission, 
                    "notify",
                     G_CALLBACK (on_permission_changed), 
                     gm);
    if(GetUseHeader() == 1)
    {
        LoadHeader_bar(gm,title);
    }    
    else
    {
        gtk_window_set_title(GTK_WINDOW(Window),title);
    }    
    g_free(title);
} 
static GtkWidget *GetGridWidget (void)
{
    GtkWidget *table;
    
    table = gtk_grid_new();
    gtk_grid_set_column_homogeneous(GTK_GRID(table),TRUE);
    gtk_grid_set_row_spacing(GTK_GRID(table), 10);
    gtk_grid_set_column_spacing(GTK_GRID(table), 10);
    
    return table;
}    
static GtkWidget *GetScrolledWidget (void)
{
    GtkWidget *Scrolled;
    
    Scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (Scrolled),
                                         GTK_SHADOW_ETCHED_IN);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (Scrolled),
                                    GTK_POLICY_NEVER,
                                    GTK_POLICY_AUTOMATIC);

    return Scrolled;
}
static GtkWidget *LoadSwitchGroup(GroupsManage *gm)
{
    GtkWidget *vbox;
    GtkWidget *vbox1;
    GtkWidget *Scrolled;
    GtkTreeModel *model;
    GtkWidget *treeview;
    GtkWidget *table;
    GtkWidget *ButtonClose;
    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
 
    vbox1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request (vbox1, -1,200);
    gtk_widget_set_hexpand(vbox1,TRUE); 
    gtk_widget_set_vexpand(vbox1,TRUE); 
    
    table = GetGridWidget();
    gtk_box_pack_start(GTK_BOX(vbox),table, TRUE, TRUE,0);
    Scrolled = GetScrolledWidget(); 
    gtk_box_pack_start (GTK_BOX (vbox1), Scrolled, TRUE, TRUE, 0);
    
    model    = GetSwicthModel();
    gm->SwitchStore = GTK_LIST_STORE(model);	
    treeview = gtk_tree_view_new_with_model (model);
    gtk_tree_view_set_search_column (GTK_TREE_VIEW (treeview),
                                     COLUMN_GROUPID);
    g_object_unref (model);
    gtk_container_add (GTK_CONTAINER (Scrolled), treeview);

    gm->TreeSwitch = treeview;
    AddSwitchGroupColumns (gm);
    
    gtk_grid_attach(GTK_GRID(table) , vbox1 , 0 , 0 , 3 , 1); 
    
    ButtonClose    =  SetButtonIcon(_("Close"),"window-close");
    gtk_grid_attach(GTK_GRID(table) , ButtonClose , 0 , 1 , 1 , 1); 
    if(GetUseHeader() == 0)
    {
    
        gtk_grid_attach(GTK_GRID(table) , gm->ButtonLock , 2 , 1 , 1 , 1); 
    }    
    g_signal_connect (ButtonClose, 
                     "clicked",
                      G_CALLBACK (CloseGroupWindow),
                      gm);
    return vbox;
}

static GtkWidget *LoadCreateGroup(GroupsManage *gm,GSList *List)
{
    GtkWidget *vbox;
    GtkWidget *vbox1;
    GtkWidget *Hbox;
    GtkWidget *Scrolled;
    GtkWidget *treeview;
    GtkWidget *table;
    GtkWidget *GroupNameLabel;
    GtkWidget *TipsLabel;
    GtkWidget *ButtonClose;
    GtkWidget *ButtonConfirm;
    GtkWidget *ButtonPlace;
    GtkTreeModel     *model;

    vbox  = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    vbox1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request (vbox1, -1,130);
    gtk_widget_set_hexpand(vbox1,TRUE); 
    gtk_widget_set_vexpand(vbox1,TRUE); 

    table = GetGridWidget();
    gtk_box_pack_start(GTK_BOX(vbox),table, TRUE, TRUE,0);
    
    GroupNameLabel = gtk_label_new(NULL);
    SetLableFontType(GroupNameLabel,"gray",10,_("New Group Name"),TRUE);
    gtk_grid_attach(GTK_GRID(table) ,GroupNameLabel ,0,0,1 ,1);

    gm->EntryGroupName = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(gm->EntryGroupName),48);
    gtk_grid_attach(GTK_GRID(table) ,gm->EntryGroupName ,1,0,1 ,1);

    Scrolled = GetScrolledWidget();
    gtk_box_pack_start (GTK_BOX (vbox1), Scrolled, TRUE, TRUE, 0);
    
    TipsLabel = gtk_label_new(NULL);
    SetLableFontType(TipsLabel,"black",12,_("Please select the user to add to the new group"),FALSE);
    gtk_grid_attach(GTK_GRID(table) , TipsLabel ,0 ,1,2 ,1);

    model = CreateAddUsersModel(List);
    treeview = gtk_tree_view_new_with_model (model);
    gtk_tree_view_set_search_column (GTK_TREE_VIEW (treeview),
                                     COLUMN_USERID);
    g_object_unref (model);
    gtk_container_add (GTK_CONTAINER (Scrolled), treeview);
    gm->TreeCreate = treeview;
    gm->NewGroupUsers = NULL;
    AddSelectUsersColumns (gm);
    gtk_grid_attach(GTK_GRID(table) ,vbox1 ,0,2,2,1);
    
    Hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5);
    gtk_grid_attach(GTK_GRID(table) , Hbox, 0,3,2,1);
    ButtonClose    =  SetButtonIcon(_("Close"),"window-close");
    g_signal_connect (ButtonClose, 
                     "clicked",
                      G_CALLBACK (CloseGroupWindow),
                      gm);
    gtk_box_pack_start (GTK_BOX (Hbox), ButtonClose, TRUE, TRUE, 0);
    
    ButtonPlace    =  gtk_label_new(NULL);
    gtk_box_pack_start (GTK_BOX (Hbox), ButtonPlace, TRUE, TRUE, 0);
    gtk_widget_hide(ButtonPlace);
    
    ButtonConfirm    =  SetButtonIcon(_("Confirm"),"object-select");
    g_signal_connect (ButtonConfirm, 
                     "clicked",
                      G_CALLBACK (CreateNewGroup),
                      gm);
    gtk_box_pack_start (GTK_BOX (Hbox), ButtonConfirm,TRUE, TRUE, 0);
    gm->ButtonConfirm = ButtonConfirm; 
    
    return vbox;
}
static GtkWidget *LoadRemoveGroup(GroupsManage *gm)
{
    GtkWidget *vbox;
    GtkWidget *vbox1;
    GtkWidget *Scrolled;
    GtkTreeModel *model;
    GtkWidget *treeview;
    GtkWidget *table;
    GtkWidget *ButtonClose;
    GtkWidget *ButtonRemove;

    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
 
    vbox1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request (vbox1, -1,200);
    gtk_widget_set_hexpand(vbox1,TRUE); 
    gtk_widget_set_vexpand(vbox1,TRUE); 
    table = GetGridWidget();
    gtk_box_pack_start(GTK_BOX(vbox),table, TRUE, TRUE,0);
    Scrolled = GetScrolledWidget(); 
    gtk_box_pack_start (GTK_BOX (vbox1), Scrolled, TRUE, TRUE, 0);
    
    model    = GetRemoveModel();
    gm->RemoveStore = GTK_LIST_STORE(model);	
    treeview = gtk_tree_view_new_with_model (model);
    gtk_tree_view_set_search_column (GTK_TREE_VIEW (treeview),
                                     COLUMN_REMOVEID);
    g_object_unref (model);
    gtk_container_add (GTK_CONTAINER (Scrolled), treeview);

    gm->TreeRemove = treeview;
    AddRemoveGroupColumns (gm);
    
    gtk_grid_attach(GTK_GRID(table) , vbox1 , 0 , 0 , 3 , 1); 
    
    ButtonClose    =  SetButtonIcon(_("Close"),"window-close");
    gtk_grid_attach(GTK_GRID(table) , ButtonClose , 0 , 1 , 1 , 1); 
    g_signal_connect (ButtonClose, 
                     "clicked",
                      G_CALLBACK (CloseGroupWindow),
                      gm);
 
    ButtonRemove    =  SetButtonIcon(_("Remove"),"list-remove");
    gtk_grid_attach(GTK_GRID(table) , ButtonRemove , 2 , 1 , 1 , 1);
    g_signal_connect (ButtonRemove, 
                     "clicked",
                      G_CALLBACK (RemoveGroup),
                      gm);
    gm->ButtonRemove = ButtonRemove;
    return vbox;
}

static void StartManageGroups (GroupsManage *gm,GSList *UsersList)
{
    GtkWidget *NoteBook;
    GtkWidget *SwitchNoteName;
    GtkWidget *CreateNoteName;
    GtkWidget *RemoveNoteName;
    
    NoteBook = gtk_notebook_new();
    gtk_container_add(GTK_CONTAINER(gm->GroupsWindow), NoteBook);
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK (NoteBook), GTK_POS_TOP);
    
    gm->NoteBook = NoteBook;
    SwitchNoteName = gtk_label_new(_("Switch Groups"));
    gm->SwitchBox = LoadSwitchGroup(gm);
    gtk_notebook_append_page(GTK_NOTEBOOK (NoteBook),gm->SwitchBox,SwitchNoteName);

    CreateNoteName = gtk_label_new(_("Create Groups"));
    gm->CreateBox = LoadCreateGroup(gm,UsersList);
    gtk_notebook_append_page(GTK_NOTEBOOK (NoteBook),gm->CreateBox,CreateNoteName);

    RemoveNoteName = gtk_label_new(_("Remove Groups"));
    gm->RemoveBox = LoadRemoveGroup(gm);
    gtk_notebook_append_page(GTK_NOTEBOOK (NoteBook),gm->RemoveBox,RemoveNoteName);
    
    gtk_widget_show_all(gm->GroupsWindow);
}    

static void user_group_window_update_list_store (GroupsManage *gm)
{
    gint          i = 0;
    int           GroupNum = 0;
    UserGroup    *group;

    GroupNum = GetGroupNum (gm->GroupsList);
    gtk_list_store_clear (GTK_LIST_STORE (gm->SwitchStore));
    gtk_list_store_clear (GTK_LIST_STORE (gm->RemoveStore));
    for (i = 0; i < GroupNum ; i++)
    {
        group = g_slist_nth_data(gm->GroupsList, i); 
        if(group == NULL)
        {
            g_error("No such the Group!!!\r\n");
            break;
        }   
        addswitchlistdata(gm->TreeSwitch, gm->SwitchStore, group, gm->username);
        if(!user_group_is_primary_group (group) &&
            user_group_get_group_id (group) >= 1000)
            addremovelistdata (gm->TreeRemove, gm->RemoveStore, group);
    }

}

static void list_remove_data (gpointer  data,
                              gpointer  user_data)
{
    GroupsManage *gm = user_data;
    UserGroup    *group = data;

    if (g_strcmp0 (gm->remove_group_name, user_group_get_group_name (group)) == 0)
        gm->GroupsList = g_slist_remove_all (gm->GroupsList, group);
}
static void user_group_remove_cb (GasGroupManager *g_manager,
                                  GasGroup        *gas,
                                  GroupsManage    *gm)
{
    UserGroup  *group;
    const char *name;

    group = user_group_new (gas);
    name = user_group_get_group_name (group);
    gm->remove_group_name = name;

    g_slist_foreach (gm->GroupsList, list_remove_data, gm);
    gm->GroupsList = g_slist_remove_all (gm->GroupsList, group);
    g_object_unref (group);
    user_group_window_update_list_store (gm);
}

static void user_group_add_cb (GasGroupManager *g_manager,
                               GasGroup        *gas,
                               GroupsManage    *gm)
{
    UserGroup *group;

    add_user_to_new_group (gm->NewGroupUsers, gas);
    group = user_group_new (gas);
    gm->GroupsList = g_slist_append (gm->GroupsList, g_object_ref(group));
    MessageReport(_("Create User Group"),
                  _("Create User Group Successfully,Please view the end of the switch-groups list."),
                   INFOR);

    user_group_window_update_list_store (gm);
    gtk_notebook_prev_page(GTK_NOTEBOOK(gm->NoteBook));
    clearconfigdata(gm);
}

void UserGroupsManage (const char *user_name, GSList *user_list)
{
    GroupsManage *gm = g_new0 (GroupsManage, 1);

    gm->username = g_strdup(user_name);
    gtk_widget_hide(WindowLogin);
    gm->GroupsList = NULL;
    gm->NewGroupUsers = NULL;
    gm->g_manager = user_group_start_group_manager ();
    gm->GroupsList = user_group_group_list (gm->g_manager);

    g_signal_connect (gm->g_manager,
                     "group-added",
                     G_CALLBACK (user_group_add_cb),
                     gm);

    g_signal_connect (gm->g_manager,
                     "group-removed",
                     G_CALLBACK (user_group_remove_cb),
                     gm);
    if(gm->GroupsList == NULL)
    {
        gtk_widget_show(WindowLogin);
        return;
    }
    gm->GroupNum = g_slist_length(gm->GroupsList);
    CreateManageWindow(gm);
    StartManageGroups(gm, user_list);
    UpdateState(gm);
    user_group_window_update_list_store (gm);
    g_signal_connect(G_OBJECT(gm->GroupsWindow),
                    "delete-event",
                     G_CALLBACK(QuitGroupWindow),
                     gm);
}
