/*************************************************************************
  File Name: user-window.h
  
  Copyright (C) 2020  zhuyaliang https://github.com/zhuyaliang/
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
                                      
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
                                               
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
                                               
  Created Time: 2021年03月12日 星期五 17时02分17秒
 ************************************************************************/

#ifndef __USER_WINDOW__
#define __USER_WINDOW__

#include <gtk/gtk.h>
#include <act/act.h>

#define USER_TYPE_WINDOW         (user_window_get_type ())
#define USER_WINDOW(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), USER_TYPE_WINDOW, UserWindow))
#define USER_WINDOW_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), USER_TYPE_WINDOW, UserWindowClass))
#define USER_IS_WINDOW(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), USER_TYPE_WINDOW))
#define USER_WINDOW_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), USER_TYPE_WINDOW, UserWindowClass))

typedef struct _UserWindow        UserWindow;
typedef struct _UserWindowClass   UserWindowClass;
typedef struct _UserWindowPrivate UserWindowPrivate;

struct _UserWindow {
    GtkWindow           parent_instance;
    UserWindowPrivate  *priv;
};

struct _UserWindowClass {
    GtkWindowClass parent_class;
};

GType         user_window_get_type         (void) G_GNUC_CONST;

UserWindow   *user_window_new              (ActUserManager *manager);

void          user_window_remove_user_cb   (ActUserManager *um,
                                            ActUser        *user,
                                            UserWindow     *win);

void          user_window_add_user_cb      (ActUserManager *um,
                                            ActUser        *user,
                                            UserWindow     *win);

#endif
