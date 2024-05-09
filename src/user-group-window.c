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

enum
{
    WINDOW_CLOSED,
    LAST_SIGNAL
};

enum
{
    COLUMN_FIXED,
    COLUMN_ICON,
    COLUMN_NAME,
    COLUMN_ID,
    COLUMN_DATA,
    NUM_COLUMNS
};

struct _UserGroupWindowPrivate
{
    GSList           *group_list;
    GasGroupManager  *g_manager;
    GPermission      *permission;

    GtkWidget        *ButtonConfirm;
    GtkWidget        *ButtonRemove;
    GtkWidget        *TreeSwitch;
    GtkWidget        *TreeCreate;
    GtkWidget        *TreeRemove;
    GtkListStore     *SwitchStore;
    GtkListStore     *RemoveStore;
    GtkListStore     *UserStore;
    GtkWidget        *EntryGroupName;
    int               remove_id;
    int               add_id;
    const gchar      *user_name;
    const gchar      *remove_group_name;
    GSList           *NewGroupUsers;
};

static guint signals[LAST_SIGNAL] = { 0 };
G_DEFINE_TYPE_WITH_PRIVATE (UserGroupWindow, user_group_window, GTK_TYPE_WINDOW)
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

static void CloseGroupWindow (GtkWidget *widget, gpointer data)
{
    gtk_widget_destroy (GTK_WIDGET (data));
}

