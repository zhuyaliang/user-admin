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

#include "user-info.h"
#include "user.h"
#include "user-share.h"

G_DEFINE_TYPE (UserInfo, user_info, G_TYPE_OBJECT)
static void user_finalize (GObject *object)
{
    UserInfo *user;

    user = USERINFO (object);
    g_free (user->UserName);
}
static void user_info_class_init (UserInfoClass *class)
{
    GObjectClass *gobject_class;
    gobject_class = G_OBJECT_CLASS (class);
    gobject_class->finalize = user_finalize;
}
static void user_info_init (UserInfo *user)
{
    user->ActUser = NULL;
}
UserInfo * user_new (void)
{
    UserInfo *user;

    user = g_object_new (USER_TYPE_INFO, NULL);
    return user;
}

/******************************************************************************
* Function:              GetPasswordModeText      
*        
* Explain: Obtain the form of cryptographic display
*        
* Input: @Type  user password type;Already set dispaly ●●●●●●
*               or not set dispaly Set up next time 
*        
* Output:   
*        
* Author:  zhuyaliang  15/05/2018
******************************************************************************/ 
const gchar * GetPasswordModeText (ActUser *user,int *Type)
{
    const gchar *text;

    if (act_user_get_locked (user)) 
    {
        text = (_("Account is disable"));
        *Type = NEWPASS;
    }
    else
    {
        switch (act_user_get_password_mode (user)) 
        {
            case ACT_USER_PASSWORD_MODE_REGULAR:
                text = ("●●●●●●");
                *Type = OLDPASS;
                    break;
            case ACT_USER_PASSWORD_MODE_SET_AT_LOGIN:
                text = (_("Set up next time"));
                *Type = NEWPASS;
                    break;
            case ACT_USER_PASSWORD_MODE_NONE:
                text = ("None");
                *Type = NEWPASS;
                    break;
                default:
                    g_assert_not_reached ();
        }
    }

        return text;
}
const gchar * GetLoginTimeText (ActUser *user)
{
    gchar *text;
    GDateTime *date_time;
    gint64 time;
    time = act_user_get_login_time (user);
    if (act_user_is_logged_in (user)) 
    {
        text = g_strdup ("Logged in");
    }
    else if (time > 0) 
    {
        date_time = g_date_time_new_from_unix_local (time);
        text = g_date_time_format (date_time, ("%m/%d %k:%M"));
        g_date_time_unref (date_time);
    }
    else 
    {
        text = g_strdup ("—");
    }
    return text;
}

const gchar *GetRealName (ActUser *user)
{
    const gchar *name = NULL;
    name = act_user_get_real_name (user);
    if(name == NULL)
        name = act_user_get_user_name (user);
    return name;
}
guint GetUserUid(ActUser *user)
{
    return act_user_get_uid(user);
}    

const gchar *GetUserName(ActUser *user)
{
    const gchar *name =NULL; 
    name = act_user_get_user_name (user);
    return name;

}        
const gchar *GetHomeName(ActUser *user)
{
    const gchar *name = NULL;
    name = act_user_get_home_dir (user);
    
    return name;

}        
const gchar *GetUserIcon(ActUser *user)
{
    const gchar *Icon = NULL;
    Icon = act_user_get_icon_file (user);
    if(access(Icon,F_OK)== 0)
        return Icon;
    return DEFAULT;
}        
const char *GetUserLang(ActUser *user)
{
    const gchar *Lang = NULL;

    Lang = act_user_get_language(user);
    if(strlen(Lang) <= 0)
        return NULL;
    return Lang;
}
gint GetUserType(ActUser *user)
{
    ActUserAccountType UserType;

    UserType = act_user_get_account_type (user);
    if(UserType == ACT_USER_ACCOUNT_TYPE_ADMINISTRATOR)
        return ADMIN;

    return STANDARD;
}        
gint GetUserAutoLogin(ActUser *user)
{
    int Auto;

    Auto = act_user_get_automatic_login(user);

    return Auto;
}       
UserInfo * GetIndexUser(GSList *UsersList,guint index)
{
    if(g_slist_length(UsersList) <= index)
    {
        return NULL;
    }
    return g_slist_nth_data(UsersList,index); 
}    
static gint SortUsers (gconstpointer a, gconstpointer b)
{
    ActUser *ua, *ub;
    gchar *name1, *name2;
    gint result;

    ua = ACT_USER (a);
    ub = ACT_USER (b);

    if (act_user_get_uid (ua) == getuid ())
    {
        result = -G_MAXINT32;
    }
    else if (act_user_get_uid (ub) == getuid ())
    {
        result = G_MAXINT32;
    }
    else
    {
        name1 = g_utf8_collate_key (GetRealName (ua), -1);
        name2 = g_utf8_collate_key (GetRealName (ub), -1);
        result = strcmp (name1, name2);
        g_free (name1);
        g_free (name2);
    }
    return result;
}
/******************************************************************************
* Function:              UserAdded      
*        
* Explain: Save user information
*        
* Input:  @index  user label      
*        
*        
* Output:  
*        
* Author:  zhuyaliang  15/05/2018
******************************************************************************/
static UserInfo *UserAdded(ActUser *ActUser)
{
    UserInfo *user;

    user = user_new();
    user->ActUser = ActUser;

    /*user name Cannot be modified*/
    user->UserName  =  g_strdup(GetUserName(ActUser));
    if(user->UserName == NULL)
        return NULL;
    
    return user;
}    

/******************************************************************************
* Function:              GetUserInfo      
*        
* Explain: Using accountsservice-0.6.49 services to get user information. 
*          Information includes: name, icon, user type, language...
*        
* Input:         
*        
*        
* Output:  Success       :0
*          failure       :-1
*        
* Author:  zhuyaliang  09/05/2018
******************************************************************************/
int GetUserInfo(UserAdmin *ua)
{
    GSList *list, *l;
    int UserCnt = 0;
    UserInfo *user;
    
    /* get all user list */
    list = act_user_manager_list_users (ua->um);
    /*user number*/
    UserCnt = g_slist_length (list);     
    if(UserCnt <= 0)
    {
        g_slist_free (list);
        mate_uesr_admin_log("Error","mate-user-admin No available users");
        MessageReport(_("Get User Info"),_("user count is 0"),ERROR);
        return -1;
    }        
    mate_uesr_admin_log("Info","mate-user-admin user %d",UserCnt);
    /*user sort */
    list = g_slist_sort (list, (GCompareFunc)SortUsers);
    ua->UsersList = NULL;
    for (l = list; l; l = l->next)
    {
        user = UserAdded (l->data);
        if(user != NULL)
        {
            ua->UsersList = g_slist_append(ua->UsersList,g_object_ref(user));
        }    
    }
    g_slist_free (list);
    return UserCnt; 
}        
