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

#ifndef __USER_ADMIN_H__
#define __USER_ADMIN_H__

#include <gtk/gtk.h>
#include <act/act.h>
G_BEGIN_DECLS

#define USER_TYPE_MANAGER         (user_manager_get_type ())
#define USER_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), USER_TYPE_MANAGER, UserManager))
#define USER_MANAGER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), USER_TYPE_MANAGER, UserManagerClass))

typedef struct _UserManager        UserManager;
typedef struct _UserManagerClass   UserManagerClass;
typedef struct _UserManagerPrivate UserManagerPrivate;

struct _UserManager
{
    GtkDialog          parent_instance;
    UserManagerPrivate  *priv;
};

struct _UserManagerClass {
    GtkDialogClass parent_class;
};

GType          user_manager_get_type                 (void) G_GNUC_CONST;

UserManager   *user_manager_new                      (void);

void  RemoveUser(ActUser *user);
void  AddNewUser(GtkWidget *widget, gpointer data);
G_END_DECLS
#endif
