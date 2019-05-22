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

#ifndef __USER_SHARE_H__
#define __USER_SHARE_H__

#include "user.h"

#define TYPEMSG    "<span foreground='red'font_desc='13'>%s </span>"
#define ERROR      1
#define WARING     2
#define INFOR      3
#define QUESTION   4
#define QUESTIONNORMAL   5

int Change;
void         mate_uesr_admin_log   (const char  *level,
                                    const char  *message,
                                    ...);
void         close_log_file        (void);
gboolean     GetUseHeader          (void);
GHashTable  *CreateGroupsHashTable (void);
int          MessageReport         (const char  *Title,
                                    const char  *Msg,
                                    int          nType);

void         SetLableFontType      (GtkWidget   *Lable ,
                                    const char  *Color,
                                    int          FontSzie,
                                    const char  *Word);

void         UserListAppend        (GtkWidget   *list,
                                    const gchar *UserIcon,
                                    const gchar *RealName,
                                    const gchar *UserName,
                                    int          Index,
                                    GtkTreeIter *iter);

GdkPixbuf  *SetUserFaceSize        (const char  *PicName, 
                                    int          Size);

void        OpenNote               (GtkWidget   *label,
                                    const char  *note,
                                    UserAdmin   *ua);

void        OffNote                (GtkWidget   *label,
                                    UserAdmin   *ua);


GtkWidget  *SetComboUserType       (const char  *s1,
                                    const char  *s2);

void        UpdateInterface        (ActUser     *ActUser,
                                    UserAdmin   *ua);

gboolean    CheckPassword          (gpointer     data);
void        SelectSetPassMode      (GtkRadioButton *button,
                                    gpointer     user_data);

void        AutoGenera             (GtkEntry            *entry,
                                    GtkEntryIconPosition icon_pos,
                                    GdkEvent            *event,
                                    gpointer             user_data);

int         GetPassStrength        (const char  *password,
                                    const char  *old_password,
                                    const char  *username,
                                    const char **message);

#endif