static gboolean restartlist (GtkTreeModel *model,
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

static void clearconfigdata (UserGroupWindow *win)
{
    GtkTreeModel *model;

    gtk_entry_set_text (GTK_ENTRY (win->priv->EntryGroupName), "");
    model = gtk_tree_view_get_model (GTK_TREE_VIEW (win->priv->TreeCreate));
    gtk_tree_model_foreach (model,
                            restartlist,
                            NULL);
    if(win->priv->NewGroupUsers != NULL)
    {
        g_slist_free (win->priv->NewGroupUsers);
        win->priv->NewGroupUsers = NULL;
    }
}

static void CreateNewGroup(GtkWidget *widget, gpointer data)
{
    UserGroupWindow *win = USER_GROUP_WINDOW (data);
    gboolean         Valid;
    char            *Message = NULL;
    const char      *s;
    GasGroup        *gas = NULL;
    GError          *error = NULL;
    GasGroupManager *manage;

    s = gtk_entry_get_text (GTK_ENTRY (win->priv->EntryGroupName));
    Valid = CheckGroupName (s, &Message);
    if (Valid == FALSE)
    {
        MessageReport (_("Create New Group"), Message, ERROR);
        return;
    }
    manage = gas_group_manager_get_default ();
    gas    = gas_group_manager_create_group (manage, s, &error);
    if (gas == NULL)
    {
        MessageReport (_("Create New Group Failed"), error->message, ERROR);
        if (error != NULL)
        {
            g_error_free (error);
            return;
        }
    }
}

static UserGroup *user_group_tree_get_group (GtkWidget *widget)
{
    GtkTreeView  *treeview = GTK_TREE_VIEW (widget);
    GtkTreeModel *model;
    GtkTreeIter   iter;
    UserGroup    *group = NULL;

    GtkTreeSelection *selection = gtk_tree_view_get_selection (treeview);
    model = gtk_tree_view_get_model (treeview);
    if(gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
        gtk_tree_model_get (model, &iter, COLUMN_DATA, &group, -1);
    }

    return group;
}

static void RemoveGroup(GtkWidget *widget, gpointer data)
{
    UserGroupWindow *win = USER_GROUP_WINDOW (data);
    UserGroup       *group;
    int              ret;
    GasGroupManager *GroupManager;
    GError          *error = NULL;

    group = user_group_tree_get_group (win->priv->TreeRemove);
    if (group != NULL)
    {
        ret = MessageReport (_("Remove Group"),
                             _("Whether to remove the selected group"),
                             QUESTIONNORMAL);
        if (ret == GTK_RESPONSE_NO)
        {
            return;
        }
        GroupManager = gas_group_manager_get_default ();
        if(!user_group_remove_group (GroupManager,
                                     group,
                                    &error))
        {
            MessageReport (_("Remove Group"),
                           error->message,
                           ERROR);
            if (error != NULL)
            {
                g_error_free (error);
            }
            return;
        }
    }
}

static void UserSelectGroup (GtkCellRendererToggle *cell,
                             gchar                 *path_str,
                             gpointer               data)
{
    UserGroupWindow *win = USER_GROUP_WINDOW (data);
    GtkTreeView     *treeview = GTK_TREE_VIEW (win->priv->TreeSwitch);
    GtkTreeModel    *model;
    GtkTreeIter      iter;
    GtkTreePath     *path;
    gboolean         fixed;
    UserGroup       *group;

    model = gtk_tree_view_get_model (treeview);
    path  = gtk_tree_path_new_from_string (path_str);
    gtk_tree_model_get_iter (model, &iter, path);
    gtk_tree_model_get (model, &iter, COLUMN_FIXED, &fixed, -1);
    gtk_tree_model_get (model, &iter, COLUMN_DATA, &group, -1);

    if (fixed == TRUE)
    {
        user_group_remove_user_from_group (group, win->priv->user_name);
    }
    else
    {
        user_group_add_user_to_group (group, win->priv->user_name);
    }
    fixed ^= 1;
    gtk_list_store_set (GTK_LIST_STORE (win->priv->SwitchStore), &iter, COLUMN_FIXED, fixed, -1);
    gtk_tree_path_free (path);
}

static GSList *remove_list_element (GSList *list, char *name)
{
    guint i;
    guint length;
    const char *data;

    length = g_slist_length (list);
    for (i = 0; i < length; i++)
    {
        data = g_slist_nth_data (list, i);
        if (g_strcmp0 (data, name) == 0)
        {
            list = g_slist_remove_all (list, data);
            break;
        }
    }

    return list;
}

static void NewGroupSelectUsers (GtkCellRendererToggle *cell,
                                 gchar                 *path_str,
                                 gpointer               data)
{
    UserGroupWindow *win = USER_GROUP_WINDOW (data);
    GtkTreeView     *treeview = GTK_TREE_VIEW (win->priv->TreeCreate);
    GtkTreeModel    *model;
    GtkTreeIter      iter;
    GtkTreePath     *path;
    gboolean         fixed;
    char            *name = NULL;

    model = gtk_tree_view_get_model (treeview);
    path  = gtk_tree_path_new_from_string (path_str);
    /* get toggled iter */
    gtk_tree_model_get_iter (model, &iter, path);
    gtk_tree_model_get (model, &iter, COLUMN_FIXED, &fixed, -1);
    gtk_tree_model_get (model, &iter, COLUMN_NAME, &name, -1);
    if (fixed == FALSE)
    {
        win->priv->NewGroupUsers = g_slist_prepend (win->priv->NewGroupUsers, name);
    }
    else
    {
        win->priv->NewGroupUsers = remove_list_element (win->priv->NewGroupUsers, name);
        g_free (name);
    }
    fixed ^= 1;

    gtk_list_store_set (GTK_LIST_STORE (model), &iter, COLUMN_FIXED, fixed, -1);
    gtk_tree_path_free (path);
}

static void addswitchlistdata(GtkWidget    *tree,
                              GtkListStore *store,
                              UserGroup    *group,
                              const gchar  *name)
{
    GtkTreeIter       iter;
    GtkTreeSelection *selection;

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(tree));
    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter,
                        COLUMN_FIXED, user_group_user_is_group (group, name),
                        COLUMN_NAME, user_group_get_group_name (group),
                        COLUMN_ID, user_group_get_group_id (group),
                        COLUMN_DATA, group,
                        -1);
    gtk_tree_selection_select_iter (selection,&iter);
}

static void addremovelistdata(GtkWidget    *tree,
                              GtkListStore *store,
                              UserGroup    *group)
{
    GtkTreeIter       iter;
    GtkTreeSelection *selection;

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(tree));
    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter,
                        COLUMN_ICON,      "edit-delete",
                        COLUMN_NAME, user_group_get_group_name (group),
                        COLUMN_ID, user_group_get_group_id (group),
                        COLUMN_DATA, group,
                        -1);
    gtk_tree_selection_select_iter (selection, &iter);
}

