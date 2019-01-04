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
                g_error(_("No such user!!!\r\n"));
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
    else 
    {
        *Message = g_strdup (_("This will be used to name your home folder and can't be changed."));
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
    UserAdmin *ua = (UserAdmin *)data;
    gboolean Valid;
    char *Message = NULL;
    const char *s;
    
    s = gtk_entry_get_text(GTK_ENTRY(ua->UserNameEntry));
    if(strlen(s) <= 0 )
        return TRUE;
    Valid = UserNameValidCheck(s,&Message);
    SetLableFontType(ua->LabelNameNote,"red",10,Message);
    
    if(UnlockFlag == 0 && Valid)
        gtk_widget_set_sensitive(ua->ButtonConfirm, TRUE);
    else
        gtk_widget_set_sensitive(ua->ButtonConfirm, FALSE);     
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
static const gchar *GetNewUserLang(GtkWidget *Combox)
{
    char *text;
    GtkTreeIter   iter;
    GtkTreeModel *model;
    const char *LangName;

    if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(Combox), &iter ))
    {
        model = gtk_combo_box_get_model(GTK_COMBO_BOX(Combox));
        gtk_tree_model_get( model, &iter, 0, &text, -1 );
        LangName = g_hash_table_lookup(LocaleHash,text);
        g_free(text);
        return LangName;
    }
    return NULL;
}    
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
    UserAdmin *ua = (UserAdmin *)data;
    UserInfo *user;
    const char *rn;
    const char *un;
    const char *NewUserlang;
    const char *Password;
    UserInfo *newuser;
    ActUser *ActUser;
    int account_type;
    int password_type = NEWPASS;
    const char *NoteMessage = _("Two inconsistencies in cipher input");

    rn = gtk_entry_get_text(GTK_ENTRY(ua->RealNameEntry));
    un = gtk_entry_get_text(GTK_ENTRY(ua->UserNameEntry));
    account_type = GetNewUserType(ua->NewUserType);
    NewUserlang = GetNewUserLang(ua->NewUserLangType);
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ua->RadioButton2)) == TRUE)
    {        
        Password = GetNewUserPassword(ua->NewPassEntry,ua->CheckPassEntry);
        if(Password == NULL)
        {    
            SetLableFontType(ua->LabelSpace,"red",10,NoteMessage);
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

        newuser = user_new();
        newuser->UserName = g_strdup(un);
        newuser->ActUser  = ActUser;
        newuser->Iter     = ua->NewUserIter;
        UserListAppend(ua->UserList,
                       DEFAULT,
                       rn,
                      "black",
                       ua->UserCount,
                      &ua->NewUserIter);
        ua->UsersList = g_slist_append(ua->UsersList,g_object_ref(newuser));
        user = GetIndexUser(ua->UsersList,gnCurrentUserIndex);
        UpdateInterface(user->ActUser,ua);
        ua->UserCount +=1;//用户个数加1
    }    
    gtk_widget_destroy(GTK_WIDGET(ua->AddUserWindow));
} 
static void CloseWindow(GtkWidget *widget,gpointer data)
{
    UserAdmin *ua = (UserAdmin *)data;
    gtk_widget_destroy(GTK_WIDGET(ua->AddUserWindow));
}

