#define _XOPEN_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <shadow.h>
#include <pwquality.h>
#include "user-password.h"
#include "user-share.h"

static void CloseNewPassWindow(GtkWidget *widget,gpointer data);

/******************************************************************************
* Function:              NextSetPass 
*        
* Explain: Set the password at the next login
*        
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  15/05/2018
******************************************************************************/
static void NextSetPass (GtkRadioButton *button,gpointer data)
{
    UserAdmin *ua = (UserAdmin *)data;
    const char *Pass = _("Set up next time");
    const char *Pass1 = _("Account is disable");
    
    memset(ua->ul[gnCurrentUserIndex].PassText,
          '\0',
          strlen(ua->ul[gnCurrentUserIndex].PassText));

    gtk_widget_set_sensitive(ua->NewPassEntry, FALSE);  //lock widget
    gtk_widget_set_sensitive(ua->CheckPassEntry, FALSE);
    gtk_widget_set_sensitive(ua->LevelBar, FALSE);
    gtk_widget_set_sensitive(ua->ButtonConfirm,TRUE);

    if(ua->CheckPassTimeId > 0)                //因为不需要检查密码所以移除定时器
    {
        g_source_remove(ua->CheckPassTimeId);
        ua->CheckPassTimeId = 0;
    }
    if (act_user_get_locked (ua->ul[gnCurrentUserIndex].User))
    {
        memcpy(ua->ul[gnCurrentUserIndex].PassText,Pass1,strlen(Pass1));
    }
    else
    {
        memcpy(ua->ul[gnCurrentUserIndex].PassText,Pass,strlen(Pass));
    }
    ua->ul[gnCurrentUserIndex].PasswordType = NEWPASS;
}        

/******************************************************************************
* Function:              NowSetPass 
*        
* Explain: Now set the password,Create a timer to check the password
*        
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  15/05/2018
******************************************************************************/
static void NowSetPass (GtkRadioButton *button,gpointer data)
{
    int CheckPassTimeId;
    const char *Pass = "●●●●●●";
    UserAdmin *ua = (UserAdmin *)data;

    memset(ua->ul[gnCurrentUserIndex].PassText,
          '\0',
          strlen(ua->ul[gnCurrentUserIndex].PassText));
    
    gtk_widget_set_sensitive(ua->CheckPassEntry, TRUE);  //Unlocking Widget
    gtk_widget_set_sensitive(ua->NewPassEntry, TRUE);
    gtk_widget_set_sensitive(ua->LevelBar, TRUE);

    CheckPassTimeId = g_timeout_add(800,(GSourceFunc)CheckPassword,ua);
    memcpy(ua->ul[gnCurrentUserIndex].PassText,Pass,strlen(Pass));
    ua->CheckPassTimeId = CheckPassTimeId;
    ua->ul[gnCurrentUserIndex].PasswordType = OLDPASS;

}        
static void ClosePassWindow(GtkWidget *widget,gpointer data)
{
    UserAdmin *ua = (UserAdmin *)data;
    gtk_widget_destroy(GTK_WIDGET(ua->PassWindow));
}
/******************************************************************************
* Function:              SetNewPass 
*        
* Explain: Confirm the change of the password
*        
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  15/05/2018
******************************************************************************/
static void SetNewPass(GtkWidget *widget,gpointer data)
{
	const char *np;
	const char *cp;
    UserAdmin *ua = (UserAdmin *)data;

    /*choose now set password*/
    if(ua->ul[gnCurrentUserIndex].PasswordType == OLDPASS)
    {        
	    np =  gtk_entry_get_text(GTK_ENTRY(ua->NewPassEntry));
	    cp =  gtk_entry_get_text(GTK_ENTRY(ua->CheckPassEntry));

	    if(strcmp(np,cp) != 0)
	    {
		    OpenNote(ua->LabelSpace,_("Inconsistent password"),ua);
	    }
	    else
	    {   
            act_user_set_password (ua->ul[gnCurrentUserIndex].User,cp, "");
            act_user_set_password_mode (ua->ul[gnCurrentUserIndex].User,
                                        ACT_USER_PASSWORD_MODE_REGULAR);
	    }
    }
    gtk_widget_destroy(ua->PassWindow);
    UpdateInterface(gnCurrentUserIndex,ua);
}

