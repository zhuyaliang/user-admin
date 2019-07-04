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
#include <grp.h>
#include <sys/types.h>
#include <libgroupservice/gas-group.h>
#include <libgroupservice/gas-group-manager.h>

#define  NUCONFIG      "/etc/mate-user-admin/nuconfig"
#define  KEYGROUPNAME  "nudefault"
#define  LANGKEY       "nulanguage"
#define  TYPEKEY       "nutype"
#define  GROUPKEY      "nugroups"

GtkWidget *WindowAddUser;
/* password and name valid */
static int UnlockFlag;
static void add_nu_dialog_response (GtkDialog *dialog,
                                    gint       response_id);
static gboolean TimeFun(gpointer data);
static gboolean CheckName(AddNUDialog *and);
G_DEFINE_TYPE (AddNUDialog, add_nu_dialog, GTK_TYPE_DIALOG);

static void RemoveTimer(AddNUDialog *and)
{
	if(and->CheckPassTimeId > 0)
        g_source_remove(and->CheckPassTimeId);
    if(and->CheckNameTimeId > 0)
        g_source_remove(and->CheckNameTimeId);

    and->CheckPassTimeId = 0;
    and->CheckNameTimeId = 0;
}

static void  AddTimer(AddNUDialog *and)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(and->RadioButton2)))
	{
		and->CheckPassTimeId = g_timeout_add(800,(GSourceFunc)TimeFun,and);
	}
    and->CheckNameTimeId = g_timeout_add(800,(GSourceFunc)CheckName,and);

}
static gboolean GetNewUserConfig(AddNUDialog *and)
{
	GKeyFile         *Kconfig = NULL;
	g_autoptr(GError) error = NULL;
	char            **ConfigGroups = NULL;
	char            **unGroups = NULL;
	gsize             Length = 0;
	char             *Value = NULL;
	gboolean          Type;

	Kconfig = g_key_file_new();
	if(Kconfig == NULL)
	{
		mate_uesr_admin_log("Warning","g_key_file_new fail");
		return FALSE;
	}
	if(!g_key_file_load_from_file(Kconfig, NUCONFIG, G_KEY_FILE_NONE, &error))
	{
		mate_uesr_admin_log("Warning","Error loading key file: %s", error->message);
		goto EXIT;
	}
	ConfigGroups = g_key_file_get_groups(Kconfig, &Length);
	if(g_strv_length(ConfigGroups) <= 0)
	{
		mate_uesr_admin_log("Warning","key file format errors are not grouped");
		goto EXIT;
	}
	if(g_key_file_has_key(Kconfig,KEYGROUPNAME,LANGKEY,&error) == FALSE)
	{
		mate_uesr_admin_log("Warning","key file format errors %s",error->message);
		goto EXIT;
	}
	Value = g_key_file_get_string(Kconfig,KEYGROUPNAME,LANGKEY,&error);
	if(Value == NULL)
	{
		mate_uesr_admin_log("Warning","key file format errors %s",error->message);
		goto EXIT;
	}
	if(mate_get_language_from_locale(Value,NULL) == NULL)
	{
		mate_uesr_admin_log("Warning","key file language format errors Language unavailability");
		goto EXIT;
	}
	and->nuLang = g_strdup((gpointer)Value);

	Type = g_key_file_get_boolean(Kconfig,KEYGROUPNAME,TYPEKEY,&error);
	if(Type == FALSE && error != NULL)
	{
		mate_uesr_admin_log("Warning","key file user type format errors %s",error->message);
		goto EXIT;
	}
	and->nuType = Type;

	unGroups = g_key_file_get_string_list(Kconfig,KEYGROUPNAME,GROUPKEY,&Length,&error);
	if(unGroups == NULL)
	{
		mate_uesr_admin_log("Info","key file No default add group is set for new users");
		g_key_file_free(Kconfig);
		return TRUE;
	}
	and->nuGroups = g_strdupv(unGroups);
	g_key_file_free(Kconfig);
	return TRUE;

EXIT:

	and->nuLang = NULL;
	and->nuGroups = NULL;
	g_key_file_free(Kconfig);
	return FALSE;
}
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

