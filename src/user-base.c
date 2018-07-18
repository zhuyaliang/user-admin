#include "user-base.h"
#include "user-password.h"
#include "user-share.h"

/******************************************************************************
* 函数:           SwitchState       
*        
* 说明: 点击切换登录状态时响应函数
*        
* 输入:  		
*        
*        
* 返回:  
*        
* 作者:  zhuyaliang  09/05/2018
******************************************************************************/ 
static void SwitchState(GtkSwitch *widget,gboolean   state,gpointer  data)
{

    GSList *list;
    GSList *l;
    int i =0 ;
    UserAdmin *ua = (UserAdmin *)data;
    
    if(Change == 0)
    {        
        ActUser *user = ua->ul[gnCurrentUserIndex].User;
        ActUserManager *um =  act_user_manager_get_default ();
        if(state == TRUE)
        {
            list = act_user_manager_list_users (um);
            for (l = list; l != NULL; l = l->next,i++)
            {
                ActUser *u = l->data;
                ua->ul[i].LoginType = FALSE;
                if (act_user_get_uid (u) != act_user_get_uid (user)) 
                {
                    act_user_set_automatic_login (user, FALSE);
                }
            }
            g_slist_free (list);
            act_user_set_automatic_login(user,TRUE);
        }
        else
            act_user_set_automatic_login(user,FALSE);
        
        ua->ul[gnCurrentUserIndex].LoginType = state; 

    }
}    
/******************************************************************************
* 函数:            ChangePass       
*        
* 说明: 点击密码时响应函数，有两种情况，1 就密码修改2设置新密码
*        
* 输入:  		
*        
*        
* 返回:  
*        
* 作者:  zhuyaliang  09/05/2018
******************************************************************************/ 
static void ChangePass(GtkWidget *widget,gpointer data)
{
    UserAdmin *ua = (UserAdmin *)data;
	//const char *s = gtk_button_get_label(GTK_BUTTON(widget));
    gtk_widget_set_sensitive(ua->MainWindow,FALSE);

    if(ua->ul[gnCurrentUserIndex].PasswordType == OLDPASS)
		ChangeOldPass(ua);      //old passwoed change
	else
		CreateNewPass(ua);      //There is no password for the user

}
/******************************************************************************
* 函数:              ComboSelectLanguage       
*        
* 说明: 点击选择用户语言类型下拉框时响应函数
*        
* 输入:  		
*        
*        
* 返回:  
*        
* 作者:  zhuyaliang  09/05/2018
******************************************************************************/ 
static void ComboSelectLanguage(GtkWidget *widget,gpointer data)
{
    UserAdmin *ua = (UserAdmin *)data;
    gint account_type;
   
    if(Change == 0) 
    {        
        account_type =  gtk_combo_box_get_active (GTK_COMBO_BOX(widget));
        ua->ul[gnCurrentUserIndex].LangType = account_type;
        if(account_type == 1)
            act_user_set_language(ua->ul[gnCurrentUserIndex].User,"en_US.UTF-8");
        else
            act_user_set_language(ua->ul[gnCurrentUserIndex].User,"zh_CN.UTF-8");
    }     
}
/******************************************************************************
* 函数:              ComboSelectUserType       
*        
* 说明: 点击选择用户类型下拉框时响应函数
*        
* 输入:  		
*        
*        
* 返回:  
*        
* 作者:  zhuyaliang  09/05/2018
******************************************************************************/ 
static void ComboSelectUserType(GtkWidget *widget,gpointer data)
{
    UserAdmin *ua = (UserAdmin *)data;
    gint account_type;

    if( Change ==0 )
    {
        account_type =  gtk_combo_box_get_active (GTK_COMBO_BOX(widget)) ? 
                                                  ACT_USER_ACCOUNT_TYPE_ADMINISTRATOR:
                                                  ACT_USER_ACCOUNT_TYPE_STANDARD;
        ua->ul[gnCurrentUserIndex].UserType = account_type;
        act_user_set_account_type(ua->ul[gnCurrentUserIndex].User,account_type);
    }    
}
/******************************************************************************
* 函数:              DisplayUserSetOther       
*        
* 说明: 显示用户帐号类型、语言、密码、自动登录、时间
*        
* 输入:  		
*        左侧布局盒
*        
* 返回:  
*        
* 作者:  zhuyaliang  09/05/2018
******************************************************************************/ 
void DisplayUserSetOther(GtkWidget *Hbox,UserAdmin *ua)
{
    GtkWidget *table;
    GtkWidget *fixed;
    GtkWidget *ButtonPass;
    GtkWidget *LabelUserType;
    GtkWidget *LabelAutoLogin;
    GtkWidget *LabelLanguage;
    GtkWidget *SwitchLogin;
    GtkWidget *LabelPass;
    GtkWidget *ComboLanguage;
    GtkWidget *LabelTime;
    GtkWidget *ButtonTime;
    GtkWidget *ComboUser;

    fixed = gtk_fixed_new();
    gtk_box_pack_start(GTK_BOX(Hbox),fixed ,TRUE, TRUE, 0);
    table = gtk_grid_new();
    gtk_grid_set_column_homogeneous(GTK_GRID(table),TRUE);
    gtk_fixed_put(GTK_FIXED(fixed), table, 0, 0);

    /*账户类型*/
    LabelUserType = gtk_label_new(NULL);
    SetLableFontType(LabelUserType,"gray",11,_("Account Type"));
    gtk_grid_attach(GTK_GRID(table) , LabelUserType , 0 , 0 , 1 , 1);

    /*设置用户类型下拉框*/
    ComboUser = SetComboUserType(_("Standard"),_("Administrators"));
    ua->ComUserType = ComboUser; 
    gtk_combo_box_set_active(GTK_COMBO_BOX(ComboUser),ua->ul[0].UserType);
    gtk_grid_attach(GTK_GRID(table) , ComboUser , 1 , 0 , 2 , 1);
    g_signal_connect(G_OBJECT(ComboUser),"changed",G_CALLBACK(ComboSelectUserType),ua);

   /*语言*/ 
    LabelLanguage = gtk_label_new(NULL);
    SetLableFontType(LabelLanguage,"gray",11,_("Language"));
    gtk_grid_attach(GTK_GRID(table) , LabelLanguage , 0 , 1 , 1 , 1);

    ComboLanguage = SetComboLanguageType(_("Chinese"),_("English"));
    ua->ComUserLanguage = ComboLanguage;
    gtk_combo_box_set_active(GTK_COMBO_BOX(ComboLanguage),ua->ul[0].LangType);
    gtk_grid_attach(GTK_GRID(table) , ComboLanguage , 1 , 1 , 2 , 1);
    g_signal_connect(G_OBJECT(ComboLanguage),"changed",G_CALLBACK(ComboSelectLanguage),ua);
    
    /*密码*/
    LabelPass = gtk_label_new(NULL);
    SetLableFontType(LabelPass,"gray",11,_("Password"));
    gtk_grid_attach(GTK_GRID(table) , LabelPass , 0 , 2 , 1 , 1);
    
    ButtonPass = gtk_button_new_with_label(ua->ul[0].PassText);
    ua->ButtonPass = ButtonPass;
    g_signal_connect (ButtonPass, "clicked",G_CALLBACK (ChangePass),ua);
    gtk_grid_attach(GTK_GRID(table) , ButtonPass , 1 , 2 , 2 , 1);
   
    /*登录*/
    LabelAutoLogin = gtk_label_new(NULL);
    SetLableFontType(LabelAutoLogin,"gray",11,_("Automatic logon"));
    gtk_grid_attach(GTK_GRID(table) , LabelAutoLogin , 0 , 3 , 1 , 1);
    
    SwitchLogin = gtk_switch_new();
    ua->SwitchAutoLogin = SwitchLogin;
    gtk_switch_set_state (GTK_SWITCH(SwitchLogin),ua->ul[0].LoginType);
    gtk_grid_attach(GTK_GRID(table) , SwitchLogin , 1 , 3 , 1 , 1);
    g_signal_connect(G_OBJECT(SwitchLogin),"state-set",G_CALLBACK(SwitchState),ua);
    
    /*最近登录时间*/
    LabelTime = gtk_label_new(NULL);
    SetLableFontType(LabelTime,"gray",11,_("Login time"));
    gtk_grid_attach(GTK_GRID(table) , LabelTime, 0 , 4 , 1 , 1);
  
    ButtonTime = gtk_button_new_with_label (ua->ul[0].UserTime);
    ua->ButtonUserTime = ButtonTime;
    gtk_grid_attach(GTK_GRID(table) , ButtonTime, 1 , 4 , 2 , 1);
  
    gtk_grid_set_row_spacing(GTK_GRID(table), 10);
    gtk_grid_set_column_spacing(GTK_GRID(table), 10);
}