/******************************************************************************
* Function:              CreateNewPass 
*        
* Explain: Set the password for the first time
*        
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  15/05/2018
******************************************************************************/
void CreateNewPass(UserAdmin *ua)
{
	GtkWidget *WindowChangePass;
    GtkWidget *Vbox;
    GtkWidget *Table;
    GtkWidget *LabelTitle;
    GSList    *RadioGroup;
    GtkWidget *RadioButton1;
    GtkWidget *RadioButton2;
    GtkWidget *LabelPass;
    GtkWidget *NewPassEntry;
    GtkWidget *LevelBar;
    GtkWidget *LabelPassNote;
    GtkWidget *LabelConfirm;
    GtkWidget *CheckPassEntry;


    GtkWidget *Hseparator;
    GtkWidget *LabelSpace;
    GtkWidget *ButtonConfirm;
    GtkWidget *ButtonCancel;

    //新建一个窗口
    WindowChangePass = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(WindowChangePass),_("Add a password"));
    gtk_window_set_position(GTK_WINDOW(WindowChangePass),GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(WindowChangePass),400,250);
    gtk_container_set_border_width(GTK_CONTAINER(WindowChangePass),20);
    ua->PassWindow = WindowChangePass;
    g_signal_connect(WindowChangePass,
                    "destroy",
                    G_CALLBACK(CloseNewPassWindow),
                    ua);

    Vbox =  gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
    gtk_container_add(GTK_CONTAINER(WindowChangePass),Vbox);

    Table = gtk_grid_new();
    gtk_box_pack_start(GTK_BOX(Vbox), Table,TRUE, TRUE, 0);
    gtk_grid_set_column_homogeneous(GTK_GRID(Table),TRUE);
    LabelTitle = gtk_label_new(_("Password"));
    gtk_grid_attach(GTK_GRID(Table) , LabelTitle , 0 , 0 , 1 , 1);

    //Select button
    RadioButton1 = gtk_radio_button_new_with_label(NULL,_("Set up next time"));
    RadioGroup = gtk_radio_button_get_group(GTK_RADIO_BUTTON(RadioButton1));
    gtk_grid_attach(GTK_GRID(Table) , RadioButton1 , 0 , 1 , 5 , 1);
    g_signal_connect(RadioButton1,"released",G_CALLBACK(NextSetPass),ua);

    RadioButton2 = gtk_radio_button_new_with_label(RadioGroup,_("Now set up"));
    gtk_grid_attach(GTK_GRID(Table) , RadioButton2 , 0 , 2 , 5 , 1);
    g_signal_connect(RadioButton2,"released",G_CALLBACK(NowSetPass),ua);

    LabelPass = gtk_label_new(NULL);
    SetLableFontType(LabelPass,"gray",11,_("User Password"));
    gtk_grid_attach(GTK_GRID(Table) ,LabelPass , 0 , 3 , 1 , 1);

    NewPassEntry = gtk_entry_new();
    ua->NewPassEntry = NewPassEntry;
    gtk_entry_set_icon_from_icon_name(GTK_ENTRY(NewPassEntry), 
                                      GTK_ENTRY_ICON_SECONDARY,
                                      "system-run");
    gtk_entry_set_max_length(GTK_ENTRY(NewPassEntry),20);
    gtk_grid_attach(GTK_GRID(Table) , NewPassEntry , 1 , 3 , 4 , 1);
    g_signal_connect (G_OBJECT(NewPassEntry), 
                     "icon-press", 
                      G_CALLBACK(AutoGenera), 
                      ua);


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

    CheckPassEntry = gtk_entry_new();
    ua->CheckPassEntry = CheckPassEntry;
    gtk_entry_set_max_length(GTK_ENTRY(CheckPassEntry),20);
    gtk_entry_set_visibility(GTK_ENTRY(CheckPassEntry),FALSE);
    gtk_grid_attach(GTK_GRID(Table) , CheckPassEntry , 1 , 6 , 4 , 1);

    LabelSpace = gtk_label_new(NULL);
    ua->LabelSpace = LabelSpace;
    gtk_grid_attach(GTK_GRID(Table) , LabelSpace , 0 , 7 , 4 , 1);

    Hseparator = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_grid_attach(GTK_GRID(Table) , Hseparator , 0 , 8 , 5 , 1);

    ButtonConfirm = gtk_button_new_with_label(_("Confirm"));
    ua->ButtonConfirm = ButtonConfirm;
   	g_signal_connect (ButtonConfirm, 
                      "clicked",
                      G_CALLBACK (SetNewPass),
                      ua);
    gtk_grid_attach(GTK_GRID(Table) , ButtonConfirm , 0 , 9 , 1 , 1);

    ButtonCancel =  gtk_button_new_with_label(_("Cancel"));
  	g_signal_connect (ButtonCancel, 
                     "clicked",
                     G_CALLBACK (ClosePassWindow),
                     ua);
    gtk_grid_attach(GTK_GRID(Table) , ButtonCancel , 4 , 9 , 1 , 1);

    gtk_grid_set_row_spacing(GTK_GRID(Table), 10);
    gtk_grid_set_column_spacing(GTK_GRID(Table), 10);

    gtk_widget_set_sensitive(NewPassEntry, FALSE);
    gtk_widget_set_sensitive(CheckPassEntry, FALSE);
    gtk_widget_set_sensitive(LevelBar, FALSE);

    gtk_widget_show_all(WindowChangePass);
}
static void get_salt(char *salt,char *passwd)
{
    int i,j;

    for(i=0,j=0;passwd[i] && j != 3;++i)
    {
        if(passwd[i] == '$')
            ++j;
    }

    strncpy(salt,passwd,i-1);
}
/******************************************************************************
* Function:            CheckInputOldPass
*        
* Explain: Check the correctness of the old password entered
*          
* Input:   @Name user name 
*          @PassWord  old password       
*        
* Output:  -1 Password error
*        
* Author:  zhuyaliang  15/05/2018
******************************************************************************/
static int CheckInputOldPass(const char *Name,const char *PassWord)
{
    struct spwd *sp;
    char salt[512]={0};
    printf("name = %s password = %s\r\n",Name,PassWord);
    if((sp=getspnam(Name)) == NULL)
            return -1;
    printf("name = %s password = %s\r\n",Name,PassWord);
    get_salt(salt,sp->sp_pwdp);
    //进行密码验证
    if(strcmp(sp->sp_pwdp,crypt(PassWord,salt)) == 0)
         return  0;
    else
         return -1; 

}        
/******************************************************************************
* Function:            ConfirmFun
*        
* Explain: Click confirmation to change the password
*          step 1 Confirm the old password for the input
*          step 2 Calculating the strength of the new cipher
*          
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  15/05/2018
******************************************************************************/
static void ConfirmFun(GtkWidget *widget,gpointer data)
{
    UserAdmin *ua = (UserAdmin *)data;
	const char *OldPass;
	const char *NewPass;
    int nRet;
	int Level = 0;
    const char *Message;

	OldPass =  gtk_entry_get_text(GTK_ENTRY(ua->OldPassEntry));
	NewPass =  gtk_entry_get_text(GTK_ENTRY(ua->NewPassEntry));
    
    nRet = CheckInputOldPass(ua->ul[gnCurrentUserIndex].UserName,OldPass);

    if(nRet < 0)
	{
		OpenNote(ua->LabelPassNote,
               _("Please enter the correct original password"),
                ua);
	}
	else
	{
		Level = GetPassStrength (NewPass, OldPass,NULL,&Message);
    	gtk_level_bar_set_value (GTK_LEVEL_BAR (ua->LevelBar), Level);
  		if(Message == NULL && Level > 1)
  		{
  		 	act_user_set_password_mode (ua->ul[gnCurrentUserIndex].User, 
                                        ACT_USER_PASSWORD_MODE_REGULAR);
            act_user_set_password (ua->ul[gnCurrentUserIndex].User,NewPass , "");
            gtk_widget_destroy(GTK_WIDGET(ua->PassWindow));
  		}
  		else
  		{
  			OpenNote(ua->LabelPassNote,Message,ua);
  		}

	}
	gtk_widget_set_sensitive(ua->ButtonConfirm, TRUE);
}
static void CloseOldPassWindow(  GtkWidget *widget,gpointer data)
{
    UserAdmin *ua = (UserAdmin *)data;
    gtk_widget_set_sensitive(ua->MainWindow,TRUE);
}

