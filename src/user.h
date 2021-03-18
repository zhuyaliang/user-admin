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

#ifndef __USER_H__
#define __USER_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtk/gtk.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pwd.h>
#include <sys/types.h>
#include <time.h>
#include <sys/time.h>
#include <libintl.h>
#include <locale.h>
#include <glib/gi18n.h>
#define MATE_DESKTOP_USE_UNSTABLE_API
#include <libmate-desktop/mate-languages.h>
#include <libmate-desktop/mate-desktop-thumbnail.h>

#include <act/act-user-manager.h>
#include <act/act-user.h>
#include <polkit/polkit.h>
#include "user-language.h"
#include "user-face.h"
#include "user-base.h"

#define  AVATARS    "Default.png"
#define  DEFAULT   FACEDIR AVATARS 
#define  PICMAX    20    
#define  NONE      3

#define  OLDPASS   0
#define  NEWPASS   1

#define  STANDARD  0
#define  ADMIN     1

enum
{
    COL_USER_FACE= 0,
    INT_COLUMN,
    LIST_LABEL ,
    N_COLUMNS
};
typedef struct 
{
    GSList           *UsersList;
    UserFace         *face;
    UserBase         *base;
    GtkWidget        *MainWindow;
    GtkWidget        *IconWindow;
    GtkWidget        *PasswordDialog;
    ActUserManager   *um;
	ActUser          *CurrentUser;
	GtkWidget        *CurrentImage;
	GtkWidget        *CurrentName;
	GtkWidget        *ButtonLock;
	GtkWidget        *ButtonRemove;
	GtkWidget        *ButtonAdd;
	GtkWidget        *Popover;
    GPermission      *Permission;
    GtkWidget        *UserList;
}UserAdmin;
extern GtkWidget  *WindowLogin;          //首页窗口
#endif
