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

#ifndef __USER_FACE_H__
#define __USER_FACE_H__

#include <gtk/gtk.h>
#include <act/act.h>

#define USER_TYPE_FACE         (user_face_get_type ())
#define USER_FACE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), USER_TYPE_FACE, UserFace))
#define USER_FACE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), USER_TYPE_FACE, UserFaceClass))

typedef struct _UserFace        UserFace;
typedef struct _UserFaceClass   UserFaceClass;
typedef struct _UserFacePrivate UserFacePrivate;

struct _UserFace {
    GtkGrid          parent_instance;
    UserFacePrivate *priv;
};

struct _UserFaceClass {
    GtkGridClass parent_class;
};

GType         user_face_get_type                 (void) G_GNUC_CONST;

UserFace     *user_face_new                      (void);

char         *user_face_get_image_name           (UserFace   *face);

char         *user_face_get_real_name            (UserFace   *face);

void          user_face_update                   (UserFace   *face,
                                                  const char *image_name,
                                                  const char *real_name);

void          user_face_fill                     (UserFace   *face,
                                                  ActUser    *user);

#endif
