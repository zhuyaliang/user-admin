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
#ifndef __USER_GROUP__
#define __USER_GROUP__

#include "user.h"
#include <libgroupservice/gas-group.h>
#include <libgroupservice/gas-group-manager.h>
G_BEGIN_DECLS

#define USER_TYPE_GROUP     (user_group_get_type ())
#define USERGROUP(object)   (G_TYPE_CHECK_INSTANCE_CAST ((object), USER_TYPE_GROUP, UserGroup))
#define IS_GROUP(object)    (G_TYPE_CHECK_INSTANCE_TYPE ((object), USER_TYPE_GROUP))

typedef struct UserGroup 
{
	GObject       parent;
    GasGroup     *gas;
    gid_t         GroupId;
    gchar        *GroupName;
}UserGroup;
typedef struct UserGroupClass
{
	GObjectClass parent_class;
} UserGroupClass;

GType          user_group_get_type           (void) G_GNUC_CONST;

UserGroup *    group_new                     (const char *GroupName,
				                              gid_t       gid);

void           UserGroupsManage              (GtkWidget  *widget, 
                                              gpointer    data);
G_END_DECLS

#endif
