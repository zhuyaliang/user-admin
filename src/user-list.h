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

#ifndef __USER_LIST_H__
#define __USER_LIST_H__

#include "user.h"
G_BEGIN_DECLS
    
#define USER_LIST_TYPE_ROW                (user_list_row_get_type ())
#define USER_LIST_ROW(object)             (G_TYPE_CHECK_INSTANCE_CAST ((object),\
                                           USER_LIST_TYPE_ROW,\
                                           UserListRow))
    
typedef struct UserListRow
{   
    GtkListBoxRow   parent_instance;
    
    ActUser        *Actuser;
    GtkWidget      *user_image;
    GtkWidget      *user_name;
    GtkWidget      *real_name;

}UserListRow;

typedef struct UserListRowClass
{
    GtkListBoxRowClass   parent_instance_class;

}UserListRowClass;

GType             user_list_row_get_type                (void) G_GNUC_CONST;

GtkWidget        *user_list_row_new                     (ActUser     *Actuser);

void              user_list_row_set_data                (UserListRow *Row);

void              init_user_option_data                 (UserAdmin   *ua);
void RefreshUserList(GtkWidget *UserList,GSList *List);
void DisplayUserList(GtkWidget *Hbox,UserAdmin *ua);
void AddRemoveUser(GtkWidget *Vbox,UserAdmin *ua);
G_END_DECLS
#endif
