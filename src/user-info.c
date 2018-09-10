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
static const gchar * GetPasswordModeText (ActUser *user,int *Type)
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
static gchar * GetLoginTimeText (ActUser *user)
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

static const gchar *GetRealName (ActUser *user)
{
    const gchar *name = NULL;
    if(user == NULL)
    {
        MessageReport(_("Get User Info"),_("user == NULL Exit!!!"),ERROR);
        exit(0);
    }        
    name = act_user_get_real_name (user);

    return name;
}
static const gchar *GetUserName(ActUser *user)
{
    const gchar *name =NULL; 
    name = act_user_get_user_name (user);
    
    return name;

}        
static const gchar *GetHomeName(ActUser *user)
{
    const gchar *name = NULL;
    name = act_user_get_home_dir (user);
    
    return name;

}        
static const gchar *GetIconPath(ActUser *user)
{
    const gchar *Path = NULL;
    Path = act_user_get_icon_file (user);
    if(access(Path,F_OK)== 0)
        return Path;
    return NULL;

}        
static const char *GetUserLang(ActUser *user)
{
    const gchar *Lang = NULL;

    Lang = act_user_get_language(user);
    return Lang;
}

static int GetUserType(ActUser *user)
{
    ActUserAccountType UserType;

    UserType = act_user_get_account_type (user);
    if(UserType == ACT_USER_ACCOUNT_TYPE_ADMINISTRATOR)
        return ADMIN;

    return STANDARD;
}        
static int GetUserAutoLogin(ActUser *user)
{
    int Auto;

    Auto = act_user_get_automatic_login(user);

    return Auto;
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
static void UserAdded(ActUser *user,int index,UserAdmin *ua)
{
    const char *UserName;
    const char *RealName;
    const char *Unknown = _("Unknown");
    const char *HomeName;
    const char *IconFile;
    const char *LangName;
    const char *PassText;
    const char *TimeLogin;
    int UserType;
    int Auto;
    char Msg[120] = { 0 };

    /*user real name Can be modified*/
    RealName = GetRealName (user);
    memset(ua->ul[index].RealName,'\0',sizeof(ua->ul[index].RealName));
    if(RealName == NULL)
    {
        memcpy(ua->ul[index].RealName,Unknown,strlen(Unknown));
    }
    else
    {        
        memcpy(ua->ul[index].RealName,RealName,strlen(RealName));
    }
    /*user name Cannot be modified*/
    UserName = GetUserName(user);
    memset(ua->ul[index].UserName,'\0',sizeof(ua->ul[index].UserName));
    memcpy(ua->ul[index].UserName,UserName,strlen(UserName));
   
    /*Home directory name*/ 
    HomeName = GetHomeName(user);
    memset(ua->ul[index].HomeName,'\0',sizeof(ua->ul[index].HomeName));
    if(HomeName == NULL)
    {
        memcpy(ua->ul[index].HomeName,Unknown,strlen(Unknown));
    }        
    else
    {
        memcpy(ua->ul[index].HomeName,HomeName,strlen(HomeName));
    }       
    /*user login icon*/
    IconFile = GetIconPath(user);
    memset(ua->ul[index].UserIcon,'\0',sizeof(ua->ul[index].UserIcon));
    if(IconFile == NULL)
    {       
        memcpy(ua->ul[index].UserIcon,DEFAULT,strlen(DEFAULT));
    }
    else 
    {
        memcpy(ua->ul[index].UserIcon,IconFile,strlen(IconFile));
    }        

    /*user password*/
    PassText = GetPasswordModeText(user,&ua->ul[index].PasswordType);
    memset(ua->ul[index].PassText,'\0',sizeof(ua->ul[index].PassText));
    memcpy(ua->ul[index].PassText,PassText,strlen(PassText));

    /*get user language type*/
    LangName = GetUserLang(user);
    if(strlen(LangName) <= 0)
    {
        sprintf(Msg,"%s %s",ua->ul[index].UserName,_("Get languages faild"));
        MessageReport(_("Get user languages"),Msg,ERROR); 
    }
    memset(ua->ul[index].LangName,'\0',sizeof(ua->ul[index].LangName));
    memcpy(ua->ul[index].LangName,LangName,strlen(LangName));
    /*get user type Administrator or standard user*/
    UserType = GetUserType(user);
    ua->ul[index].UserType = UserType;

    /*auto login*/
    Auto = GetUserAutoLogin(user);
    ua->ul[index].LoginType = Auto;         
   
    /*login time*/
    memset(ua->ul[index].UserTime,'\0',sizeof(ua->ul[index].UserTime));
    TimeLogin = GetLoginTimeText(user); 
    memcpy(ua->ul[index].UserTime,TimeLogin,strlen(TimeLogin));
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
    ActUserManager *Manager;
    ActUser *user;
    int i = 0;
    int UserCnt;
    
    /* get all user list */
    Manager = act_user_manager_get_default ();
    list = act_user_manager_list_users (Manager);
    /*user number*/
    UserCnt = g_slist_length (list);     
    if(UserCnt <= 0)
    {
        g_slist_free (list);
        MessageReport(_("Get User Info"),_("user count is 0"),ERROR);
        return -1;
    }        
    /*user sort */
    list = g_slist_sort (list, (GCompareFunc)SortUsers);
    
    for (l = list; l; l = l->next,i++)
    {
        user = l->data;
        ua->ul[i].User = user;
        UserAdded (user,i,ua);
    }
    g_slist_free (list);

    return UserCnt; 
}        

