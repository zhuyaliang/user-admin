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

#include "user.h"
#include "user-share.h"
#include "user-admin.h"
#include "user-info.h"
#include "user-list.h"
#include <pwd.h>
#include <sys/types.h>

GtkWidget *WindowAddUser;
/* password and name valid */
static int UnlockFlag;

static uid_t GetLoginUserUid(void)
{
    int fd;
    char LoginUid[10] = { 0 };

    fd = open("/proc/self/loginuid",O_RDONLY);
    if(fd < 0)
    {
        return -1;
    }

    if(read(fd,LoginUid,10) < 0)
    {
        close(fd);
        return -1;
    }

    close(fd);
    return atoi(LoginUid);
}

static gboolean CheckLoginUser(uid_t uid)
{
    if(uid == GetLoginUserUid())
    {
        return TRUE;
    }    
    return FALSE;
}    
/******************************************************************************
* Function:              RemoveUser 
*        
* Explain: delete user
*        
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  09/05/2018
******************************************************************************/
void RemoveUser(GtkWidget *widget, gpointer data)
{
    UserAdmin      *ua = (UserAdmin *)data;
    GtkTreeIter     iter;
    int             nRet;
    int             RemoveCount = 0;
    gboolean        RemoveType = TRUE;
    ActUserManager *Manager;
    UserInfo       *user;
    GError         *error = NULL;
    Manager =       act_user_manager_get_default ();

    user = GetIndexUser(ua->UsersList,gnCurrentUserIndex);
    if (gtk_tree_selection_get_selected(ua->UserSelect, &ua->Model, &iter))
    {
        if(CheckLoginUser(act_user_get_uid(user->ActUser)) == TRUE)
        {
            MessageReport(_("Remove User"),
                          _("You cannot delete your own account."),
                           ERROR); 
            return;
        
        }    
        nRet = MessageReport(_("Remove User"),
                             _("Whether to remove the user's home directory"),
                              QUESTION);
        if(nRet == GTK_RESPONSE_NO)
        {
             RemoveType = FALSE;
        }
        else if(nRet == GTK_RESPONSE_DELETE_EVENT ||
                nRet ==  GTK_RESPONSE_ACCEPT)
        {
             return;
        }
         /* remove autologin */
        if (act_user_get_automatic_login (user->ActUser)) 
        {
            act_user_set_automatic_login (user->ActUser, FALSE);
        }
        while(!act_user_manager_delete_user(Manager,user->ActUser,RemoveType,&error))
        {
            error = NULL;
            /*This function will go wrong and need to be called many times*/
            if(RemoveCount > 5)  
            {   
                MessageReport(_("Remove User"),
                              _("Remove user failure and try again"),
                               ERROR); 
                break;
            }    
        }
        if(RemoveCount < 5)    //5次操作内成功移除用户
        {    
            ua->UsersList = g_slist_remove(ua->UsersList,user);
            g_object_unref(user);
            RefreshUserList(ua->UserList,ua->UsersList);
            /*Scavenging user information*/
            user = GetIndexUser(ua->UsersList,gnCurrentUserIndex);
            if(user == NULL)
            {
                g_error(_("No such user!!!"));
            }    
            UpdateInterface(user->ActUser,ua);                  
            ua->UserCount--;                                        
        }
    }

}        

static gboolean CheckUserNameUsed (const gchar *UserName)
{
    struct passwd *pwent;
    
    if (UserName == NULL || UserName[0] == '\0') 
    {
        return FALSE;
    }

    pwent = getpwnam (UserName);

    return pwent != NULL;
}

