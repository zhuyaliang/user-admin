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

#ifndef __USER_INFO_H__
#define __USER_INFO_H__

#include "user.h"
G_BEGIN_DECLS

#define USER_TYPE_INFO       (user_info_get_type ())
#define USERINFO(object)     (G_TYPE_CHECK_INSTANCE_CAST ((object), \
                                                           USER_TYPE_INFO,\
                                                           UserInfo))

#define IS_USERINFO(object)  (G_TYPE_CHECK_INSTANCE_TYPE ((object),\
                                                           USER_TYPE_INFO))


typedef struct UserInfo 
{
    GObject      parent;
    ActUser     *ActUser;
    gchar       *UserName;
    int          PasswordType;   //passwod type
    GtkTreeIter  Iter;           //user list iter
}UserInfo;
typedef struct UserInfoClass
{
    GObjectClass parent_class;
} UserInfoClass;

GType          user_info_get_type                (void) G_GNUC_CONST;
UserInfo *     user_new                     (void);
int GetUserInfo(UserAdmin *ua);
const gchar *GetPasswordModeText (ActUser *user,int *Type);
const gchar *GetLoginTimeText (ActUser *user);
const gchar *GetRealName (ActUser *user);
const gchar *GetUserName(ActUser *user);
const gchar *GetHomeName(ActUser *user);
const gchar *GetUserIcon(ActUser *user);
const gchar *GetUserLang(ActUser *user);
UserInfo * GetIndexUser(UserAdmin *ua,guint index);
gint  GetUserType(ActUser *user);
gint  GetUserAutoLogin(ActUser *user);
G_END_DECLS

#endif