static GtkTreeModel *create_tree_model (void)
{
    GtkListStore *store = NULL;

    store = gtk_list_store_new (NUM_COLUMNS,
                                G_TYPE_BOOLEAN,
                                G_TYPE_STRING,
                                G_TYPE_STRING,
                                G_TYPE_UINT,
                                G_TYPE_POINTER);

    return GTK_TREE_MODEL (store);
}

static void user_list_store_update (GSList *List, GtkListStore *store)
{
    gint          i = 0;
    GtkTreeIter   iter;
    int           UserNum = 0;
    ActUser      *Actuser;

    UserNum = g_slist_length (List);

    for (i = 0; i < UserNum ; i++)
    {
        Actuser = g_slist_nth_data (List, i);
        if(Actuser == NULL)
        {
            g_error ("No such the User!!!\r\n");
            break;
        }
        gtk_list_store_append (store, &iter);
        gtk_list_store_set (store, &iter,
                            COLUMN_FIXED, FALSE,
                            COLUMN_NAME, GetUserName(Actuser),
                            COLUMN_ID, GetUserUid (Actuser),
                            -1);
    }
}

static GtkCellRenderer *create_tree_toggled_renderer (GtkTreeView *tree,
                                                      const char  *name,
                                                      int          index)
{
    GtkCellRenderer   *renderer;
    GtkTreeViewColumn *column;

    renderer = gtk_cell_renderer_toggle_new ();

    column = gtk_tree_view_column_new_with_attributes (name,
                                                       renderer,
                                                       "active",
                                                       index,
                                                       NULL);

    gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                     GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_fixed_width (GTK_TREE_VIEW_COLUMN (column), 50);
    gtk_tree_view_append_column (tree, column);

    return renderer;
}

static void create_tree_text_renderer (GtkTreeView *tree,
                                       const char  *name,
                                       int          index)
{
    GtkCellRenderer   *renderer;
    GtkTreeViewColumn *column;

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (name,
                                                       renderer,
                                                      "text",
                                                       index,
                                                       NULL);
    gtk_tree_view_column_set_sort_column_id (column, index);

    gtk_tree_view_append_column (tree, column);
}

static void create_tree_pixbuf_renderer (GtkTreeView *tree,
                                         const char  *name,
                                         int          index)
{
    GtkCellRenderer   *renderer;
    GtkTreeViewColumn *column;

    renderer = gtk_cell_renderer_pixbuf_new ();
    column = gtk_tree_view_column_new_with_attributes (name,
                                                       renderer,
                                                      "icon-name",
                                                       index,
                                                       NULL);
    gtk_tree_view_column_set_sort_column_id (column, index);

    gtk_tree_view_append_column (tree, column);
}

static GasGroupManager *user_group_start_group_manager (void)
{
    GasGroupManager *GroupManager = NULL;

    GroupManager = gas_group_manager_get_default ();
    if (GroupManager == NULL)
    {
        MessageReport (_("Initialization group management"),
                       _("Initialization failed, please see Group \n Management Service Interface function"),
                      ERROR);
        return NULL;
    }
    if (gas_group_manager_no_service(GroupManager) == TRUE)
    {
        MessageReport (_("Failed to contact the group service"),
                       _("Please make sure that the group-service is installed and enabled.\n url: https://github.com/zhuyaliang/group-service"),
                        ERROR);
        return  NULL;
    }

    return GroupManager;
}

static void AddUnlockTooltip (UserGroupWindow *win, gboolean mode)
{
    if (!mode)
    {
        gtk_widget_set_tooltip_markup (GTK_WIDGET (win),
                                     _("Click the unlock button on the \"swith-group\" page"));
    }
    else
    {
        gtk_widget_set_tooltip_markup (GTK_WIDGET (win), NULL);
    }
}

