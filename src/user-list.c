#include "user-list.h"
#include "user-admin.h"
#include "user-share.h"

/******************************************************************************
* 函数:              on_changed
*
* 说明:  选择用户信号相应函数
*
* 输入:
*
*
* 返回:
*
* 作者:  zhuyaliang  09/05/2018
******************************************************************************/
static void  on_changed(GtkWidget *widget,  gpointer data)
{
    UserAdmin *ua = (UserAdmin *) data;
    GtkTreeIter iter;
    gint count = 0;

    if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(widget), &ua->Model, &iter))
    {
//        gtk_tree_model_get(ua->Model, &iter, LIST_TEXT, &value, -1);
        /*Get the user line number*/
        gtk_tree_model_get (ua->Model, &iter,
                            INT_COLUMN, &count,
                            -1);
        gnCurrentUserIndex = count;
        UpdateInterface(count,ua);
    }
}

/******************************************************************************
* 函数:              ListModelCreate        
*        
* 说明:  创建用户列表用户列表
*        
* 输入:  		
*        
*        
* 返回:  列表
*        
* 作者:  zhuyaliang  09/05/2018
******************************************************************************/ 
static GtkTreeModel  *ListModelCreate(UserAdmin *ua)
{   

    GtkListStore *store;
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
* 函数:              ListViewInit       
*        
* 说明:  初始化用户列表
*        
* 输入:  		
*        列表
*        
* 返回:  
*        
* 作者:  zhuyaliang  09/05/2018
******************************************************************************/ 
static void ListViewInit(GtkWidget *list)
{
    GtkCellRenderer *renderer_icon,*renderer_text;
    GtkTreeViewColumn *column;
    column=gtk_tree_view_column_new ();

    gtk_tree_view_column_set_title(column,_("User List"));
    
    renderer_icon = gtk_cell_renderer_pixbuf_new();   //user icon
    renderer_text = gtk_cell_renderer_text_new();     //user name text

    gtk_tree_view_column_pack_start (column, renderer_icon, FALSE);
    gtk_tree_view_column_set_attributes (column, renderer_icon,"pixbuf", COL_USER_FACE,NULL);

    gtk_tree_view_column_pack_start(column,renderer_text,FALSE);
    gtk_tree_view_column_add_attribute(column,renderer_text,"text",LIST_TEXT);
    gtk_tree_view_column_add_attribute(column,renderer_text,"foreground",LIST_COLOR);
    gtk_tree_view_column_add_attribute(column,renderer_text,"weight",LIST_FRONT);
    gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);
}

/******************************************************************************
* 函数:              DisplayUserList        
*        
* 说明:  在左侧显示所有用户的名字头像。显示用户列表
*        
* 输入:  		
*        @Hbox     左侧布局盒
*        @UserCnt  用户个数
*        
* 返回:  
*        
* 作者:  zhuyaliang  09/05/2018
******************************************************************************/ 
void DisplayUserList(GtkWidget *Hbox,UserAdmin *ua)
{
    GtkWidget *table;
    GtkTreeSelection *selection;
    GtkWidget *UserList;
    GtkWidget *Scrolled;
    GtkTreeModel *model;
    int i;
    
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
    for( i = 0; i < ua->UserCount; i ++)
    {
        UserListAppend(UserList,
                       ua->ul[i].UserIcon,
                       ua->ul[i].RealName,
                       "black",
                       i,
                       &ua->ul[i].Iter);
    } 
    
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
void AddRemoveUser(GtkWidget *Vbox,UserAdmin *ua)
{
    GtkWidget *ButtonRemove;
    GtkWidget *ButtonAdd;
    GtkWidget *LableSpace;
    GtkWidget *table;
    
    table = gtk_grid_new();
    gtk_grid_set_column_homogeneous(GTK_GRID(table),TRUE);
    gtk_box_pack_start(GTK_BOX(Vbox),table, TRUE, TRUE,0);

    LableSpace = gtk_label_new(NULL);
    gtk_grid_attach(GTK_GRID(table) , LableSpace , 0 , 0 , 4 , 1);
    ButtonAdd    =  gtk_button_new_with_label(_("Add User"));
    ButtonRemove =  gtk_button_new_with_label(_("Remove User"));

    gtk_grid_attach(GTK_GRID(table) , ButtonRemove , 0 , 1 , 1 , 1);
    gtk_grid_attach(GTK_GRID(table) , ButtonAdd , 4 , 1 , 1 , 1);
    g_signal_connect (ButtonRemove, "clicked",G_CALLBACK (RemoveUser),ua);
    g_signal_connect (ButtonAdd, "clicked",G_CALLBACK (AddUser),ua);
    
    gtk_grid_set_row_spacing(GTK_GRID(table), 10);
    gtk_grid_set_column_spacing(GTK_GRID(table), 10);

}  

