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
#include "user-group.h"
#include <pwd.h>

struct _UserGroupPrivate
{
    GasGroup  *gas;
    gid_t      gid ;
    gchar     *gname;
};

G_DEFINE_TYPE_WITH_PRIVATE (UserGroup, user_group, G_TYPE_OBJECT)
static void group_finalize (GObject *object)
{
    UserGroup *group;

    group = USER_GROUP (object);
    if (group->priv->gname != NULL)
    {
        g_free (group->priv->gname);
        group->priv->gname = NULL;
    }
    g_clear_object (&group->priv->gas);
}

static void user_group_class_init (UserGroupClass *class)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS (class);
    gobject_class->finalize = group_finalize;

}

static void user_group_init (UserGroup *group)
{
    group->priv = user_group_get_instance_private (group);
}

UserGroup *user_group_new (GasGroup *gas)
{
    UserGroup  *group;
    const char *name = NULL;
    gid_t       gid;

    name = gas_group_get_group_name (gas);
    gid  = gas_group_get_gid (gas);

    group = g_object_new (USER_TYPE_GROUP, NULL);

    group->priv->gas = g_object_ref (gas);
    group->priv->gname = g_strdup (name);
    group->priv->gid = gid;

    return group;
}

gid_t user_group_get_group_id (UserGroup *group)
{
    g_return_val_if_fail (USER_IS_GROUP (group), -1);
    return group->priv->gid;
}

const char *user_group_get_group_name (UserGroup *group)
{
    g_return_val_if_fail (USER_IS_GROUP (group), NULL);
    return group->priv->gname;
}

void user_group_add_user_to_group (UserGroup  *group,
                                   const char *user)
{
    g_return_if_fail (USER_IS_GROUP (group));
    g_return_if_fail (user != NULL);

    gas_group_add_user_group (group->priv->gas, user);
}

void user_group_remove_user_from_group (UserGroup  *group,
                                        const char *user)
{
    g_return_if_fail (USER_IS_GROUP (group));
    g_return_if_fail (user != NULL);

    gas_group_remove_user_group (group->priv->gas, user);
}

gboolean user_group_remove_group (GasGroupManager *ggm,
                                  UserGroup       *group,
                                  GError         **error)
{
    g_return_val_if_fail (GAS_IS_GROUP_MANAGER (ggm), FALSE);
    g_return_val_if_fail (USER_IS_GROUP (group), FALSE);

    return gas_group_manager_delete_group(ggm,
                                          group->priv->gas,
                                          error);
}

gboolean user_group_user_is_group (UserGroup  *group,
                                  const gchar *user)
{
    g_return_val_if_fail (USER_IS_GROUP (group), FALSE);
    g_return_val_if_fail (user != NULL, FALSE);

    return gas_group_user_is_group (group->priv->gas, user);
}

gboolean user_group_is_primary_group (UserGroup *group)
{
    g_return_val_if_fail (USER_IS_GROUP (group), FALSE);

    return gas_group_is_primary_group (group->priv->gas);
}

void add_user_to_new_group(GSList *list, GasGroup *gas)
{
    GSList     *node;
    const char *name;

    for(node = list; node; node = node->next)
    {
        name = node->data;
        gas_group_add_user_group (gas, name);
    }
}