static void DeleteOldUserDone (ActUserManager *manager,
                               GAsyncResult   *res,
                               UserAdmin      *ua)
{
	GError *error = NULL;

	if (!act_user_manager_delete_user_finish (manager, res, &error)) 
	{
		if (!g_error_matches (error, ACT_USER_MANAGER_ERROR, ACT_USER_MANAGER_ERROR_PERMISSION_DENIED)) 
		{
			MessageReport(_("Remove User"),
                          error->message,
                          ERROR);
			g_error_free (error);
		}
	}	

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
    gboolean        RemoveType = TRUE;
    ActUserManager *Manager;
    UserInfo       *user;
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

		act_user_manager_delete_user_async (Manager,
                                            user->ActUser,
                                            RemoveType,
                                            NULL,
                                            (GAsyncReadyCallback)DeleteOldUserDone,
                                            ua);

        g_object_unref (user);
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
static gboolean CheckName(AddNUDialog *and)
{
    gboolean    Valid;
    int         input = 0;
    char       *Message = NULL;
    const char *s;
    const char *r;
    const char *FixedNote = _("This will be used to name your home folder and can't be changed");   
    
    gtk_entry_set_icon_from_icon_name(GTK_ENTRY(and->UserNameEntry),
                                      GTK_ENTRY_ICON_SECONDARY,
                                      NULL);
    s = gtk_entry_get_text(GTK_ENTRY(and->UserNameEntry));
    r = gtk_entry_get_text(GTK_ENTRY(and->RealNameEntry));
    if(strlen(r) > 0)
    {
        gtk_entry_set_icon_from_icon_name(GTK_ENTRY(and->RealNameEntry),
                                          GTK_ENTRY_ICON_SECONDARY,
                                         "emblem-ok-symbolic");
    
    }    
    else
    {
        input = 1;
        gtk_entry_set_icon_from_icon_name(GTK_ENTRY(and->RealNameEntry),
                                          GTK_ENTRY_ICON_SECONDARY,
                                          NULL);
    }
    if(strlen(s) <= 0 )
    {
        gtk_widget_set_sensitive(and->ButtonConfirm, FALSE);
        return TRUE;
    }
    Valid = UserNameValidCheck(s,&Message);
    if(Message != NULL)
        SetLableFontType(and->LabelNameNote,"red",10,Message);
    else
    { 
        gtk_entry_set_icon_from_icon_name(GTK_ENTRY(and->UserNameEntry),
                                          GTK_ENTRY_ICON_SECONDARY,
                                         "emblem-ok-symbolic");
        
        SetLableFontType(and->LabelNameNote,"gray",10,FixedNote);
    }
    if(UnlockFlag == 0 && Valid && input == 0)
        gtk_widget_set_sensitive(and->ButtonConfirm, TRUE);
    else
        gtk_widget_set_sensitive(and->ButtonConfirm, FALSE);     
    return TRUE;
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
static const gchar *GetNewUserLang(AddNUDialog *and)
{
	if(and->nuLang != NULL)
	{
		mate_uesr_admin_log("Debug","nuLang = %s",and->nuLang);
		return and->nuLang;;
	}
    return "en_US.utf8";
}   

static void add_user_to_group(const char *name, char **groups)
{
	int groups_num = 0,i;
    GasGroup        *gas = NULL;
    GasGroupManager *manage;

	if(groups != NULL)
	{
		manage = gas_group_manager_get_default();
        gas_group_manager_list_groups(manage);
		groups_num = g_strv_length(groups);
		for(i = 0;i < groups_num; i++)
		{
            if(getgrnam (groups[i]) == NULL)
            {
                if(g_utf8_strchr(groups[i],-1,' ') != NULL)
                {
				    mate_uesr_admin_log("Warning","Configuration file error,Please delete the extra space keys");
                }    
				mate_uesr_admin_log("Warning","Configuration file error, no group %s",groups[i]);
				continue;
            }    
			gas = gas_group_manager_get_group(manage,groups[i]);
			if(gas == NULL)
			{
				mate_uesr_admin_log("Warning","Configuration file error, no group %s",groups[i]);
				continue;
			}
			gas_group_add_user_group(gas,name);
		}

	}
}
static void CloseWindow(GtkWidget *widget)
{
	gtk_widget_hide (widget);    

    UnlockFlag = 0;
}       
static void NewUserLoaded (ActUser         *user,
                           GParamSpec      *pspec,
                           AddNUDialog     *and)
{
    int         UserType;
    const char *NewUserLang;
    const char *Password;
    const char *un;
    int         PasswordType = NEWPASS;
    
	UserType = GetNewUserType(and->NewUserType);
    NewUserLang  = GetNewUserLang(and);
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(and->RadioButton2)) == TRUE)
    {        
        Password = GetNewUserPassword(and->NewPassEntry,and->CheckPassEntry);
        if(Password == NULL)
        {    
            return;
        }    
        PasswordType = OLDPASS;
    }    

    if(UserType == ADMIN)
    {        
        act_user_set_account_type(user,ACT_USER_ACCOUNT_TYPE_ADMINISTRATOR);
    }    
    act_user_set_language(user,NewUserLang);
    
    if(PasswordType == NEWPASS)
    {
        act_user_set_password_mode (user,ACT_USER_PASSWORD_MODE_SET_AT_LOGIN);
    }
    else
    {
        act_user_set_password_mode (user, ACT_USER_PASSWORD_MODE_REGULAR);
        act_user_set_password (user,Password, "");
    }
    un = gtk_entry_get_text(GTK_ENTRY(and->UserNameEntry));
	add_user_to_group(un,and->nuGroups);
    
    CloseWindow(GTK_WIDGET(and));

}