/******************************************************************************
* Function:            ChangeOldPass
*        
* Explain: Change the old password 
*          
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  15/05/2018
******************************************************************************/
static void CloseNewPassWindow(GtkWidget *widget,gpointer data)
{
    UserAdmin *ua = (UserAdmin *)data;
    //清除检查密码是否合理的定时器
    if(ua->CheckPassTimeId > 0)
    {
        g_source_remove(ua->CheckPassTimeId);
        ua->CheckPassTimeId = 0;
    }
    gtk_widget_set_sensitive(ua->MainWindow,TRUE);
}        
/******************************************************************************
* Function:            ChangeOldPass
*        
* Explain: Change the old password 
*          
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  15/05/2018
******************************************************************************/
void ChangeOldPass(UserAdmin *ua)
{
	GtkWidget *WindowChangePass;
    GtkWidget *Vbox;
    GtkWidget *Table;

    GtkWidget *LabelTitle;
    GtkWidget *LabelPass;
    GtkWidget *NewPassEntry;
    GtkWidget *OldPassEntry;
    GtkWidget *LevelBar;
    GtkWidget *LabelPassNote;
    GtkWidget *LabelConfirm;

    GtkWidget *Hseparator;
    GtkWidget *LabelSapce;
    GtkWidget *ButtonConfirm;
    GtkWidget *ButtonCancel;

    //create window
    WindowChangePass = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(WindowChangePass),_("Change password"));
    gtk_window_set_position(GTK_WINDOW(WindowChangePass),GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(WindowChangePass),400,200);
    gtk_container_set_border_width(GTK_CONTAINER(WindowChangePass),20);
    ua->PassWindow = WindowChangePass;
    g_signal_connect(WindowChangePass,
                    "destroy",
                    G_CALLBACK(CloseOldPassWindow),
                    ua);

    Vbox =  gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
    gtk_container_add(GTK_CONTAINER(WindowChangePass),Vbox);

    Table = gtk_grid_new();
    gtk_box_pack_start(GTK_BOX(Vbox), Table,TRUE, TRUE, 0);
    gtk_grid_set_column_homogeneous(GTK_GRID(Table),TRUE);
    
    LabelTitle = gtk_label_new(_("Password"));  
    gtk_grid_attach(GTK_GRID(Table) , LabelTitle , 0 , 0 , 1 , 1);

    /*input old password*/
    LabelPass = gtk_label_new(NULL);
    SetLableFontType(LabelPass,"gray",11,_("Old Password"));
    gtk_grid_attach(GTK_GRID(Table) ,LabelPass , 0 , 3 , 1 , 1);

    OldPassEntry = gtk_entry_new();
    ua->OldPassEntry = OldPassEntry;
    gtk_entry_set_max_length(GTK_ENTRY(OldPassEntry),20);
    gtk_entry_set_visibility(GTK_ENTRY(OldPassEntry),FALSE);
    gtk_grid_attach(GTK_GRID(Table) , OldPassEntry , 1 , 3 , 4 , 1);

    /*input new password*/
   	LabelConfirm = gtk_label_new (NULL);
    SetLableFontType(LabelConfirm,"gray",11,_("New Password"));
    gtk_grid_attach(GTK_GRID(Table) ,LabelConfirm , 0 , 4 , 1 , 1);

    NewPassEntry = gtk_entry_new();
    ua->NewPassEntry = NewPassEntry;
    gtk_entry_set_max_length(GTK_ENTRY(NewPassEntry),20);
    gtk_entry_set_visibility(GTK_ENTRY(NewPassEntry),FALSE);
    gtk_grid_attach(GTK_GRID(Table) , NewPassEntry , 1 , 4 , 4 , 1);


    /*Display cipher strength*/
    LevelBar = gtk_level_bar_new ();
    gtk_level_bar_set_min_value(GTK_LEVEL_BAR(LevelBar),0.0);
    gtk_level_bar_set_max_value(GTK_LEVEL_BAR(LevelBar),5.0);
    gtk_level_bar_set_mode(GTK_LEVEL_BAR(LevelBar),GTK_LEVEL_BAR_MODE_DISCRETE);
    ua->LevelBar = LevelBar;
 	gtk_grid_attach(GTK_GRID(Table) ,LevelBar , 1 , 5 , 4 , 1);

    /*Problem reminding*/
    LabelPassNote = gtk_label_new (NULL);
    ua->LabelPassNote = LabelPassNote;
    gtk_grid_attach(GTK_GRID(Table) ,LabelPassNote , 0 , 6 , 4 , 1);

    /*Space*/
    LabelSapce = gtk_label_new("");
    gtk_grid_attach(GTK_GRID(Table) , LabelSapce , 0 , 7 , 4 , 1);
    Hseparator = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_grid_attach(GTK_GRID(Table) , Hseparator , 0 , 8 , 5 , 1);
    
    /*confirm button*/
    ButtonConfirm = gtk_button_new_with_label(_("Confirm"));
    ua->ButtonConfirm = ButtonConfirm;

  	g_signal_connect (ButtonConfirm, "clicked",G_CALLBACK (ConfirmFun),ua);
    gtk_grid_attach(GTK_GRID(Table) , ButtonConfirm , 0 , 9 , 1 , 1);

    /*cancel button*/
    ButtonCancel =  gtk_button_new_with_label(_("Cancel"));
    g_signal_connect (ButtonCancel, 
                     "clicked",
                      G_CALLBACK (ClosePassWindow),
                      ua);
    gtk_grid_attach(GTK_GRID(Table) , ButtonCancel , 4 , 9 , 1 , 1);

    gtk_grid_set_row_spacing(GTK_GRID(Table), 10);
    gtk_grid_set_column_spacing(GTK_GRID(Table), 10);

    gtk_widget_show_all(WindowChangePass);
}