static void RemoveTime(GtkWidget *widget,gpointer data)
{
    UserAdmin *ua = (UserAdmin *)data;

    if(ua->CheckPassTimeId > 0)
        g_source_remove(ua->CheckPassTimeId);
    if(ua->CheckNameTimeId > 0)
        g_source_remove(ua->CheckNameTimeId);

    ua->CheckPassTimeId = 0;
    ua->CheckNameTimeId = 0;
    UnlockFlag = 0;
    gtk_widget_set_sensitive(ua->MainWindow, TRUE);	
}       
static gchar *GetLoginUserLang(ActUser *ActUser)
{
    if(getuid() == act_user_get_uid(ActUser))
    {
        return act_user_get_language(ActUser);
    }    
    return "en_US.utf8";
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
static void SetNewUserInfo(GtkWidget *Vbox,UserAdmin *ua)
{
    GtkWidget *Table;
    GtkWidget *LabelUserName;
    GtkWidget *LabelRealName;
    GtkWidget *LabelNameNote;
    GtkWidget *LabelUserType;
    GtkWidget *LabelLanguageType;
    GtkWidget *RealNameEntry;
    GtkWidget *UserNameEntry;
    GtkWidget *NewUserType;
    GtkWidget *NewLanguage;
    const gchar *LoginUserLang;
    int        TimeId;
    int        index;
    UserInfo  *user;
    const char *FixedNote = _("This will be used to name your home folder and can't be changed");   

    Table = gtk_grid_new();
    gtk_box_pack_start(GTK_BOX(Vbox), Table,TRUE, TRUE, 0);
    gtk_grid_set_column_homogeneous(GTK_GRID(Table),TRUE);

    LabelUserName = gtk_label_new(NULL);
    SetLableFontType(LabelUserName,"gray",11,_("User Name"));
    gtk_grid_attach(GTK_GRID(Table) , LabelUserName , 0 , 0 , 1 , 1);

    UserNameEntry = gtk_entry_new();
    ua->UserNameEntry = UserNameEntry;
    TimeId = g_timeout_add(800,(GSourceFunc)CheckName,ua);
    ua->CheckNameTimeId = TimeId;
    gtk_entry_set_max_length(GTK_ENTRY(UserNameEntry),24);
    gtk_grid_attach(GTK_GRID(Table) ,UserNameEntry , 1 , 0 , 3 , 1);

    LabelNameNote = gtk_label_new (NULL);
    SetLableFontType(LabelNameNote,"gray",10,FixedNote);
    ua->LabelNameNote = LabelNameNote;
    gtk_grid_attach(GTK_GRID(Table) ,LabelNameNote , 0 , 1, 4 , 1);

    LabelRealName = gtk_label_new(NULL);
    SetLableFontType(LabelRealName,"gray",11,_("Login Name"));
    gtk_grid_attach(GTK_GRID(Table) , LabelRealName , 0 , 2 , 1 , 1);

    RealNameEntry = gtk_entry_new();
    ua->RealNameEntry = RealNameEntry;
    gtk_entry_set_max_length(GTK_ENTRY(RealNameEntry),24);
    gtk_grid_attach(GTK_GRID(Table) , RealNameEntry , 1 , 2 , 3 , 1);
	  
    LabelUserType = gtk_label_new(NULL);
    SetLableFontType(LabelUserType,"gray",11,_("Account Type"));
    gtk_grid_attach(GTK_GRID(Table) ,LabelUserType , 0 , 3 , 1 , 1);        
     
    user = GetIndexUser(ua->UsersList,0);
    NewUserType = SetComboUserType(_("Standard"),_("Administrators"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(NewUserType),STANDARD);
    ua->NewUserType = NewUserType; 
   
    gtk_grid_attach(GTK_GRID(Table) ,NewUserType , 1 , 3 , 3 , 1);        
    LabelLanguageType = gtk_label_new(NULL);
    SetLableFontType(LabelLanguageType,"gray",11,_("Language"));
    gtk_grid_attach(GTK_GRID(Table) ,LabelLanguageType , 0 , 4 , 1 , 1);        

    NewLanguage = SetComboLanguageType();
    LoginUserLang = GetLoginUserLang(user->ActUser);
    index = GetCurrentLangIndex(LoginUserLang);
    gtk_combo_box_set_active(GTK_COMBO_BOX(NewLanguage), index);
    ua->NewUserLangType = NewLanguage;
    gtk_grid_attach(GTK_GRID(Table) ,NewLanguage , 1 , 4 , 3 , 1);        

    gtk_grid_set_row_spacing(GTK_GRID(Table), 10);
    gtk_grid_set_column_spacing(GTK_GRID(Table), 10);

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
    UserAdmin  *ua = (UserAdmin *)data;
    const char *s;
    int         Level;
    const char *Message;

    s = gtk_entry_get_text(GTK_ENTRY(ua->NewPassEntry));
    if(strlen(s) == 0)
    {
        gtk_entry_set_visibility(GTK_ENTRY(ua->NewPassEntry),FALSE);
    }
    Level = GetPassStrength (s, NULL,NULL,&Message);
    gtk_level_bar_set_value (GTK_LEVEL_BAR (ua->LevelBar), Level);

    if(Message == NULL && Level > 1)
    {
        gtk_label_set_markup(GTK_LABEL(ua->LabelPassNote),NULL); 
        UnlockFlag = 0;
        return TRUE;
    }
    SetLableFontType(ua->LabelPassNote,"gray",10,Message);
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
    UserAdmin *ua = (UserAdmin *)data;
 
    gtk_widget_set_sensitive(ua->NewPassEntry, FALSE);  //lock widget
    gtk_widget_set_sensitive(ua->CheckPassEntry, FALSE);
    gtk_widget_set_sensitive(ua->LevelBar, FALSE);
 
    if(ua->CheckPassTimeId > 0)                //因为不需要检查密码所以移除定时器
    {
        g_source_remove(ua->CheckPassTimeId);
        ua->CheckPassTimeId = 0;
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
    int CheckPassTimeId;
    UserAdmin *ua = (UserAdmin *)data;
 
    UnlockFlag = 1;
    gtk_widget_set_sensitive(ua->CheckPassEntry, TRUE);  //Unlocking Widget
    gtk_widget_set_sensitive(ua->NewPassEntry, TRUE);
    gtk_widget_set_sensitive(ua->LevelBar, TRUE);

    CheckPassTimeId = g_timeout_add(800,(GSourceFunc)TimeFun,ua);
    ua->CheckPassTimeId = CheckPassTimeId;

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
static void SetNewUserPass(GtkWidget *Vbox,UserAdmin *ua)
{
    GtkWidget *Table;
    GtkWidget *LabelTitle;
    GSList    *RadioGroup;
    GtkWidget *RadioButton1;
    GtkWidget *RadioButton2;
    GtkWidget *LabelPass;
    GtkWidget *NewEntryPass;
    GtkWidget *LevelBar;
    GtkWidget *LabelPassNote;
    GtkWidget *LabelConfirm;
    GtkWidget *CheckPass;

    GtkWidget *Hseparator;
    GtkWidget *LabelSpace;
    GtkWidget *ButtonConfirm;
    GtkWidget *ButtonCancel;
    
    Table = gtk_grid_new();
    gtk_box_pack_start(GTK_BOX(Vbox), Table,TRUE, TRUE, 20);
    gtk_grid_set_column_homogeneous(GTK_GRID(Table),TRUE);
    LabelTitle = gtk_label_new(_("Password"));
    gtk_grid_attach(GTK_GRID(Table) , LabelTitle , 0 , 0 , 1 , 1);

    //新建两个单选按钮

    RadioButton1 = gtk_radio_button_new_with_label(NULL,_("Set up next time"));
    RadioGroup = gtk_radio_button_get_group(GTK_RADIO_BUTTON(RadioButton1));
    gtk_grid_attach(GTK_GRID(Table) , RadioButton1 , 0 , 1 , 5 , 1);
    g_signal_connect(RadioButton1,
                    "released",
                     G_CALLBACK(LoginSetPass),
                     ua); 
    
    RadioButton2 = gtk_radio_button_new_with_label(RadioGroup,_("Now set the password"));
    gtk_grid_attach(GTK_GRID(Table) , RadioButton2 , 0 , 2 , 5 , 1);
    ua->RadioButton2 = RadioButton2;
    g_signal_connect(RadioButton2,
                    "released",
                     G_CALLBACK(NowSetNewUserPass),
                     ua); 
   
    LabelPass = gtk_label_new(NULL);
    SetLableFontType(LabelPass,"gray",11,_("Password"));
    gtk_grid_attach(GTK_GRID(Table) ,LabelPass , 0 , 3 , 1 , 1);     

    NewEntryPass = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(NewEntryPass),FALSE);
    ua->NewPassEntry = NewEntryPass;
    gtk_entry_set_icon_from_icon_name(GTK_ENTRY(NewEntryPass), 
                                      GTK_ENTRY_ICON_SECONDARY,
                                     "system-run");
    gtk_entry_set_max_length(GTK_ENTRY(NewEntryPass),24);
    gtk_grid_attach(GTK_GRID(Table) , NewEntryPass , 1 , 3 , 4 , 1);
    g_signal_connect (G_OBJECT(NewEntryPass), 
                     "icon-press", 
                      G_CALLBACK(AutoGenera), 
                      ua);
   
    gtk_entry_set_icon_tooltip_text (GTK_ENTRY(NewEntryPass),
                                     GTK_ENTRY_ICON_SECONDARY,
                                    _("generation password"));	
	
    LevelBar = gtk_level_bar_new ();
    ua->LevelBar = LevelBar;
    gtk_level_bar_set_min_value(GTK_LEVEL_BAR(LevelBar),0.0);
    gtk_level_bar_set_max_value(GTK_LEVEL_BAR(LevelBar),5.0);
    gtk_level_bar_set_mode(GTK_LEVEL_BAR(LevelBar),GTK_LEVEL_BAR_MODE_DISCRETE);
    gtk_grid_attach(GTK_GRID(Table) ,LevelBar , 1 , 4 , 4 , 1);
	
	
    LabelPassNote = gtk_label_new (NULL);
    ua->LabelPassNote = LabelPassNote;
    gtk_grid_attach(GTK_GRID(Table) ,LabelPassNote , 0 , 5 , 4 , 1);

    LabelConfirm = gtk_label_new (NULL);
    SetLableFontType(LabelConfirm,"gray",11,_("Confirm"));
    gtk_grid_attach(GTK_GRID(Table) ,LabelConfirm , 0 , 6 , 1 , 1);

    CheckPass = gtk_entry_new();
    ua->CheckPassEntry = CheckPass;
    gtk_entry_set_max_length(GTK_ENTRY(CheckPass),24);
    gtk_entry_set_visibility(GTK_ENTRY(CheckPass),FALSE);
    gtk_grid_attach(GTK_GRID(Table) , CheckPass , 1 , 6 , 4 , 1);
    
    LabelSpace = gtk_label_new("");
    ua->LabelSpace = LabelSpace;
    gtk_grid_attach(GTK_GRID(Table) , LabelSpace , 0 , 7 , 4 , 1);

    Hseparator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_grid_attach(GTK_GRID(Table) , Hseparator , 0 , 8 , 5 , 1);

    ButtonConfirm = gtk_button_new_with_label(_("Confirm"));
    ua->ButtonConfirm = ButtonConfirm;
    gtk_widget_set_sensitive(ButtonConfirm,FALSE);
    g_signal_connect (ButtonConfirm,
                     "clicked",
                      G_CALLBACK(CreateNewUser),
                      ua);
    gtk_grid_attach(GTK_GRID(Table) , ButtonConfirm , 0 , 9 , 1 , 1);
    
    ButtonCancel =  gtk_button_new_with_label(_("Cancel"));
    g_signal_connect (ButtonCancel, 
                     "clicked",
                      G_CALLBACK (CloseWindow),
                      ua);
    gtk_grid_attach(GTK_GRID(Table) , ButtonCancel , 4 , 9 , 1 , 1);
         
    gtk_widget_set_sensitive(NewEntryPass, FALSE);  //lock widget
    gtk_widget_set_sensitive(CheckPass, FALSE);
    gtk_widget_set_sensitive(LevelBar, FALSE);   

    gtk_grid_set_row_spacing(GTK_GRID(Table), 10);
    gtk_grid_set_column_spacing(GTK_GRID(Table), 10);
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
    gtk_widget_set_sensitive(WindowLogin, FALSE);	
	
    //新建一个窗口
    WindowAddUser = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(WindowAddUser),_("Add User"));
    gtk_window_set_position(GTK_WINDOW(WindowAddUser),GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(WindowAddUser),400,540);
    gtk_container_set_border_width(GTK_CONTAINER(WindowAddUser),20);
    ua->AddUserWindow = WindowAddUser;
    g_signal_connect(WindowAddUser,
                    "destroy",
                     G_CALLBACK(RemoveTime),
                     ua);
	
    Vbox =  gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
    gtk_container_add(GTK_CONTAINER(WindowAddUser),Vbox);

    Vbox1 =  gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
    gtk_box_pack_start(GTK_BOX(Vbox), Vbox1,TRUE, TRUE, 0);
    Vbox2 =  gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
    gtk_box_pack_start(GTK_BOX(Vbox), Vbox2,TRUE, TRUE, 0);
    
    ua->CheckNameTimeId = 0;
    ua->CheckPassTimeId = 0;
    SetNewUserInfo(Vbox1,ua); 
    SetNewUserPass(Vbox2,ua);

    gtk_widget_show_all(WindowAddUser);
}        
