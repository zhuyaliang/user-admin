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

#ifndef __USER_LANGUAGE_H__
#define __USER_LANGUAGE_H__

#include <gtk/gtk.h>
#include <act/act.h>

G_BEGIN_DECLS

#define USER_TYPE_LANGUAGE         (user_language_get_type ())
#define USER_LANGUAGE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), USER_TYPE_LANGUAGE, UserLanguage))
#define USER_LANGUAGE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), USER_TYPE_LANGUAGE, UserLanguageClass))

typedef struct _UserLanguage        UserLanguage;
typedef struct _UserLanguageClass   UserLanguageClass;
typedef struct _UserLanguagePrivate UserLanguagePrivate;

struct _UserLanguage {
    GtkDialog             parent_instance;
    UserLanguagePrivate  *priv;
};

struct _UserLanguageClass {
    GtkDialogClass parent_class;
};

GType            user_language_get_type        (void) G_GNUC_CONST;

UserLanguage    *user_language_new             (ActUser         *user);

void             user_language_clear_filter    (UserLanguage    *chooser);

const gchar     *user_language_get_language    (UserLanguage    *chooser);

void             user_language_set_language    (UserLanguage    *chooser,
                                                const gchar     *language);
G_END_DECLS
#endif