/******************************************************************************
* Function:              UserNameValidCheck 
*        
* Explain: check the validity of a username.include Is it empty
*          Whether or not to use and character check
*        
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  09/05/2018
******************************************************************************/
static gboolean UserNameValidCheck (const gchar *UserName, gchar **Message)
{
    gboolean empty;
    gboolean home_use;
    gboolean in_use;
    gboolean valid;
    const gchar *c;
    char HomeName[32] = { 0 };

    if (UserName == NULL || UserName[0] == '\0') 
    {
        empty = TRUE;
        in_use = FALSE;
        home_use = FALSE;
    } 
    else 
    {
        empty = FALSE;
        in_use = CheckUserNameUsed (UserName);
        sprintf(HomeName,"/home/%s",UserName);
        home_use = access(HomeName,F_OK ) == 0;
    }
    valid = TRUE;
    if (!in_use && !empty && !home_use) 
    {
        for (c = UserName; *c; c++) 
        {
            if (! ((*c >= 'a' && *c <= 'z') ||
                   (*c >= 'A' && *c <= 'Z') ||
                   (*c >= '0' && *c <= '9') ||
                   (*c == '_') || (*c == '.') ||
                   (*c == '-' && c != UserName)))
               valid = FALSE;
        }
    }

    valid = !empty && !in_use && !home_use && valid;
    if (!empty && (in_use || home_use || !valid))
    {
        if (in_use) 
        {
            *Message = g_strdup (_("Repeat of user name.Please try another"));
        }
        else if(home_use)
        {
            *Message = g_strdup (_("Repeat of user home name.Please try another"));
        }        
        else if (UserName[0] == '-') 
        {
            *Message = g_strdup (_("The username cannot start with a - ."));
        }
        else 
        {
            *Message = g_strdup (_("The username should only consist of upper and lower case \nletters from a-z,digits and the following characters: . - _"));
        }
    }

    return valid;
}

/******************************************************************************
* Function:             CheckName 
*        
* Explain: 800 milliseconds to check the validity of a username
*        
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  09/05/2018
******************************************************************************/
static gboolean CheckName(gpointer data)
{
    CreateUser *newuser = (CreateUser *)data;
    gboolean    Valid;
    int         input = 0;
    char       *Message = NULL;
    const char *s;
    const char *r;
    const char *FixedNote = _("This will be used to name your home folder and can't be changed");   
    
    gtk_entry_set_icon_from_icon_name(GTK_ENTRY(newuser->UserNameEntry),
                                      GTK_ENTRY_ICON_SECONDARY,
                                      NULL);
    s = gtk_entry_get_text(GTK_ENTRY(newuser->UserNameEntry));
    r = gtk_entry_get_text(GTK_ENTRY(newuser->RealNameEntry));
    if(strlen(r) > 0)
    {
        gtk_entry_set_icon_from_icon_name(GTK_ENTRY(newuser->RealNameEntry),
                                          GTK_ENTRY_ICON_SECONDARY,
                                         "emblem-ok-symbolic");
    
    }    
    else
    {
        input = 1;
        gtk_entry_set_icon_from_icon_name(GTK_ENTRY(newuser->RealNameEntry),
                                          GTK_ENTRY_ICON_SECONDARY,
                                          NULL);
    }
    if(strlen(s) <= 0 )
    {
        gtk_widget_set_sensitive(newuser->ButtonConfirm, FALSE);
        return TRUE;
    }
    Valid = UserNameValidCheck(s,&Message);
    if(Message != NULL)
        SetLableFontType(newuser->LabelNameNote,"red",10,Message);
    else
    { 
        gtk_entry_set_icon_from_icon_name(GTK_ENTRY(newuser->UserNameEntry),
                                          GTK_ENTRY_ICON_SECONDARY,
                                         "emblem-ok-symbolic");
        
        SetLableFontType(newuser->LabelNameNote,"gray",10,FixedNote);
    }
    if(UnlockFlag == 0 && Valid && input == 0)
        gtk_widget_set_sensitive(newuser->ButtonConfirm, TRUE);
    else
        gtk_widget_set_sensitive(newuser->ButtonConfirm, FALSE);     
    return TRUE;
}