static void update_sensitive (UserGroupWindow *win)
{
    gboolean Authorized;

    Authorized = g_permission_get_allowed (G_PERMISSION (win->priv->permission));

    gtk_widget_set_sensitive (win->priv->TreeSwitch, Authorized);
    gtk_widget_set_sensitive (win->priv->TreeCreate, Authorized);
    gtk_widget_set_sensitive (win->priv->EntryGroupName, Authorized);
    gtk_widget_set_sensitive (win->priv->ButtonConfirm, Authorized);
    gtk_widget_set_sensitive (win->priv->TreeRemove, Authorized);
    gtk_widget_set_sensitive (win->priv->ButtonRemove, Authorized);

    AddUnlockTooltip (win, Authorized);
}

static void on_permission_changed (GPermission *permission,
                                   GParamSpec  *pspec,
                                   gpointer     data)
{
    UserGroupWindow *win = USER_GROUP_WINDOW (data);

    update_sensitive (win);
}

static GtkWidget *grid_widget_new (void)
{
    GtkWidget *table;

    table = gtk_grid_new ();
    gtk_grid_set_column_homogeneous (GTK_GRID (table), TRUE);
    gtk_grid_set_row_spacing (GTK_GRID (table), 10);
    gtk_grid_set_column_spacing (GTK_GRID (table), 10);

    return table;
}

static GtkWidget *scrolled_widget_new (void)
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

static GtkWidget *vbox_widget_new (void)
{
    GtkWidget *box;

    box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request (box, -1, 200);
    gtk_widget_set_hexpand (box, TRUE); 
    gtk_widget_set_vexpand (box, TRUE);

    return box;
}

static GtkWidget *load_select_group (UserGroupWindow *win, GtkWidget *button_lock)
{
    GtkWidget    *vbox;
    GtkWidget    *vbox1;
    GtkWidget    *Scrolled;
    GtkTreeModel *model;
    GtkWidget    *treeview;
    GtkWidget    *table;
    GtkWidget    *ButtonClose;
    GtkCellRenderer *renderer;

    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

    vbox1 = vbox_widget_new ();

    table = grid_widget_new ();
    gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);
    Scrolled = scrolled_widget_new ();
    gtk_box_pack_start (GTK_BOX (vbox1), Scrolled, TRUE, TRUE, 0);

    model = create_tree_model ();
    win->priv->SwitchStore = GTK_LIST_STORE (model);
    treeview = gtk_tree_view_new_with_model (model);
    gtk_tree_view_set_search_column (GTK_TREE_VIEW (treeview),
                                     COLUMN_ID);
    g_object_unref (model);
    gtk_container_add (GTK_CONTAINER (Scrolled), treeview);

    win->priv->TreeSwitch = treeview;
    renderer = create_tree_toggled_renderer (GTK_TREE_VIEW (treeview), _("Select"), COLUMN_FIXED);
    g_signal_connect (renderer,
                     "toggled",
                      G_CALLBACK (UserSelectGroup),
                      win);
    create_tree_text_renderer (GTK_TREE_VIEW (treeview), _("Group Name"), COLUMN_NAME);
    create_tree_text_renderer (GTK_TREE_VIEW (treeview), _("Group ID"), COLUMN_ID);

    gtk_grid_attach (GTK_GRID (table), vbox1, 0, 0, 3, 1);

    ButtonClose = SetButtonIcon (_("Close"), "window-close");
    gtk_grid_attach (GTK_GRID (table), ButtonClose, 0, 1, 1, 1);
    g_signal_connect (ButtonClose,
                     "clicked",
                      G_CALLBACK (CloseGroupWindow),
                      win);
    gtk_grid_attach (GTK_GRID (table), button_lock, 2, 1, 1, 1);
    return vbox;
}

