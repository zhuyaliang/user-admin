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
G_BEGIN_DECLS

#define ADD_NU_TYPE_DIALOG     (add_nu_dialog_get_type ())
#define ADDNUDIALOG(obj)       (G_TYPE_CHECK_INSTANCE_CAST ((obj), ADD_NU_TYPE_DIALOG, AddNUDialog))
#define ADD_NU_IS_DIALOG(obj)  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ADD_NU_TYPE_DIALOG))

typedef struct AddNUDialog 
{
	GtkDialog         parent;
    GCancellable     *cancellable;
	GPermission      *permission;
    GtkWidget        *RealNameEntry;
    GtkWidget        *UserNameEntry;
    GtkWidget        *LabelNameNote;
    GtkWidget        *NewUserType;
    GtkWidget        *RadioButton1;
    GtkWidget        *RadioButton2;
    GtkWidget        *NewPassEntry;
    GtkWidget        *LevelBar;
    GtkWidget        *LabelPassNote;
    GtkWidget        *CheckPassEntry;
    GtkWidget        *LabelSpace;
    int               CheckPassTimeId;       //Check the password format timer
    int               CheckNameTimeId;       //Check the Realname format timer
    GtkWidget        *ButtonCancel;
    GtkWidget        *ButtonConfirm;
    GtkTreeIter       NewUserIter;           //user list iter
	char             *nuLang;
	char            **nuGroups;
	gboolean          nuType;
}AddNUDialog; 
typedef struct AddNUDialogClass 
{
	GtkDialogClass parent_class;
}AddNUDialogClass;

GType        add_nu_dialog_get_type (void) G_GNUC_CONST;
AddNUDialog *Add_NUDialog_new (void);
void  RemoveUser(GtkWidget *widget, gpointer data);
void  AddNewUser(GtkWidget *widget, gpointer data);
void AddRemoveUser(GtkWidget *Vbox, gpointer data);
G_END_DECLS
#endif