static void CreateUserDone (ActUserManager  *Manager,
							GAsyncResult    *res,
							AddNUDialog    *and)
{
    GError  *error = NULL;
	ActUser *user;

	user = act_user_manager_create_user_finish (Manager, res, &error);
    if(user == NULL)
    {
        MessageReport(_("Creating User"),error->message,ERROR);
        g_error_free(error);
		AddTimer(and);
        return;
    }   
	mate_uesr_admin_log("Debug","Created user: %s success", act_user_get_user_name (user));
    if (act_user_is_loaded (user))
		NewUserLoaded(user,NULL,and);
    else
		g_signal_connect (user, "notify::is-loaded", G_CALLBACK (NewUserLoaded), and);
        
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
static void CreateLocalNewUser(AddNUDialog *and)
{
    ActUserManager *Manager;
    const char *rn;
    const char *un;

    rn = gtk_entry_get_text(GTK_ENTRY(and->RealNameEntry));
    un = gtk_entry_get_text(GTK_ENTRY(and->UserNameEntry));
	
	RemoveTimer(and);
    Manager = act_user_manager_get_default ();
	mate_uesr_admin_log("Debug","username %s realname %s",
                        un,rn);
    act_user_manager_create_user_async (Manager,
                                        un,
                                        rn,
										ACT_USER_ACCOUNT_TYPE_STANDARD,
                                        and->cancellable,
                                        (GAsyncReadyCallback)CreateUserDone,
                                        and);
		
} 
/******************************************************************************
* Function:             SetNewUserInfo 
*        
* Explain: New user name and type and language
*        
* Input:  @CanConfig 
*         
*         TERU    Use configuration files /etc/mate-user-admin/unconfig
*         FALSE   Use default values
*        
* Output: 
*        
* Author:  zhuyaliang  09/05/2018
******************************************************************************/
static void SetNewUserInfo(GtkWidget *Vbox,AddNUDialog *and,gboolean CanConfig)
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

    and->UserNameEntry   = gtk_entry_new();
    and->CheckNameTimeId = g_timeout_add(800,(GSourceFunc)CheckName,and);
    gtk_entry_set_max_length(GTK_ENTRY(and->UserNameEntry),24);
    gtk_grid_attach(GTK_GRID(Table) ,and->UserNameEntry , 1 , 0 , 3 , 1);

    and->LabelNameNote = gtk_label_new (NULL);
    SetLableFontType(and->LabelNameNote,"gray",10,FixedNote);
    gtk_grid_attach(GTK_GRID(Table) ,and->LabelNameNote , 0 , 1, 4 , 1);

    LabelRealName = gtk_label_new(NULL);
    SetLableFontType(LabelRealName,"gray",11,_("Login Name"));
    gtk_grid_attach(GTK_GRID(Table) , LabelRealName , 0 , 2 , 1 , 1);

    and->RealNameEntry = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(and->RealNameEntry),24);
    gtk_grid_attach(GTK_GRID(Table), and->RealNameEntry , 1 , 2 , 3 , 1);
	  
    LabelUserType = gtk_label_new(NULL);
    SetLableFontType(LabelUserType,"gray",11,_("Account Type"));
    gtk_grid_attach(GTK_GRID(Table) ,LabelUserType , 0 , 3 , 1 , 1);        
     
    and->NewUserType = SetComboUserType(_("Standard"),_("Administrators"));
	if(CanConfig == TRUE)
	{
		gtk_combo_box_set_active(GTK_COMBO_BOX(and->NewUserType),and->nuType);
	}
	else
	{
		gtk_combo_box_set_active(GTK_COMBO_BOX(and->NewUserType),STANDARD);
	}
   
    gtk_grid_attach(GTK_GRID(Table) ,and->NewUserType , 1 , 3 , 3 , 1);        

    gtk_grid_set_row_spacing(GTK_GRID(Table), 10);
    gtk_grid_set_column_spacing(GTK_GRID(Table), 10);

}
static void ComparePassword (AddNUDialog *and)
{
    const gchar *password;
    const gchar *NoteMessage = _("The passwords entered twice are different");

    password = gtk_entry_get_text(GTK_ENTRY(and->CheckPassEntry));
    if(strlen(password) <=0)
        return;
    if(GetNewUserPassword(and->NewPassEntry,and->CheckPassEntry) == NULL)
    {
        gtk_entry_set_icon_from_icon_name(GTK_ENTRY(and->CheckPassEntry),
                                          GTK_ENTRY_ICON_SECONDARY,
                                          NULL);
        SetLableFontType(and->LabelSpace,"red",10,NoteMessage);
        UnlockFlag = 1;
        return;
    }    
    gtk_entry_set_icon_from_icon_name(GTK_ENTRY(and->CheckPassEntry),
                                      GTK_ENTRY_ICON_SECONDARY,
                                     "emblem-ok-symbolic");
    gtk_label_set_markup(GTK_LABEL(and->LabelSpace),NULL); 
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
    AddNUDialog *and = ADDNUDIALOG(data);
    const char  *s;
    int          Level;
    const char  *Message;
    const char  *tip = _("Hybrid passwords improve security");

    s = gtk_entry_get_text(GTK_ENTRY(and->NewPassEntry));
    if(strlen(s) == 0)
    {
        //gtk_entry_set_visibility(GTK_ENTRY(newuser->NewPassEntry),FALSE);
        SetLableFontType(and->LabelPassNote,"gray",10,tip);
        return TRUE;
    }
    Level = GetPassStrength (s, NULL,NULL,&Message);
    gtk_level_bar_set_value (GTK_LEVEL_BAR (and->LevelBar), Level);

    if(Message == NULL && Level > 1)
    {
        gtk_entry_set_icon_from_icon_name(GTK_ENTRY(and->NewPassEntry),
                                          GTK_ENTRY_ICON_SECONDARY,
                                          "emblem-ok-symbolic");
        gtk_widget_set_sensitive(and->CheckPassEntry, TRUE);
        SetLableFontType(and->LabelPassNote,"gray",10,tip);
        ComparePassword(and);
        return TRUE;
    }
    gtk_entry_set_icon_from_icon_name(GTK_ENTRY(and->NewPassEntry), 
                                      GTK_ENTRY_ICON_SECONDARY,
                                     "system-run");
    SetLableFontType(and->LabelPassNote,"red",10,Message);
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
    AddNUDialog *and = ADDNUDIALOG(data);
 
    gtk_widget_set_sensitive(and->NewPassEntry, FALSE);  //lock widget
    gtk_widget_set_sensitive(and->CheckPassEntry, FALSE);
    gtk_widget_set_sensitive(and->LevelBar, FALSE);
 
    if(and->CheckPassTimeId > 0)                //因为不需要检查密码所以移除定时器
    {
        g_source_remove(and->CheckPassTimeId);
        and->CheckPassTimeId = 0;
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
    AddNUDialog *and = ADDNUDIALOG(data);
 
    UnlockFlag = 1;
    gtk_widget_set_sensitive(and->NewPassEntry, TRUE);
    gtk_widget_set_sensitive(and->LevelBar, TRUE);

    and->CheckPassTimeId = g_timeout_add(800,(GSourceFunc)TimeFun,and);

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
static void SetNewUserPass(GtkWidget *Vbox,AddNUDialog *and)
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

    and->RadioButton1 = gtk_radio_button_new_with_label(NULL,_("Set up next time"));
    RadioGroup = gtk_radio_button_get_group(GTK_RADIO_BUTTON(and->RadioButton1));
    gtk_grid_attach(GTK_GRID(Table) , and->RadioButton1 , 0 , 1 , 5 , 1);
    g_signal_connect(and->RadioButton1,
                    "released",
                     G_CALLBACK(LoginSetPass),
                     and); 
    
    and->RadioButton2 = gtk_radio_button_new_with_label(RadioGroup,_("Now set the password"));
    gtk_grid_attach(GTK_GRID(Table) , and->RadioButton2 , 0 , 2 , 5 , 1);
    g_signal_connect(and->RadioButton2,
                    "released",
                     G_CALLBACK(NowSetNewUserPass),
                     and); 
   
    LabelPass = gtk_label_new(NULL);
    SetLableFontType(LabelPass,"gray",11,_("Password"));
    gtk_grid_attach(GTK_GRID(Table) ,LabelPass , 0 , 3 , 1 , 1);     

    and->NewPassEntry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(and->NewPassEntry),FALSE);
    gtk_entry_set_icon_from_icon_name(GTK_ENTRY(and->NewPassEntry), 
                                      GTK_ENTRY_ICON_SECONDARY,
                                     "system-run");
    gtk_entry_set_max_length(GTK_ENTRY(and->NewPassEntry),24);
    gtk_grid_attach(GTK_GRID(Table) , and->NewPassEntry , 1 , 3 , 4 , 1);
   
    gtk_entry_set_icon_tooltip_text (GTK_ENTRY(and->NewPassEntry),
                                     GTK_ENTRY_ICON_SECONDARY,
                                    _("generation password"));	
	
    and->LevelBar = gtk_level_bar_new ();
    gtk_level_bar_set_min_value(GTK_LEVEL_BAR(and->LevelBar),0.0);
    gtk_level_bar_set_max_value(GTK_LEVEL_BAR(and->LevelBar),5.0);
    gtk_level_bar_set_mode(GTK_LEVEL_BAR(and->LevelBar),GTK_LEVEL_BAR_MODE_DISCRETE);
    gtk_grid_attach(GTK_GRID(Table) ,and->LevelBar , 1 , 4 , 4 , 1);
	
    and->LabelPassNote = gtk_label_new (NULL);
    gtk_grid_attach(GTK_GRID(Table) ,and->LabelPassNote , 0 , 5 , 4 , 1);

    LabelConfirm = gtk_label_new (NULL);
    SetLableFontType(LabelConfirm,"gray",11,_("Confirm"));
    gtk_grid_attach(GTK_GRID(Table) ,LabelConfirm , 0 , 6 , 1 , 1);

    and->CheckPassEntry = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(and->CheckPassEntry),24);
    gtk_entry_set_visibility(GTK_ENTRY(and->CheckPassEntry),FALSE);
    gtk_grid_attach(GTK_GRID(Table) ,and->CheckPassEntry, 1 , 6 , 4 , 1);
    g_signal_connect (G_OBJECT(and->NewPassEntry), 
                     "icon-press", 
                      G_CALLBACK(AutoGenera), 
                      and->CheckPassEntry);
    
    and->LabelSpace = gtk_label_new("");
    gtk_grid_attach(GTK_GRID(Table) , and->LabelSpace , 0 , 7 , 4 , 1);

    gtk_widget_set_sensitive(and->NewPassEntry, FALSE);  //lock widget
    gtk_widget_set_sensitive(and->CheckPassEntry, FALSE);
    gtk_widget_set_sensitive(and->LevelBar, FALSE);   

    gtk_grid_set_row_spacing(GTK_GRID(Table), 10);
    gtk_grid_set_column_spacing(GTK_GRID(Table), 10);
}        
static void LoadHeader_bar(AddNUDialog *and)
{
    GtkWidget *Header;
    Header = gtk_header_bar_new (); 
    gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (Header), TRUE);
    gtk_header_bar_set_title (GTK_HEADER_BAR (Header), _("Create New User"));

    and->ButtonConfirm = gtk_button_new_with_label (_("Confirm"));
    gtk_header_bar_pack_end (GTK_HEADER_BAR (Header), and->ButtonConfirm);
    and->ButtonCancel = gtk_button_new_with_label ("Cancel");
    gtk_header_bar_pack_start (GTK_HEADER_BAR (Header), and->ButtonCancel);
    
    gtk_window_set_titlebar (GTK_WINDOW (and), Header);
}  
static void
add_nu_dialog_init (AddNUDialog *dialog)
{
	GtkWidget *Vbox;
    GtkWidget *Vbox1;
    GtkWidget *Vbox2;
	gboolean   ret;

	gtk_widget_set_size_request (GTK_WIDGET (dialog),500,450);
	if(GetUseHeader() == 1)
    {
        LoadHeader_bar(dialog);
    }
	else
	{
		dialog->ButtonCancel  = gtk_dialog_add_button (GTK_DIALOG (dialog), _("Cancel"), GTK_RESPONSE_CANCEL);
		dialog->ButtonConfirm = gtk_dialog_add_button (GTK_DIALOG (dialog), _("_Confirm"), GTK_RESPONSE_OK);
	}
    gtk_widget_set_sensitive(dialog->ButtonConfirm,FALSE); 
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
	gtk_widget_grab_default (dialog->ButtonConfirm);
	gtk_window_set_title (GTK_WINDOW (dialog), _("Create New User"));
    
	Vbox =  gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
    gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                        Vbox,
                        TRUE, 
                        TRUE, 
                        8);

    Vbox1 =  gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
    gtk_box_pack_start(GTK_BOX(Vbox), Vbox1,TRUE, TRUE, 0);
    Vbox2 =  gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
    gtk_box_pack_start(GTK_BOX(Vbox), Vbox2,TRUE, TRUE, 0);
	
	ret = GetNewUserConfig(dialog);
    dialog->CheckNameTimeId = 0;
    dialog->CheckPassTimeId = 0;
	SetNewUserInfo(Vbox1,dialog,ret); 
    SetNewUserPass(Vbox2,dialog);
    
}

