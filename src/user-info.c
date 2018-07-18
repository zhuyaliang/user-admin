#include "user-info.h"
#include "user.h"
#include "user-share.h"

/******************************************************************************
* 函数:              GetPasswordModeText      
*        
* 说明:  获取用户密码设置类型信息。为设置、设置两类。
*        
* 输入:  @user    用户
*       
*        
* 返回:  
*        
* 作者:  zhuyaliang  15/05/2018
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
/******************************************************************************
* 函数:             GetLoginTimeText      
*        
* 说明:  获取用户登录时间
*        
* 输入:  @user    用户
*       
*        
* 返回:  
*        
* 作者:  zhuyaliang  15/05/2018
******************************************************************************/ 
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

/******************************************************************************
* 函数:              GetRealName    
*        
* 说明: 获取用户名字，display name
*        
* 输入:  @user    用户
*        @index  		
*       
*        
* 返回:  用户名
*        
* 作者:  zhuyaliang  15/05/2018
******************************************************************************/ 
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
static int GetUserLang(ActUser *user)
{
    const gchar *Lang = NULL;
    Lang = act_user_get_language(user);
    if(strcmp(Lang,"zh_CN.UTF-8")== 0)
        return CHINSE;
    else if(strcmp(Lang,"en_US.UTF-8") == 0)
        return ENGLIST;

    return -1;

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
/******************************************************************************
* 函数:              SortUsers       
*        
* 说明:  用户排序
*        
* 输入:  @a   用户名
*        @b   用户名		
*       
*        
* 返回:  
*        
* 作者:  zhuyaliang  15/05/2018
******************************************************************************/ 
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
* 函数:              UserAdded       
*        
* 说明:  将所有用户信息写入用户结构体内
*        
* 输入:  @user    用户
*        @index   用户号		
*       
*        
* 返回:  
*        
* 作者:  zhuyaliang  15/05/2018
******************************************************************************/ 
static void UserAdded(ActUser *user,int index,UserAdmin *ua)
{
    const char *UserName;
    const char *RealName;
    const char *Unknown = _("Unknown");
    const char *HomeName;
    const char *IconFile;
    const char *PassText;
    const char *TimeLogin;
    int UserType;
    int LangType;
    int Auto;

    /*用户名字*/
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

    UserName = GetUserName(user);
    memset(ua->ul[index].UserName,'\0',sizeof(ua->ul[index].UserName));
    memcpy(ua->ul[index].UserName,UserName,strlen(UserName));
    
    memset(ua->ul[index].HomeName,'\0',sizeof(ua->ul[index].HomeName));
    HomeName = GetHomeName(user);
    if(HomeName == NULL)
    {
        memcpy(ua->ul[index].HomeName,Unknown,strlen(Unknown));
    }        
    else
    {
        memcpy(ua->ul[index].HomeName,HomeName,strlen(HomeName));
    }       
    /*用户头像*/

    memset(ua->ul[index].UserIcon,'\0',sizeof(ua->ul[index].UserIcon));
    IconFile = GetIconPath(user);
    if(IconFile == NULL)
    {       
        memcpy(ua->ul[index].UserIcon,DEFAULT,strlen(DEFAULT));
    }
    else 
    {
        memcpy(ua->ul[index].UserIcon,IconFile,strlen(IconFile));
    }        

    /*用户密码显示*/
    memset(ua->ul[index].PassText,'\0',sizeof(ua->ul[index].PassText));
    PassText = GetPasswordModeText(user,&ua->ul[index].PasswordType);
    memcpy(ua->ul[index].PassText,PassText,strlen(PassText));

    /*用户语言类型*/
    LangType = GetUserLang(user);
    ua->ul[index].LangType = LangType;
    
    /*用户类型，管理员、普通*/

    UserType = GetUserType(user);
    ua->ul[index].UserType = UserType;

    /*是否自动登录*/
    Auto = GetUserAutoLogin(user);
    ua->ul[index].LoginType = Auto;         
   
    /*登录时间*/
    memset(ua->ul[index].UserTime,'\0',sizeof(ua->ul[index].UserTime));
    TimeLogin = GetLoginTimeText(user); 
    memcpy(ua->ul[index].UserTime,TimeLogin,strlen(TimeLogin));
}    

/******************************************************************************
* 函数:              GetUserInfo        
*        
* 说明:  获取所有用户信息，包括用户名字、头像、用户类型、语言、密码、自动登录、时间
*        
* 输入:  ul 用户信息结构体 		
*       
*        
* 返回: 用户个数 
*        
* 作者:  zhuyaliang  09/05/2018
******************************************************************************/ 
int GetUserInfo(UserAdmin *ua)
{
    GSList *list, *l;
    ActUserManager *Manager;
    ActUser *user;
    int i = 0;
    int UserCnt;
    
    Manager = act_user_manager_get_default ();//获取所以用户列表
    list = act_user_manager_list_users (Manager);
    UserCnt = g_slist_length (list);            //用户总个数
    if(UserCnt <= 0)
    {
        g_slist_free (list);
        MessageReport(_("Get User Info"),_("user count is 0"),ERROR);
        return -1;
    }        
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

