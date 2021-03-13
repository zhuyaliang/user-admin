/*************************************************************************
  File Name: user-avatar.h
  
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
                                               
  Created Time: 2021年03月13日 星期五 20时27分17秒
 ************************************************************************/

#ifndef __USER_AVATAR__
#define __USER_AVATAR__

#include <gtk/gtk.h>
#include <act/act.h>

#define USER_TYPE_AVATAR         (user_avatar_get_type ())
#define USER_AVATAR(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), USER_TYPE_AVATAR, UserAvatar))
#define USER_AVATAR_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), USER_TYPE_AVATAR, UserAvatarClass))

typedef struct _UserAvatar        UserAvatar;
typedef struct _UserAvatarClass   UserAvatarClass;
typedef struct _UserAvatarPrivate UserAvatarPrivate;

struct _UserAvatar {
    GtkPopover          parent_instance;
    UserAvatarPrivate  *priv;
};

struct _UserAvatarClass {
    GtkPopoverClass parent_class;
};

GType         user_avatar_get_type                 (void) G_GNUC_CONST;
 
UserAvatar   *user_avatar_new                      (GtkWidget      *button);

char         *user_avatar_get_file_name            (UserAvatar     *avatar);

void          user_avatar_popup_button_clicked     (GtkWidget      *button,
                                                    UserAvatar     *avatar);

gboolean      user_avatar_popup_button_pressed     (GtkWidget      *button,
                                                    GdkEventButton *event,
                                                    UserAvatar     *avatar);
#endif
