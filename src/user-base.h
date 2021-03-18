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

#ifndef __USER_BASE_H__
#define __USER_BASE_H__

#include <act/act.h>
#include <gtk/gtk.h>

#define USER_TYPE_BASE         (user_base_get_type ())
#define USER_BASE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), USER_TYPE_BASE, UserBase))
#define USER_BASE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), USER_TYPE_BASE, UserBaseClass))

typedef struct _UserBase        UserBase;
typedef struct _UserBaseClass   UserBaseClass;
typedef struct _UserBasePrivate UserBasePrivate;

struct _UserBase {
    GtkGrid           parent_instance;
    UserBasePrivate  *priv;
};

struct _UserBaseClass {
    GtkGridClass parent_class;
};

GType         user_base_get_type                 (void) G_GNUC_CONST;

UserBase     *user_base_new                      (ActUser  *user);

void          user_base_set_user                 (UserBase *base,
                                                  ActUser  *user);

void          user_base_update_user_info         (UserBase *base,
                                                  ActUser  *user);

void          user_base_set_public_sensitive     (UserBase *base,
                                                  gboolean  sensitive);

void          user_base_set_private_sensitive    (UserBase *base,
                                                  gboolean  sensitive);
#endif
