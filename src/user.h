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
#define MATE_DESKTOP_USE_UNSTABLE_API
#include <mate-desktop-2.0/libmate-desktop/mate-languages.h>
#include <mate-desktop-2.0/libmate-desktop/mate-desktop-thumbnail.h>

#include <accountsservice-1.0/act/act-user-manager.h>
#include <accountsservice-1.0/act/act-user.h>
#include <libintl.h>   
#include <locale.h>   
#include <polkit/polkit.h>
#include "user-language.h"

#define  AVATARS    "Default.png"
#define  DEFAULT   FACEDIR AVATARS 
#define  PICMAX    20    
#define  NONE      3

#define  OLDPASS   0
#define  NEWPASS   1

#define  STANDARD  0
#define  ADMIN     1
#define _(STRING)  gettext(STRING)   

enum
{
    COL_USER_FACE= 0,
    INT_COLUMN,
    LIST_LABEL ,
    N_COLUMNS
};
typedef struct
{
    GSList           *GroupsList;
    GtkWidget        *GroupsWindow;
    GtkWidget        *NoteBook;
    GtkWidget        *SwitchBox;
    GtkWidget        *CreateBox;
    GtkWidget        *RemoveBox;
    GtkWidget        *ButtonConfirm;
    GtkWidget        *ButtonRemove;
    GtkWidget        *TreeSwitch;
    GtkWidget        *TreeCreate;
    GtkWidget        *TreeRemove;
    GtkListStore     *SwitchStore;
    GtkListStore     *RemoveStore;
	GtkWidget        *EntryGroupName;
	GtkWidget        *ButtonLock;
    GPermission      *Permission;
    int               GroupNum;
    const gchar      *username;
    GSList           *NewGroupUsers;          
}GroupsManage;

typedef struct
{
    GtkWidget        *AddUserDialog;
    GtkWidget        *ButtonCancel;
    GtkWidget        *ButtonConfirm;
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
    GtkTreeIter       NewUserIter;           //user list iter
	char             *nuLang;
	char            **nuGroups;
	gboolean          nuType;
}CreateUser;
typedef struct 
{
    GSList           *UsersList;
    GroupsManage      gm;
    LanguageChooser  *language_chooser;
    CreateUser        newuser;
    GtkWidget        *MainWindow;
    GtkWidget        *IconWindow;
    GtkWidget        *PasswordDialog;
    ActUserManager   *um;
	GtkWidget        *ButtonLock;
    GPermission      *Permission;
    GtkTreeModel     *Model;
    GtkTreeSelection *UserSelect;
    GtkWidget        *UserList;
    GtkListStore     *ListSTore;
    GtkWidget        *ButtonFace;
    GtkWidget        *EntryName;
    GtkWidget        *ComUserType;
    GtkWidget        *ButtonLanguage;
    GtkWidget        *ButtonPass;
    GtkWidget        *SwitchAutoLogin;
    GtkWidget        *ButtonUserTime;
    GtkWidget        *ButtonUserGroup;
    GtkWidget        *NewPassEntry;
    GtkWidget        *CheckPassEntry;
    GtkWidget        *LabelPassNote;
    GtkWidget        *LevelBar;
    GtkWidget        *ButtonConfirm;
    GtkWidget        *LabelSpace;
    GtkWidget        *RadioButton1;
    GtkWidget        *RadioButton2;
    GtkWidget        *CropArea;
    GtkWidget        *ButtonRemove;
    GtkWidget        *ButtonAdd;
    int               CheckPassTimeId;//Check the password format timer
    int               CheckNameTimeId; //Check the Realname format timer
    int               UserCount;             //Valid user number
    int               MouseId;
    int               KeyId;
    char              TmpPass[24];
}UserAdmin;
int         gnCnt;                //计数
int         gnCurrentUserIndex;   //代表当前用户标号
GtkWidget  *WindowLogin;          //首页窗口
#endif
