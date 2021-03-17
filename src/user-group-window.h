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

#ifndef __USER_GROUP_WINDOW_H__
#define __USER_GROUP_WINDOW_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define USER_TYPE_GROUP_WINDOW         (user_group_window_get_type ())
#define USER_GROUP_WINDOW(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), USER_TYPE_GROUP_WINDOW, UserGroupWindow))
#define USER_GROUP_WINDOW_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), USER_TYPE_GROUP_WINDOW, UserGroupWindowClass))

typedef struct _UserGroupWindow        UserGroupWindow;
typedef struct _UserGroupWindowClass   UserGroupWindowClass;
typedef struct _UserGroupWindowPrivate UserGroupWindowPrivate;

struct _UserGroupWindow {
    GtkWindow             parent_instance;
    UserGroupWindowPrivate  *priv;
};

struct _UserGroupWindowClass {
    GtkWindowClass parent_class;
};

GType               user_group_window_get_type        (void) G_GNUC_CONST;

UserGroupWindow    *user_group_window_new             (const char *user_name,
                                                       GSList     *user_list);

void                UserGroupsManage (const char *user_name, GSList *user_list);
G_END_DECLS
#endif