static void GetPermission (GObject      *source_object,
                           GAsyncResult *res,
                           gpointer      data)
{
    AddNUDialog *and = ADDNUDIALOG (data);
    GError *error = NULL;

    if (g_permission_acquire_finish (and->permission, res, &error)) 
	{	
		g_return_if_fail (g_permission_get_allowed (and->permission));
        add_nu_dialog_response (GTK_DIALOG (and), GTK_RESPONSE_OK);
    } 
	else if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) 
	{
        mate_uesr_admin_log ("Warning","Failed to acquire permission: %s", error->message);
    }

    g_clear_error (&error);
    g_object_unref (and);
}


static void
add_nu_dialog_finalize (GObject *obj)
{
	AddNUDialog *self = ADDNUDIALOG (obj);

    if (self->cancellable)
                g_object_unref (self->cancellable);
    g_clear_object (&self->permission);

    G_OBJECT_CLASS (add_nu_dialog_parent_class)->finalize (obj);
}

static void
add_nu_dialog_response (GtkDialog *dialog,
                        gint response_id)
{
	AddNUDialog *and = ADDNUDIALOG (dialog);

    switch (response_id) 
	{
		case GTK_RESPONSE_OK:
			if (and->permission && !g_permission_get_allowed (and->permission)) 
			{
				g_permission_acquire_async (and->permission, 
											and->cancellable,
						                    GetPermission, 
											g_object_ref (and));
                return;
			}	

			CreateLocalNewUser (and);
			break;
		case GTK_RESPONSE_CANCEL:
		case GTK_RESPONSE_DELETE_EVENT:
            g_cancellable_cancel (and->cancellable);
            CloseWindow(GTK_WIDGET(and));
            break;
    }

}
static void
add_nu_dialog_dispose (GObject *obj)
{
	AddNUDialog *and = ADDNUDIALOG (obj);
   
	RemoveTimer(and);
	if(and->nuLang != NULL)
	{
		g_free(and->nuLang);
	}
	if(and->nuGroups != NULL)
	{
		g_free(and->nuGroups);
	}

}
static void
add_nu_dialog_class_init (AddNUDialogClass *klass)
{
	GObjectClass   *object_class = G_OBJECT_CLASS (klass);
	GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (klass);

	object_class->dispose =  add_nu_dialog_dispose;
	object_class->finalize = add_nu_dialog_finalize;

	dialog_class->response = add_nu_dialog_response;
}



AddNUDialog *Add_NUDialog_new (void)
{
	AddNUDialog *dialog;

	dialog = g_object_new(ADD_NU_TYPE_DIALOG, NULL);
	
	gtk_widget_show_all(GTK_WIDGET(dialog));
	return dialog;
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
    UserAdmin *ua = (UserAdmin *)data;

	ua->NUDialog = Add_NUDialog_new();
	
}        