static GtkWidget *load_create_group (UserGroupWindow *win)
{
    GtkWidget *vbox;
    GtkWidget *vbox1;
    GtkWidget *Hbox;
    GtkWidget *Scrolled;
    GtkWidget *treeview;
    GtkWidget *table;
    GtkWidget *label;
    GtkWidget *ButtonClose;
    GtkWidget *ButtonConfirm;
    GtkTreeModel     *model;
    GtkCellRenderer  *renderer;

    vbox  = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

    vbox1 = vbox_widget_new ();

    table = grid_widget_new ();
    gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);

    label = gtk_label_new (NULL);
    SetLableFontType (label, "gray", 10, _("New Group Name"), TRUE);
    gtk_grid_attach (GTK_GRID(table) ,label, 0, 0, 1, 1);

    win->priv->EntryGroupName = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY (win->priv->EntryGroupName), 48);
    gtk_grid_attach (GTK_GRID (table), win->priv->EntryGroupName, 1, 0, 1, 1);

    Scrolled = scrolled_widget_new ();
    gtk_box_pack_start (GTK_BOX (vbox1), Scrolled, TRUE, TRUE, 0);

    label = gtk_label_new (NULL);
    SetLableFontType(label, NULL, 12, _("Please select the user to add to the new group"), FALSE);
    gtk_grid_attach (GTK_GRID (table), label, 0, 1, 2, 1);

    model = create_tree_model ();
    win->priv->UserStore = GTK_LIST_STORE (model);
    treeview = gtk_tree_view_new_with_model (model);
    gtk_tree_view_set_search_column (GTK_TREE_VIEW (treeview),
                                     COLUMN_ID);
    g_object_unref (model);
    gtk_container_add (GTK_CONTAINER (Scrolled), treeview);
    win->priv->TreeCreate = treeview;

    renderer = create_tree_toggled_renderer (GTK_TREE_VIEW (treeview), _("Select"), COLUMN_FIXED);
    g_signal_connect (renderer,
                     "toggled",
                      G_CALLBACK (NewGroupSelectUsers),
                      win);
    create_tree_text_renderer (GTK_TREE_VIEW (treeview), _("Username"), COLUMN_NAME);
    create_tree_text_renderer (GTK_TREE_VIEW (treeview), _("User ID"), COLUMN_ID);
    gtk_grid_attach (GTK_GRID (table), vbox1, 0, 2, 2, 1);

    Hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_grid_attach (GTK_GRID (table), Hbox, 0, 3, 2, 1);
    ButtonClose = SetButtonIcon (_("Close"), "window-close");
    g_signal_connect (ButtonClose,
                     "clicked",
                      G_CALLBACK (CloseGroupWindow),
                      win);
    gtk_box_pack_start (GTK_BOX (Hbox), ButtonClose, TRUE, TRUE, 0);

    label = gtk_label_new (NULL);
    gtk_box_pack_start (GTK_BOX (Hbox), label, TRUE, TRUE, 0);
    gtk_widget_hide (label);

    ButtonConfirm = SetButtonIcon (_("Confirm"), "object-select");
    g_signal_connect (ButtonConfirm,
                     "clicked",
                      G_CALLBACK (CreateNewGroup),
                      win);
    gtk_box_pack_start (GTK_BOX (Hbox), ButtonConfirm, TRUE, TRUE, 0);
    win->priv->ButtonConfirm = ButtonConfirm;

    return vbox;
}

static GtkWidget *load_remove_group (UserGroupWindow *win)
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

    vbox1 = vbox_widget_new ();

    table = grid_widget_new ();
    gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE,0);

    Scrolled = scrolled_widget_new ();
    gtk_box_pack_start (GTK_BOX (vbox1), Scrolled, TRUE, TRUE, 0);

    model = create_tree_model ();
    win->priv->RemoveStore = GTK_LIST_STORE (model);
    treeview = gtk_tree_view_new_with_model (model);
    gtk_tree_view_set_search_column (GTK_TREE_VIEW (treeview),
                                     COLUMN_ID);
    g_object_unref (model);
    gtk_container_add (GTK_CONTAINER (Scrolled), treeview);

    win->priv->TreeRemove = treeview;
    create_tree_pixbuf_renderer (GTK_TREE_VIEW (treeview), _("Remove"), COLUMN_ICON);
    create_tree_text_renderer (GTK_TREE_VIEW (treeview), _("Group Name"), COLUMN_NAME);
    create_tree_text_renderer (GTK_TREE_VIEW (treeview), _("Group id"), COLUMN_ID);

    gtk_grid_attach (GTK_GRID (table), vbox1, 0, 0, 3, 1);

    ButtonClose = SetButtonIcon (_("Close"), "window-close");
    gtk_grid_attach (GTK_GRID (table), ButtonClose, 0, 1, 1, 1);
    g_signal_connect (ButtonClose,
                     "clicked",
                      G_CALLBACK (CloseGroupWindow),
                      win);

    ButtonRemove = SetButtonIcon (_("Remove"), "list-remove");
    gtk_grid_attach (GTK_GRID (table), ButtonRemove, 2, 1, 1, 1);
    g_signal_connect (ButtonRemove,
                     "clicked",
                      G_CALLBACK (RemoveGroup),
                      win);
    win->priv->ButtonRemove = ButtonRemove;

    return vbox;
}

