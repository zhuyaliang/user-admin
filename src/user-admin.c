#include "user.h"
#include "user-share.h"
#include "user-admin.h"
#include <pwd.h>
#include <sys/types.h>

GtkWidget *WindowAddUser;
static int UnlockFlag;
/******************************************************************************
* 函数:           RemoveUserData       
*        
* 说明:  清楚记录用户信息的结构体内容
*        
* 输入:  @UserIdex  被清除用户标号
*        
*        
* 返回:  
*        
* 作者:  zhuyaliang  09/05/2018
******************************************************************************/ 
static void RemoveUserData(UserAdmin *ua,int UserIndex)
{
    memset(ua->ul[UserIndex].UserName,'\0',strlen(ua->ul[UserIndex].UserName));            
    memset(ua->ul[UserIndex].RealName,'\0',strlen(ua->ul[UserIndex].RealName));            
    memset(ua->ul[UserIndex].HomeName,'\0',strlen(ua->ul[UserIndex].HomeName));            
    memset(ua->ul[UserIndex].UserIcon,'\0',strlen(ua->ul[UserIndex].UserIcon)); 
    memset(ua->ul[UserIndex].PassText,'\0',strlen(ua->ul[UserIndex].PassText));
    memset(ua->ul[UserIndex].UserTime,'\0',strlen(ua->ul[UserIndex].UserTime));
}
/******************************************************************************
* 函数:           RemoveUser       
*        
* 说明: 点击移除用户按键响应函数
*        
* 输入:         
*        
*        
* 返回:  
*        
* 作者:  zhuyaliang  09/05/2018
******************************************************************************/ 
void RemoveUser(GtkWidget *widget, gpointer data)
{
    UserAdmin *ua = (UserAdmin *)data;
    GtkTreeIter iter;
    int Index;
    int nRet;
    int RemoveCount = 0;
    gboolean RemoveType = TRUE;
	ActUserManager *Manager;
    GError *error = NULL;
    
    Manager = act_user_manager_get_default ();
    if (gtk_tree_selection_get_selected(ua->UserSelect, &ua->Model, &iter))
    {
        Index = gnCurrentUserIndex;  // gnCurrentUserIndex 代表当前用户标号
        nRet = MessageReport(_("Remove User"),
                      _("Whether to remove the user's home directory"),
                      QUESTION);
        if(nRet == GTK_RESPONSE_NO)
        {
             RemoveType = FALSE;
        }
        while(!act_user_manager_delete_user(Manager,ua->ul[Index].User,RemoveType,&error))
        {
            error = NULL;
            if(RemoveCount > 5)  //有时候此函数不能一次成功需要多次调用
            {   
                MessageReport(_("Remove User"),_("Remove user failure and try again"),ERROR); 
                break;
            }    
        }
        if(RemoveCount < 5)    //5次操作内成功移除用户
        {    
            gtk_list_store_remove (GTK_LIST_STORE (ua->Model), &iter);//在列表中移除用户
            RemoveUserData(ua,Index);                                //清除用户信息
            UpdateInterface(gnCurrentUserIndex,ua);                  //
            ua->UserCount--;                                        //用户总个数-1
        }
    }

}        

static gboolean CheckUserNameUsed (const gchar *UserName)
{
        struct passwd *pwent;

        if (UserName == NULL || UserName[0] == '\0') {
                return FALSE;
        }

        pwent = getpwnam (UserName);

        return pwent != NULL;
}
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
            *Message = g_strdup (_("The username should only consist of upper and lower case \r\nletters from a-z,digits and the following characters: . - _"));
        }
    }
    else 
    {
        *Message = g_strdup (_("This will be used to name your home folder and cant be changed."));
    }

    return valid;
}
        
        
/******************************************************************************
* 函数:           CheckName       
*        
* 说明: 定时器响应函数800ms进入一次，检查用户输入的用户名是否合法
*        
* 输入:         
*        
*        
* 返回:  
*        
* 作者:  zhuyaliang  09/05/2018
******************************************************************************/ 
static gboolean CheckName(gpointer data)
{
    UserAdmin *ua = (UserAdmin *)data;
    gboolean Valid;
    char *Message = NULL;
    const char *s;
    const char *p;

    p = gtk_entry_get_text(GTK_ENTRY(ua->RealNameEntry));
    if(strlen(p) <= 0)
        return TRUE;
    s = gtk_entry_get_text(GTK_ENTRY(ua->UserNameEntry));
    Valid = UserNameValidCheck(s,&Message);
    SetLableFontType(ua->LabelNameNote,"red",10,Message);
    
    if(UnlockFlag == 0 && Valid)
        gtk_widget_set_sensitive(ua->ButtonConfirm, TRUE);
    else
        gtk_widget_set_sensitive(ua->ButtonConfirm, FALSE);     
    return TRUE;
}

