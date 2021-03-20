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

#ifndef __USER_HISTORY_H__
#define __USER_HISTORY_H__

#include <gtk/gtk.h>
#include <act/act.h>

G_BEGIN_DECLS

#define TYPE_LOGIN_HISTORY        (login_history_dialog_get_type ())
#define LOGINHISTORY(object)      (G_TYPE_CHECK_INSTANCE_CAST ((object), \
                                   TYPE_LOGIN_HISTORY, \
                                   LoginHistoryDialog))

typedef struct LoginHistoryDialog
{
    GtkDialog     parent_instance;

    GtkHeaderBar *header_bar;
    GtkListBox   *history_box;
    GtkButton    *next_button;
    GtkButton    *previous_button;

    GDateTime    *week;
    GDateTime    *current_week;

    ActUser      *Actuser;
    int           is_header;
}LoginHistoryDialog;
typedef struct LoginHistoryDialogClass
{
    GtkDialogClass parent_class;
}   LoginHistoryDialogClass;

GType               login_history_dialog_get_type          (void) G_GNUC_CONST;

LoginHistoryDialog *login_history_dialog_new               (ActUser *user);

G_END_DECLS
#endif