/******************************************************************************
* Function:             WriteUserInfo 
*        
* Explain: create new user 
*        
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  09/05/2018
******************************************************************************/
static ActUser *WriteUserInfo(const gchar *UserName,
                              const gchar *RealName,
                              int          UserType,
                              const gchar *LangName,
                              int          PasswordType,
                              const gchar *TmpPass)
{
    ActUserManager *Manager;
    GError *error = NULL;
    ActUser *user;

    Manager = act_user_manager_get_default ();
    user = act_user_manager_create_user(Manager,
                                        UserName,
                                        RealName,
                                        ACT_USER_ACCOUNT_TYPE_STANDARD,
                                       &error);
    if(user == NULL)
    {
        g_error("error Create NewUser Error %s\r\n",error->message);
        g_error_free(error); 
        MessageReport(_("Creating User"),_("Creating a user failed"),ERROR);
        return NULL;
    }        
    if(UserType == ADMIN)
    {        
        act_user_set_account_type(user,ACT_USER_ACCOUNT_TYPE_ADMINISTRATOR);
    }    
    act_user_set_language(user,LangName);
    
    if(PasswordType == NEWPASS)
    {
        act_user_set_password_mode (user,ACT_USER_PASSWORD_MODE_SET_AT_LOGIN);
    }
    else
    {
        act_user_set_password_mode (user, ACT_USER_PASSWORD_MODE_REGULAR);
        act_user_set_password (user,TmpPass, "");
    }
    return user;

}       
static const gchar *GetNewUserPassword(GtkWidget *EntryPass1,GtkWidget *EntryPass2)
{
    const char *p1 = NULL;
    const char *p2 = NULL;

    p1 =  gtk_entry_get_text(GTK_ENTRY(EntryPass1));   
    p2 =  gtk_entry_get_text(GTK_ENTRY(EntryPass2));
        
    if(strcmp(p1,p2) != 0 )
    {
        return NULL;
    }
    return p2;
}    
static int GetNewUserType(GtkWidget *Switch)
{
    return gtk_combo_box_get_active (GTK_COMBO_BOX(Switch)) ?
                                     ACT_USER_ACCOUNT_TYPE_ADMINISTRATOR : 
                                     ACT_USER_ACCOUNT_TYPE_STANDARD;
}  
static const gchar *GetNewUserLang(UserAdmin *ua)
{
    UserInfo *user;
    char     *Lang_id;
    
    user = GetIndexUser(ua->UsersList,gnCurrentUserIndex);
    Lang_id = act_user_get_language(user->ActUser);
       
    if(Lang_id != NULL)
    {
        return mate_get_language_from_locale (Lang_id, NULL);
    }    
    return mate_get_language_from_locale ("en_US.utf8",NULL);
}    
static void CloseWindow(GtkWidget *widget,gpointer data);
/******************************************************************************
* Function:             CreateNewUser
*        
* Explain: Create new users
*          step 1 Check the password for the two input
*          step 2 Add the user to the list 
*          step 3 create user
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  09/05/2018
******************************************************************************/
static void CreateNewUser(GtkWidget *widget,gpointer data)
{
    UserAdmin  *ua = (UserAdmin *)data;
    UserInfo   *user;
    UserInfo   *currentuser;
    const char *rn;
    const char *un;
    const char *NewUserlang;
    const char *Password;
    ActUser    *ActUser;
    int         account_type;
    int         password_type = NEWPASS;

    rn = gtk_entry_get_text(GTK_ENTRY(ua->newuser.RealNameEntry));
    un = gtk_entry_get_text(GTK_ENTRY(ua->newuser.UserNameEntry));
    account_type = GetNewUserType(ua->newuser.NewUserType);
    NewUserlang  = GetNewUserLang(ua);

    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ua->newuser.RadioButton2)) == TRUE)
    {        
        Password = GetNewUserPassword(ua->newuser.NewPassEntry,ua->newuser.CheckPassEntry);
        if(Password == NULL)
        {    
            return;
        }    
        password_type = OLDPASS;
    }    

    UnlockFlag = 0;
    ActUser = WriteUserInfo(un,
                            rn,
                            account_type,
                            NewUserlang,
                            password_type,
                            Password);
    if(ActUser != NULL)
    {   

        user = user_new();
        user->UserName = g_strdup(un);
        user->ActUser  = ActUser;
        UserListAppend(ua->UserList,
                       DEFAULT,
                       rn,
                       un,
                       ua->UserCount,
                      &ua->newuser.NewUserIter);
        user->Iter     = ua->newuser.NewUserIter;
        ua->UsersList  = g_slist_append(ua->UsersList,g_object_ref(user));
        currentuser    = GetIndexUser(ua->UsersList,gnCurrentUserIndex);
        UpdateInterface(currentuser->ActUser,ua);
        ua->UserCount +=1;//用户个数加1
    }    
    CloseWindow(NULL,&ua->newuser);
} 
static void CloseWindow(GtkWidget *widget,gpointer data)
{
    CreateUser *newuser = (CreateUser *)data;
    gtk_widget_destroy(GTK_WIDGET(newuser->AddUserDialog));

    if(newuser->CheckPassTimeId > 0)
        g_source_remove(newuser->CheckPassTimeId);
    if(newuser->CheckNameTimeId > 0)
        g_source_remove(newuser->CheckNameTimeId);

    newuser->CheckPassTimeId = 0;
    newuser->CheckNameTimeId = 0;
    UnlockFlag = 0;
}       
/******************************************************************************
* Function:             SetNewUserInfo 
*        
* Explain: New user name and type and language
*        
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  09/05/2018
******************************************************************************/
static void SetNewUserInfo(GtkWidget *Vbox,CreateUser *newuser)
{
    GtkWidget *Table;
    GtkWidget *LabelUserName;
    GtkWidget *LabelRealName;
    GtkWidget *LabelUserType;
    const char *FixedNote = _("This will be used to name your home folder and can't be changed");   

    Table = gtk_grid_new();
    gtk_box_pack_start(GTK_BOX(Vbox), Table,TRUE, TRUE, 0);
    gtk_grid_set_column_homogeneous(GTK_GRID(Table),TRUE);

    LabelUserName = gtk_label_new(NULL);
    SetLableFontType(LabelUserName,"gray",11,_("User Name"));
    gtk_grid_attach(GTK_GRID(Table) , LabelUserName , 0 , 0 , 1 , 1);

    newuser->UserNameEntry   = gtk_entry_new();
    newuser->CheckNameTimeId = g_timeout_add(800,(GSourceFunc)CheckName,newuser);
    gtk_entry_set_max_length(GTK_ENTRY(newuser->UserNameEntry),24);
    gtk_grid_attach(GTK_GRID(Table) ,newuser->UserNameEntry , 1 , 0 , 3 , 1);

    newuser->LabelNameNote = gtk_label_new (NULL);
    SetLableFontType(newuser->LabelNameNote,"gray",10,FixedNote);
    gtk_grid_attach(GTK_GRID(Table) ,newuser->LabelNameNote , 0 , 1, 4 , 1);

    LabelRealName = gtk_label_new(NULL);
    SetLableFontType(LabelRealName,"gray",11,_("Login Name"));
    gtk_grid_attach(GTK_GRID(Table) , LabelRealName , 0 , 2 , 1 , 1);

    newuser->RealNameEntry = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(newuser->RealNameEntry),24);
    gtk_grid_attach(GTK_GRID(Table), newuser->RealNameEntry , 1 , 2 , 3 , 1);
	  
    LabelUserType = gtk_label_new(NULL);
    SetLableFontType(LabelUserType,"gray",11,_("Account Type"));
    gtk_grid_attach(GTK_GRID(Table) ,LabelUserType , 0 , 3 , 1 , 1);        
     
    newuser->NewUserType = SetComboUserType(_("Standard"),_("Administrators"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(newuser->NewUserType),STANDARD);
   
    gtk_grid_attach(GTK_GRID(Table) ,newuser->NewUserType , 1 , 3 , 3 , 1);        

    gtk_grid_set_row_spacing(GTK_GRID(Table), 10);
    gtk_grid_set_column_spacing(GTK_GRID(Table), 10);

}
static void ComparePassword (CreateUser *newuser)
{
    const gchar *password;
    const gchar *NoteMessage = _("The passwords entered twice are different");

    password = gtk_entry_get_text(GTK_ENTRY(newuser->CheckPassEntry));
    if(strlen(password) <=0)
        return;
    if(GetNewUserPassword(newuser->NewPassEntry,newuser->CheckPassEntry) == NULL)
    {
        gtk_entry_set_icon_from_icon_name(GTK_ENTRY(newuser->CheckPassEntry),
                                          GTK_ENTRY_ICON_SECONDARY,
                                          NULL);
        SetLableFontType(newuser->LabelSpace,"red",10,NoteMessage);
        UnlockFlag = 1;
        return;
    }    
    gtk_entry_set_icon_from_icon_name(GTK_ENTRY(newuser->CheckPassEntry),
                                      GTK_ENTRY_ICON_SECONDARY,
                                     "emblem-ok-symbolic");
    gtk_label_set_markup(GTK_LABEL(newuser->LabelSpace),NULL); 
    UnlockFlag = 0;
}    
/******************************************************************************
* Function:              TimeFun
*        
* Explain: Whether the timer is legitimate to check the password
*        
* Input:   Password valid    UnlockFlag = 0
*          Password invalid  UnlockFlag = 1
* Output: 
*        
* Author:  zhuyaliang  09/05/2018
******************************************************************************/
static gboolean TimeFun(gpointer data)
{
    CreateUser  *newuser = (CreateUser *)data;
    const char  *s;
    int          Level;
    const char  *Message;
    const char  *tip = _("Hybrid passwords improve security");

    s = gtk_entry_get_text(GTK_ENTRY(newuser->NewPassEntry));
    if(strlen(s) == 0)
    {
        //gtk_entry_set_visibility(GTK_ENTRY(newuser->NewPassEntry),FALSE);
        SetLableFontType(newuser->LabelPassNote,"gray",10,tip);
        return TRUE;
    }
    Level = GetPassStrength (s, NULL,NULL,&Message);
    gtk_level_bar_set_value (GTK_LEVEL_BAR (newuser->LevelBar), Level);

    if(Message == NULL && Level > 1)
    {
        gtk_entry_set_icon_from_icon_name(GTK_ENTRY(newuser->NewPassEntry),
                                          GTK_ENTRY_ICON_SECONDARY,
                                          "emblem-ok-symbolic");
        gtk_widget_set_sensitive(newuser->CheckPassEntry, TRUE);
        SetLableFontType(newuser->LabelPassNote,"gray",10,tip);
        ComparePassword(newuser);
        return TRUE;
    }
    gtk_entry_set_icon_from_icon_name(GTK_ENTRY(newuser->NewPassEntry), 
                                      GTK_ENTRY_ICON_SECONDARY,
                                     "system-run");
    SetLableFontType(newuser->LabelPassNote,"red",10,Message);
    UnlockFlag = 1;
    return TRUE;
}