/******************************************************************************
* 函数:           NewUserSelectType       
*        
* 说明:  新用户选择用户类型响应函数
*        
* 输入:         
*        
*        
* 返回:  
*        
* 作者:  zhuyaliang  09/05/2018
******************************************************************************/ 
static void NewUserSelectType(GtkWidget *widget,gpointer data)
{
    UserAdmin *ua = (UserAdmin *)data;
 	char *text;
    GtkTreeIter   iter;
    GtkTreeModel *model;
    gint account_type;
    int NewUserIndex;

    if( gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter ) )
    {
        model = gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
        gtk_tree_model_get( model, &iter, 0, &text, -1 );
    }
    account_type =  gtk_combo_box_get_active (GTK_COMBO_BOX(widget)) ?
                                              ACT_USER_ACCOUNT_TYPE_ADMINISTRATOR : 
                                              ACT_USER_ACCOUNT_TYPE_STANDARD;
    NewUserIndex = ua->UserCount;
    ua->ul[NewUserIndex].UserType = account_type;
    g_free(text);
}
/******************************************************************************
* 函数:           NewUserSelectLanguage      
*        
* 说明:  新用户选择用户语言响应函数
*        
* 输入:         
*        
*        
* 返回:  
*        
* 作者:  zhuyaliang  09/05/2018
******************************************************************************/ 
static void NewUserSelectLanguage(GtkWidget *widget,gpointer data)
{
    UserAdmin *ua = (UserAdmin *)data;
    char *text;
    GtkTreeIter   iter;
    GtkTreeModel *model;
    gint account_type;
    int NewUserIndex;

    if( gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter ) )
    {
        model = gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
        gtk_tree_model_get( model, &iter, 0, &text, -1 );
    }
    account_type =  gtk_combo_box_get_active (GTK_COMBO_BOX(widget));
    NewUserIndex = ua->UserCount;
    ua->ul[NewUserIndex].LangType = account_type;
    g_free(text);
}
/******************************************************************************
* 函数:           WriteUserInfo      
*        
* 说明:  新用户将信息写入系统
*        
* 输入:         
*        
*        
* 返回:  成功   0
*        失败   -1
*        
* 作者:  zhuyaliang  09/05/2018
******************************************************************************/ 
static int WriteUserInfo(int index,UserAdmin *ua)
{
	ActUserManager *Manager;
    GError *error = NULL;
    ActUser *user;
    gint lang_index;

    Manager = act_user_manager_get_default ();
    user = act_user_manager_create_user(Manager,
                                       ua->ul[index].UserName,
                                       ua->ul[index].UserName,
                                       ACT_USER_ACCOUNT_TYPE_STANDARD,
                                       &error);
    if(user == NULL)
    {
        MessageReport(_("Creating User"),_("Creating a user failed"),ERROR);
        return -1;
    }        
    ua->ul[index].User = user;
    if(ua->ul[index].UserType == ADMIN)
    {        
        act_user_set_account_type(user,ACT_USER_ACCOUNT_TYPE_ADMINISTRATOR);
    }    

    lang_index = ua->ul[index].LangType;
    act_user_set_language(user, all_languages[lang_index]);
    
    if(ua->ul[index].PasswordType == NEWPASS)
    {
        act_user_set_password_mode (user,ACT_USER_PASSWORD_MODE_SET_AT_LOGIN);
    }
    else
    {
        act_user_set_password_mode (user, ACT_USER_PASSWORD_MODE_REGULAR);
        act_user_set_password (user,ua->TmpPass, "");
    }
    return 0;

}        
/******************************************************************************
* 函数:           CreateNewUser      
*        
* 说明:  新用户创建点击确认按钮响应函数
*        
* 输入:         
*        
*        
* 返回:  
*        
* 作者:  zhuyaliang  09/05/2018
******************************************************************************/ 
static void CreateNewUser(GtkWidget *widget,gpointer data)
{
    UserAdmin *ua = (UserAdmin *)data;
	const char *rn;
	const char *un;
	const char *p;
	const char *s1;
    const char *NoteMessage = _("Two inconsistencies in cipher input");
    int NewUserIndex;

    NewUserIndex = ua->UserCount;
    if(ua->ul[NewUserIndex].PasswordType == OLDPASS)
    {        
	    s1 = gtk_entry_get_text(GTK_ENTRY(ua->NewPassEntry));   
	    p =  gtk_entry_get_text(GTK_ENTRY(ua->CheckPassEntry));
        
        memset(ua->TmpPass,'\0',sizeof(ua->TmpPass));
        memcpy(ua->TmpPass,p,strlen(p));
	    if(strcmp(s1,p) != 0 )
	    {
            SetLableFontType(ua->LabelSpace,"red",10,NoteMessage);
            return;
	    }
    }    
	rn = gtk_entry_get_text(GTK_ENTRY(ua->RealNameEntry));
	un = gtk_entry_get_text(GTK_ENTRY(ua->UserNameEntry));
	memcpy(ua->ul[NewUserIndex].UserName,un,strlen(un));
	memcpy(ua->ul[NewUserIndex].RealName,rn,strlen(rn));
	UserListAppend(ua->UserList,
                   ua->ul[NewUserIndex].UserIcon,
                   ua->ul[NewUserIndex].RealName,
                  "black",
                   NewUserIndex,
                  &ua->ul[NewUserIndex].Iter);
    UnlockFlag = 0;
	gtk_widget_destroy(GTK_WIDGET(ua->AddUserWindow));

    if(WriteUserInfo(NewUserIndex,ua) >= 0)
    {    
        UpdateInterface(NewUserIndex,ua);
	    ua->UserCount +=1;//用户个数加1
    }    

} 
/******************************************************************************
* 函数:           CloseWindow     
*        
* 说明:  点击添加用户界面取消按钮响应函数
*        
* 输入:         
*        
*        
* 返回:  
*        
* 作者:  zhuyaliang  09/05/2018
******************************************************************************/ 
static void CloseWindow(GtkWidget *widget,gpointer data)
{
    UserAdmin *ua = (UserAdmin *)data;
	gtk_widget_destroy(GTK_WIDGET(ua->AddUserWindow));
}
static void InitNewUser(UserAdmin *ua)
{
    int NewUserIndex;
    const char *s = _("Set up next time");

    NewUserIndex = ua->UserCount;

    memset(ua->ul[NewUserIndex].UserIcon,'\0',sizeof(ua->ul[NewUserIndex].UserIcon));
    memcpy(ua->ul[NewUserIndex].UserIcon,DEFAULT,strlen(DEFAULT));
    ua->ul[NewUserIndex].UserType = STANDARD;
    ua->ul[NewUserIndex].LangType = 0;
    ua->ul[NewUserIndex].PasswordType = NEWPASS;
    memset(ua->ul[NewUserIndex].PassText,'\0',sizeof(ua->ul[NewUserIndex].PassText));
    memcpy(ua->ul[NewUserIndex].PassText,s,strlen(s));
    ua->ul[NewUserIndex].LoginType = FALSE;

    ua->CheckNameTimeId = 0;
    ua->CheckPassTimeId = 0;

}        
/******************************************************************************
* 函数:           RemoveTime      
*        
* 说明: 关闭窗口响应函数，移除定时器
*        
* 输入:         
*        
*        
* 返回:  
*        
* 作者:  zhuyaliang  09/05/2018
******************************************************************************/ 
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
/******************************************************************************
* 函数:           GetNewUserInfo       
*        
* 说明: 添加用户，用户输入信息界面
*        
* 输入:         
*        
*        
* 返回:  
*        
* 作者:  zhuyaliang  09/05/2018
******************************************************************************/ 
static void GetNewUserInfo(GtkWidget *Vbox,UserAdmin *ua)
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
    int       TimeId;
 const char *FixedNote = _("This will be used to name your home folder and can't be changed");   

    Table = gtk_grid_new();
    gtk_box_pack_start(GTK_BOX(Vbox), Table,TRUE, TRUE, 0);
    gtk_grid_set_column_homogeneous(GTK_GRID(Table),TRUE);

    LabelRealName = gtk_label_new(NULL);
    SetLableFontType(LabelRealName,"gray",11,_("Real Name"));
    gtk_grid_attach(GTK_GRID(Table) , LabelRealName , 0 , 0 , 1 , 1);

    RealNameEntry = gtk_entry_new();
    ua->RealNameEntry = RealNameEntry;
    gtk_entry_set_max_length(GTK_ENTRY(RealNameEntry),24);
    gtk_grid_attach(GTK_GRID(Table) , RealNameEntry , 1 , 0 , 3 , 1);

    LabelUserName = gtk_label_new(NULL);
    SetLableFontType(LabelUserName,"gray",11,_("User Name"));
    gtk_grid_attach(GTK_GRID(Table) , LabelUserName , 0 , 1 , 1 , 1);

    UserNameEntry = gtk_entry_new();
    ua->UserNameEntry = UserNameEntry;
 	TimeId = g_timeout_add(800,(GSourceFunc)CheckName,ua);
    ua->CheckNameTimeId = TimeId;
    gtk_entry_set_max_length(GTK_ENTRY(UserNameEntry),24);
    gtk_grid_attach(GTK_GRID(Table) ,UserNameEntry , 1 , 1 , 3 , 1);

    LabelNameNote = gtk_label_new (NULL);
    SetLableFontType(LabelNameNote,"gray",10,FixedNote);
    ua->LabelNameNote = LabelNameNote;
    gtk_grid_attach(GTK_GRID(Table) ,LabelNameNote , 0 , 2, 4 , 1);
    	  
    LabelUserType = gtk_label_new(NULL);
    SetLableFontType(LabelUserType,"gray",11,_("Account Type"));
    gtk_grid_attach(GTK_GRID(Table) ,LabelUserType , 0 , 3 , 1 , 1);        
    
    NewUserType = SetComboUserType(_("Standard"),_("Administrators"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(NewUserType),STANDARD);
    ua->NewUserType = NewUserType; 
    g_signal_connect(G_OBJECT(NewUserType),
                    "changed",
                     G_CALLBACK(NewUserSelectType),
                     ua);
    gtk_grid_attach(GTK_GRID(Table) ,NewUserType , 1 , 3 , 3 , 1);        
   
    LabelLanguageType = gtk_label_new(NULL);
    SetLableFontType(LabelLanguageType,"gray",11,_("Language"));
    gtk_grid_attach(GTK_GRID(Table) ,LabelLanguageType , 0 , 4 , 1 , 1);        

    NewLanguage = SetComboLanguageType();
    gtk_combo_box_set_active(GTK_COMBO_BOX(NewLanguage), 0);
    ua->NewUserLangType = NewLanguage;
    g_signal_connect(G_OBJECT(NewLanguage),
                    "changed",
                     G_CALLBACK(NewUserSelectLanguage),
                     ua);
    gtk_grid_attach(GTK_GRID(Table) ,NewLanguage , 1 , 4 , 3 , 1);        

    gtk_grid_set_row_spacing(GTK_GRID(Table), 10);
    gtk_grid_set_column_spacing(GTK_GRID(Table), 10);

}
static gboolean TimeFun(gpointer data)
{
    UserAdmin *ua = (UserAdmin *)data;
    const char *s;
    int Level;
    const char *Message;

    s = gtk_entry_get_text(GTK_ENTRY(ua->NewPassEntry));
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

static void LoginSetPass(GtkRadioButton *button,gpointer data)
{
    UserAdmin *ua = (UserAdmin *)data;
    const char *Pass = _("Set up next time");
    int NewUserIndex;
 
    NewUserIndex = ua->UserCount;
    memset(ua->ul[NewUserIndex].PassText,
          '\0',
          sizeof(ua->ul[NewUserIndex].PassText));
 
    gtk_widget_set_sensitive(ua->NewPassEntry, FALSE);  //lock widget
    gtk_widget_set_sensitive(ua->CheckPassEntry, FALSE);
    gtk_widget_set_sensitive(ua->LevelBar, FALSE);
 
    if(ua->CheckPassTimeId > 0)                //因为不需要检查密码所以移除定时器
    {
        g_source_remove(ua->CheckPassTimeId);
        ua->CheckPassTimeId = 0;
    }
    memcpy(ua->ul[NewUserIndex].PassText,Pass,strlen(Pass));
    ua->ul[NewUserIndex].PasswordType = NEWPASS;
    UnlockFlag = 0;

}
static void NowSetNewUserPass(GtkRadioButton *button,gpointer data)
{
    int CheckPassTimeId;
    const char *Pass = "●●●●●●";
    UserAdmin *ua = (UserAdmin *)data;
    int NewUserIndex;
 
    UnlockFlag = 1;
    NewUserIndex = ua->UserCount;
    memset(ua->ul[NewUserIndex].PassText,
          '\0',
          sizeof(ua->ul[NewUserIndex].PassText));

    gtk_widget_set_sensitive(ua->CheckPassEntry, TRUE);  //Unlocking Widget
    gtk_widget_set_sensitive(ua->NewPassEntry, TRUE);
    gtk_widget_set_sensitive(ua->LevelBar, TRUE);

    CheckPassTimeId = g_timeout_add(800,(GSourceFunc)TimeFun,ua);
    memcpy(ua->ul[NewUserIndex].PassText,Pass,strlen(Pass));
    ua->CheckPassTimeId = CheckPassTimeId;
    ua->ul[NewUserIndex].PasswordType = OLDPASS;

}        
/******************************************************************************
* 函数:           GetNewUserPass       
*        
* 说明: 添加用户，用户设置密码界面
*        
* 输入:         
*        
*        
* 返回:  
*        
* 作者:  zhuyaliang  09/05/2018
******************************************************************************/ 
static void GetNewUserPass(GtkWidget *Vbox,UserAdmin *ua)
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
    gtk_box_pack_start(GTK_BOX(Vbox), Table,TRUE, TRUE, 0);
    gtk_grid_set_column_homogeneous(GTK_GRID(Table),TRUE);
    LabelTitle = gtk_label_new(_("Password"));
    gtk_grid_attach(GTK_GRID(Table) , LabelTitle , 0 , 0 , 1 , 1);

    //新建两个单选按钮

    RadioButton1 = gtk_radio_button_new_with_label(NULL,_("Set up next time"));
    RadioGroup = gtk_radio_button_get_group(GTK_RADIO_BUTTON(RadioButton1));
    gtk_grid_attach(GTK_GRID(Table) , RadioButton1 , 0 , 1 , 5 , 1);
    g_signal_connect(RadioButton1,"released",G_CALLBACK(LoginSetPass),ua); 
    
    RadioButton2 = gtk_radio_button_new_with_label(RadioGroup,_("Now set the password"));
    gtk_grid_attach(GTK_GRID(Table) , RadioButton2 , 0 , 2 , 5 , 1);
    g_signal_connect(RadioButton2,"released",G_CALLBACK(NowSetNewUserPass),ua); 
   
    LabelPass = gtk_label_new(NULL);
    SetLableFontType(LabelPass,"gray",11,_("Password"));
    gtk_grid_attach(GTK_GRID(Table) ,LabelPass , 0 , 3 , 1 , 1);     

    NewEntryPass = gtk_entry_new();
    ua->NewPassEntry = NewEntryPass;
    gtk_entry_set_icon_from_icon_name(GTK_ENTRY(NewEntryPass), 
                                      GTK_ENTRY_ICON_SECONDARY,
                                      "system-run");
    gtk_entry_set_max_length(GTK_ENTRY(NewEntryPass),24);
    gtk_grid_attach(GTK_GRID(Table) , NewEntryPass , 1 , 3 , 4 , 1);
    g_signal_connect (G_OBJECT(NewEntryPass), 
                     "icon-press", 
                      G_CALLBACK(AutoGenera), ua);
	
	
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
    g_signal_connect (ButtonConfirm, "clicked",G_CALLBACK (CreateNewUser),ua);
    gtk_grid_attach(GTK_GRID(Table) , ButtonConfirm , 0 , 9 , 1 , 1);
    
    ButtonCancel =  gtk_button_new_with_label(_("Cancel"));
    g_signal_connect (ButtonCancel, "clicked",G_CALLBACK (CloseWindow),ua);
    gtk_grid_attach(GTK_GRID(Table) , ButtonCancel , 4 , 9 , 1 , 1);
    
    gtk_grid_set_row_spacing(GTK_GRID(Table), 10);
    gtk_grid_set_column_spacing(GTK_GRID(Table), 10);
}        
/******************************************************************************
* 函数:           AddUser       
*        
* 说明: 点击添加用户按键响应函数
*        
* 输入:         
*        
*        
* 返回:  
*        
* 作者:  zhuyaliang  09/05/2018
******************************************************************************/ 
void AddUser(GtkWidget *widget, gpointer data)
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
    
    InitNewUser(ua);

    GetNewUserInfo(Vbox1,ua); 
   
    GetNewUserPass(Vbox2,ua);

    gtk_widget_show_all(WindowAddUser);
}        