static void user_group_window_update_list_store (UserGroupWindow *win)
{
    gint          i = 0;
    int           GroupNum = 0;
    UserGroup    *group;

    GroupNum = g_slist_length (win->priv->group_list);
    gtk_list_store_clear (GTK_LIST_STORE (win->priv->SwitchStore));
    gtk_list_store_clear (GTK_LIST_STORE (win->priv->RemoveStore));
    for (i = 0; i < GroupNum ; i++)
    {
        group = g_slist_nth_data (win->priv->group_list, i);
        if (group == NULL)
        {
            g_error ("No such the Group!!!\r\n");
            break;
        }
        addswitchlistdata (win->priv->TreeSwitch, win->priv->SwitchStore, group, win->priv->user_name);
        if (!user_group_is_primary_group (group) &&
            user_group_get_group_id (group) >= 1000)
            addremovelistdata (win->priv->TreeRemove, win->priv->RemoveStore, group);
    }
}

static void list_remove_data (gpointer  data,
                              gpointer  user_data)
{
    UserGroupWindow *win = USER_GROUP_WINDOW (user_data);
    UserGroup       *group = data;

    if (g_strcmp0 (win->priv->remove_group_name, user_group_get_group_name (group)) == 0)
    {
        win->priv->group_list = g_slist_remove (win->priv->group_list, group);
        g_object_unref (group);
    }
}

static void user_group_remove_cb (GasGroupManager  *g_manager,
                                  GasGroup         *gas,
                                  UserGroupWindow  *win)
{
    UserGroup  *group;
    const char *name;

    group = user_group_new (gas);
    name = user_group_get_group_name (group);

    win->priv->remove_group_name = g_strdup (name);
    g_slist_foreach (win->priv->group_list, list_remove_data, win);
    g_object_unref (group);
    user_group_window_update_list_store (win);
}

static void user_group_add_cb (GasGroupManager   *g_manager,
                               GasGroup          *gas,
                               UserGroupWindow   *win)
{
    UserGroup *group;
    add_user_to_new_group (win->priv->NewGroupUsers, gas);

    group = user_group_new (gas);
    win->priv->group_list = g_slist_append (win->priv->group_list, g_object_ref (group));
    MessageReport (_("Create User Group"),
                   _("Create User Group Successfully,Please view the end of the switch-groups list."),
                   INFOR);

    user_group_window_update_list_store (win);
    clearconfigdata (win);
}

static void user_group_window_dispose (GObject *object)
{
    UserGroupWindow *win = USER_GROUP_WINDOW (object);

    if (win->priv->user_name != NULL)
    {
        g_free (win->priv->user_name);
        win->priv->user_name = NULL;
    }
    if (win->priv->remove_group_name != NULL)
    {
        g_free (win->priv->remove_group_name);
        win->priv->remove_group_name = NULL;
    }
    if (win->priv->group_list != NULL)
    {
        g_slist_free_full (win->priv->group_list, g_object_unref);
        win->priv->group_list = NULL;
    }
    if (win->priv->NewGroupUsers != NULL)
    {
        g_slist_free (win->priv->NewGroupUsers);
        win->priv->NewGroupUsers = NULL;
    }
    if (win->priv->add_id > 0 )
    {
        g_signal_handler_disconnect (win->priv->g_manager, win->priv->add_id);
        win->priv->add_id = 0;
    }
    if (win->priv->remove_id > 0)
    {
        g_signal_handler_disconnect (win->priv->g_manager, win->priv->remove_id);
        win->priv->remove_id = 0;
    }
    g_signal_handlers_disconnect_by_func (win->priv->permission, on_permission_changed, win);
    g_signal_emit (win, signals[WINDOW_CLOSED], 0);
    G_OBJECT_CLASS (user_group_window_parent_class)->dispose (object);
}

