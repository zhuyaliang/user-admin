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

#include <gtk/gtk.h>
#include <sys/types.h>
#include <grp.h>
#include <libgroupservice/gas-group.h>
#include <libgroupservice/gas-group-manager.h>
G_BEGIN_DECLS

#define USER_TYPE_GROUP         (user_group_get_type ())
#define USER_GROUP(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), USER_TYPE_GROUP, UserGroup))
#define USER_GROUP_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), USER_TYPE_GROUP, UserGroupClass))
#define USER_IS_GROUP(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), USER_TYPE_GROUP))

typedef struct _UserGroup        UserGroup;
typedef struct _UserGroupClass   UserGroupClass;
typedef struct _UserGroupPrivate UserGroupPrivate;

struct _UserGroup {
    GObject          parent_instance;
    UserGroupPrivate  *priv;
};

struct _UserGroupClass {
    GObjectClass parent_class;
};

GType          user_group_get_type                (void) G_GNUC_CONST;

UserGroup     *user_group_new                     (GasGroup        *gas);

gboolean       user_group_remove_group            (GasGroupManager *ggm,
                                                   UserGroup       *group,
                                                   GError         **error);

gboolean       user_group_user_is_group           (UserGroup       *group,
                                                   const gchar     *user);

gid_t          user_group_get_group_id            (UserGroup       *group);

const char    *user_group_get_group_name          (UserGroup       *group);

void           user_group_add_user_to_group       (UserGroup       *group,
                                                   const char      *user);

void           user_group_remove_user_from_group  (UserGroup       *group,
                                                   const char      *user);

gboolean       user_group_is_primary_group        (UserGroup       *group);
G_END_DECLS

#endif
