/*   mate-user-admin 
*   Copyright (C) 2021  zhuyaliang https://github.com/zhuyaliang/
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

typedef struct _UserListRow        UserListRow;
typedef struct _UserListRowClass   UserListRowClass;
typedef struct _UserListRowPrivate UserListRowPrivate;

typedef struct _UserListRow
{
    GtkListBoxRow   parent_instance;
    UserListRowPrivate  *priv;

}UserListRow;

typedef struct _UserListRowClass
{
    GtkListBoxRowClass   parent_instance_class;

}UserListRowClass;

GType             user_list_row_get_type                (void) G_GNUC_CONST;

GtkWidget        *user_list_row_new                     (ActUser     *user);

void              user_list_row_set_data                (UserListRow *Row);

GtkWidget        *user_list_row_get_image_label         (UserListRow *row);

GtkWidget        *user_list_row_get_name_label          (UserListRow *row);

ActUser          *user_list_row_get_user                (UserListRow *row);

void              user_list_box_update                  (GtkWidget   *list_box,
                                                         GSList      *user_list);

G_END_DECLS
#endif