void user_group_window_class_init (UserGroupWindowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->dispose = user_group_window_dispose;

    signals [WINDOW_CLOSED] =
         g_signal_new ("window-closed",
                       G_TYPE_FROM_CLASS (klass),
                       G_SIGNAL_RUN_LAST,
                       0,
                       NULL, NULL,
                       g_cclosure_marshal_VOID__VOID,
                       G_TYPE_NONE, 0);
}

void user_group_window_init (UserGroupWindow *group_window)
{
    GtkWidget   *button_lock;
    GError      *error = NULL;
    GtkWidget   *note_book;
    GtkWidget   *label;
    GtkWidget   *box;

    group_window->priv = user_group_window_get_instance_private (group_window);
    gtk_container_set_border_width (GTK_CONTAINER (group_window), 10);
    group_window->priv->permission = polkit_permission_new_sync (USER_GROUP_PERMISSION, NULL, NULL, &error);
    group_window->priv->NewGroupUsers = NULL;
    if (group_window->priv->permission == NULL)
    {
        mate_user_admin_log ("Warning","Cannot create '%s' permission: %s", USER_GROUP_PERMISSION, error->message);
        g_error_free (error);
    }
    button_lock = gtk_lock_button_new (group_window->priv->permission);
    gtk_lock_button_set_permission (GTK_LOCK_BUTTON (button_lock), group_window->priv->permission);
    gtk_widget_grab_focus (button_lock);
    g_signal_connect (group_window->priv->permission,
                    "notify",
                     G_CALLBACK (on_permission_changed),
                     group_window);

    note_book = gtk_notebook_new ();
    gtk_container_add (GTK_CONTAINER (group_window), note_book);
    gtk_notebook_set_tab_pos (GTK_NOTEBOOK (note_book), GTK_POS_TOP);

    label = gtk_label_new (_("Switch Groups"));
    box = load_select_group (group_window, button_lock);
    gtk_notebook_append_page (GTK_NOTEBOOK (note_book), box, label);

    label = gtk_label_new (_("Create Groups"));
    box = load_create_group (group_window);
    gtk_notebook_append_page (GTK_NOTEBOOK (note_book), box, label);

    label = gtk_label_new (_("Remove Groups"));
    box = load_remove_group (group_window);
    gtk_notebook_append_page (GTK_NOTEBOOK (note_book), box, label);
}

UserGroupWindow *user_group_window_new (const char *user_name,
                                        GSList     *user_list)
{
    UserGroupWindow  *group_window;
    g_autofree gchar *title = NULL;

    title = g_strdup_printf (_("Current User - %s"), user_name);

    group_window = g_object_new (USER_TYPE_GROUP_WINDOW,
                                "window-position", GTK_WIN_POS_CENTER,
                                 "title", title,
                                 NULL);
    group_window->priv->user_name = g_strdup (user_name);
    group_window->priv->g_manager = user_group_start_group_manager ();
    group_window->priv->group_list = user_group_get_group_list (group_window->priv->g_manager);

    group_window->priv->add_id = g_signal_connect (group_window->priv->g_manager,
                                                  "group-added",
                                                   G_CALLBACK (user_group_add_cb),
                                                   group_window);

    group_window->priv->remove_id = g_signal_connect (group_window->priv->g_manager,
                                                     "group-removed",
                                                      G_CALLBACK (user_group_remove_cb),
                                                      group_window);

    if(group_window->priv->group_list == NULL)
    {
        gtk_widget_destroy (GTK_WIDGET (group_window));
        return NULL;
    }
    update_sensitive (group_window);
    user_group_window_update_list_store (group_window);
    user_list_store_update (user_list, group_window->priv->UserStore);

    return group_window;
}