/******************************************************************************
* Function:             LoginSetPass
*        
* Explain: Set up next time
*        
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  09/05/2018
******************************************************************************/
static void LoginSetPass(GtkRadioButton *button,gpointer data)
{
    CreateUser *newuser = (CreateUser *)data;
 
    gtk_widget_set_sensitive(newuser->NewPassEntry, FALSE);  //lock widget
    gtk_widget_set_sensitive(newuser->CheckPassEntry, FALSE);
    gtk_widget_set_sensitive(newuser->LevelBar, FALSE);
 
    if(newuser->CheckPassTimeId > 0)                //因为不需要检查密码所以移除定时器
    {
        g_source_remove(newuser->CheckPassTimeId);
        newuser->CheckPassTimeId = 0;
    }
    UnlockFlag = 0;
}
/******************************************************************************
* Function:            NowSetNewUserPass 
*        
* Explain: Now set the password,Whether the timer is legitimate to check the password
*        
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  09/05/2018
******************************************************************************/
static void NowSetNewUserPass(GtkRadioButton *button,gpointer data)
{
    CreateUser *newuser = (CreateUser *)data;
 
    UnlockFlag = 1;
    gtk_widget_set_sensitive(newuser->NewPassEntry, TRUE);
    gtk_widget_set_sensitive(newuser->LevelBar, TRUE);

    newuser->CheckPassTimeId = g_timeout_add(800,(GSourceFunc)TimeFun,newuser);

}        
/******************************************************************************
* Function:              GetNewUserPass 
*        
* Explain: Set new user password
*        
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  09/05/2018
******************************************************************************/
static void SetNewUserPass(GtkWidget *Vbox,CreateUser *newuser)
{
    GtkWidget *Table;
    GtkWidget *LabelTitle;
    GSList    *RadioGroup;
    GtkWidget *LabelPass;
    GtkWidget *LabelConfirm;
    
    Table = gtk_grid_new();
    gtk_box_pack_start(GTK_BOX(Vbox), Table,TRUE, TRUE, 20);
    gtk_grid_set_column_homogeneous(GTK_GRID(Table),TRUE);
    LabelTitle = gtk_label_new(_("Password"));
    gtk_grid_attach(GTK_GRID(Table) , LabelTitle , 0 , 0 , 1 , 1);

    //新建两个单选按钮

    newuser->RadioButton1 = gtk_radio_button_new_with_label(NULL,_("Set up next time"));
    RadioGroup = gtk_radio_button_get_group(GTK_RADIO_BUTTON(newuser->RadioButton1));
    gtk_grid_attach(GTK_GRID(Table) , newuser->RadioButton1 , 0 , 1 , 5 , 1);
    g_signal_connect(newuser->RadioButton1,
                    "released",
                     G_CALLBACK(LoginSetPass),
                     newuser); 
    
    newuser->RadioButton2 = gtk_radio_button_new_with_label(RadioGroup,_("Now set the password"));
    gtk_grid_attach(GTK_GRID(Table) , newuser->RadioButton2 , 0 , 2 , 5 , 1);
    g_signal_connect(newuser->RadioButton2,
                    "released",
                     G_CALLBACK(NowSetNewUserPass),
                     newuser); 
   
    LabelPass = gtk_label_new(NULL);
    SetLableFontType(LabelPass,"gray",11,_("Password"));
    gtk_grid_attach(GTK_GRID(Table) ,LabelPass , 0 , 3 , 1 , 1);     

    newuser->NewPassEntry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(newuser->NewPassEntry),FALSE);
    gtk_entry_set_icon_from_icon_name(GTK_ENTRY(newuser->NewPassEntry), 
                                      GTK_ENTRY_ICON_SECONDARY,
                                     "system-run");
    gtk_entry_set_max_length(GTK_ENTRY(newuser->NewPassEntry),24);
    gtk_grid_attach(GTK_GRID(Table) , newuser->NewPassEntry , 1 , 3 , 4 , 1);
   
    gtk_entry_set_icon_tooltip_text (GTK_ENTRY(newuser->NewPassEntry),
                                     GTK_ENTRY_ICON_SECONDARY,
                                    _("generation password"));	
	
    newuser->LevelBar = gtk_level_bar_new ();
    gtk_level_bar_set_min_value(GTK_LEVEL_BAR(newuser->LevelBar),0.0);
    gtk_level_bar_set_max_value(GTK_LEVEL_BAR(newuser->LevelBar),5.0);
    gtk_level_bar_set_mode(GTK_LEVEL_BAR(newuser->LevelBar),GTK_LEVEL_BAR_MODE_DISCRETE);
    gtk_grid_attach(GTK_GRID(Table) ,newuser->LevelBar , 1 , 4 , 4 , 1);
	
    newuser->LabelPassNote = gtk_label_new (NULL);
    gtk_grid_attach(GTK_GRID(Table) ,newuser->LabelPassNote , 0 , 5 , 4 , 1);

    LabelConfirm = gtk_label_new (NULL);
    SetLableFontType(LabelConfirm,"gray",11,_("Confirm"));
    gtk_grid_attach(GTK_GRID(Table) ,LabelConfirm , 0 , 6 , 1 , 1);

    newuser->CheckPassEntry = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(newuser->CheckPassEntry),24);
    gtk_entry_set_visibility(GTK_ENTRY(newuser->CheckPassEntry),FALSE);
    gtk_grid_attach(GTK_GRID(Table) ,newuser->CheckPassEntry, 1 , 6 , 4 , 1);
    g_signal_connect (G_OBJECT(newuser->NewPassEntry), 
                     "icon-press", 
                      G_CALLBACK(AutoGenera), 
                      newuser->CheckPassEntry);
    
    newuser->LabelSpace = gtk_label_new("");
    gtk_grid_attach(GTK_GRID(Table) , newuser->LabelSpace , 0 , 7 , 4 , 1);

    gtk_widget_set_sensitive(newuser->NewPassEntry, FALSE);  //lock widget
    gtk_widget_set_sensitive(newuser->CheckPassEntry, FALSE);
    gtk_widget_set_sensitive(newuser->LevelBar, FALSE);   

    gtk_grid_set_row_spacing(GTK_GRID(Table), 10);
    gtk_grid_set_column_spacing(GTK_GRID(Table), 10);
}        
static void LoadHeader_bar(UserAdmin *ua)
{
    GtkWidget *Header;
    Header = gtk_header_bar_new (); 
    gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (Header), TRUE);
    gtk_header_bar_set_title (GTK_HEADER_BAR (Header), _("Create New User"));

    ua->newuser.ButtonConfirm = gtk_button_new_with_label (_("Confirm"));
    gtk_header_bar_pack_end (GTK_HEADER_BAR (Header), ua->newuser.ButtonConfirm);
    ua->newuser.ButtonCancel = gtk_button_new_with_label ("Cancel");
    gtk_header_bar_pack_start (GTK_HEADER_BAR (Header), ua->newuser.ButtonCancel);
    
    gtk_window_set_titlebar (GTK_WINDOW (ua->newuser.AddUserDialog), Header);
}    
/******************************************************************************
* Function:              AddNewUser 
*        
* Explain: add new user
*        
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  09/05/2018
******************************************************************************/
void AddNewUser(GtkWidget *widget, gpointer data)
{
    GtkWidget *Vbox;
    GtkWidget *Vbox1;
    GtkWidget *Vbox2;
    UserAdmin *ua = (UserAdmin *)data;
	
    ua->newuser.AddUserDialog = gtk_dialog_new_with_buttons ("Interactive Dialog",
                                        GTK_WINDOW (WindowLogin),
                                        GTK_DIALOG_MODAL| GTK_DIALOG_DESTROY_WITH_PARENT,
                                        NULL,
                                        NULL);
    if(GetUseHeader() == 1)
    {
        LoadHeader_bar(ua); 
    }
    else
    {
        ua->newuser.ButtonConfirm = gtk_dialog_add_button(GTK_DIALOG(ua->newuser.AddUserDialog),
                                                         _("Confirm"),
                                                         GTK_RESPONSE_NONE);    
        ua->newuser.ButtonCancel  = gtk_dialog_add_button(GTK_DIALOG(ua->newuser.AddUserDialog),
                                                         _("Cancel"),
                                                         GTK_RESPONSE_NONE);
        gtk_window_set_title(GTK_WINDOW(ua->newuser.AddUserDialog),_("Create New User"));
    }    
    gtk_window_set_deletable(GTK_WINDOW (ua->newuser.AddUserDialog), FALSE);
    gtk_window_set_default_size (GTK_WINDOW (ua->newuser.AddUserDialog), 500, 450);
    gtk_container_set_border_width(GTK_CONTAINER(ua->newuser.AddUserDialog),20);
   

    gtk_widget_set_sensitive(ua->newuser.ButtonConfirm,FALSE); 
    g_signal_connect (ua->newuser.ButtonConfirm,
                     "clicked",
                      G_CALLBACK(CreateNewUser),
                      ua);
    g_signal_connect (ua->newuser.ButtonCancel, 
                     "clicked",
                      G_CALLBACK (CloseWindow),
                      &ua->newuser);

    Vbox =  gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
    gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (ua->newuser.AddUserDialog))),
                        Vbox,
                        TRUE, 
                        TRUE, 
                        8);

    Vbox1 =  gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
    gtk_box_pack_start(GTK_BOX(Vbox), Vbox1,TRUE, TRUE, 0);
    Vbox2 =  gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
    gtk_box_pack_start(GTK_BOX(Vbox), Vbox2,TRUE, TRUE, 0);
    
    ua->newuser.CheckNameTimeId = 0;
    ua->newuser.CheckPassTimeId = 0;
    SetNewUserInfo(Vbox1,&ua->newuser); 
    SetNewUserPass(Vbox2,&ua->newuser);
    
    gtk_widget_show_all(ua->newuser.AddUserDialog);
}        
