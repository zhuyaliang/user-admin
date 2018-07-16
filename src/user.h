#ifndef __USER_H__
#define __USER_H__

#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <pwd.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#define GNOME_DESKTOP_USE_UNSTABLE_API
#include <gnome-desktop-3.0/libgnome-desktop/gnome-languages.h>

#include <accountsservice-1.0/act/act-user-manager.h>
#include <accountsservice-1.0/act/act-user.h>
#include <libintl.h> // gettext 库支持  
#include <locale.h> // 本地化locale的翻译支持  

#define  DEFAULT    "/usr/share/pixmaps/isoft-faces/Default.jpg"
#define  NUMMAX    20
#define  PICMAX    20    
#define  NONE      3
#define  ENGLIST   1
#define  CHINSE    0


#define  OLDPASS   0
#define  NEWPASS   1

#define  STANDARD  0
#define  ADMIN     1
#define _(STRING)  gettext(STRING) //为了减少代码量和输入量，用别名_(STRING)替换gettext(STRONG)  
#define PACKAGE    "user-admin" // 定义软件包的名字为hello  
#define LOCALEDIR  "/usr/share/locale/" //定义系统语言环境的目录，该目录下存放各种国际化的语言，不同系统可能有差异。

enum
{
    COL_USER_FACE= 0,
    INT_COLUMN,
    LIST_TEXT ,
    LIST_COLOR,
    LIST_FRONT,
    N_COLUMNS
};
/*用户属性结构体*/

typedef  struct
{
    char        UserName[48];   //user name
    char        RealName[48];   //real name
    char        HomeName[48];   //user home directory
    char        UserIcon[128];  //user icon path
    char        UserType;       //user type admin or standard
    char        LangType;       //language type temporary support for English and Chinese
    int         PasswordType;   //passwod type 
    char        PassText[48];   //user login password
    gboolean    LoginType;      //user automatic logon
    char        UserTime[48];   //user login time
    GtkTreeIter Iter;           //user list iter
    ActUser     *User;         
}UserInfoList;    
typedef struct 
{
    UserInfoList     ul[NUMMAX];
    GtkWidget        *MainWindow;
    GtkWidget        *IconWindow;
    GtkWidget        *PassWindow;
    GtkWidget        *AddUserWindow;
    GtkTreeModel     *Model;
    GtkTreeSelection *UserSelect;
    GtkWidget        *UserList;
    GtkListStore     *ListSTore;
    GtkWidget        *ButtonFace;
    GtkWidget        *EntryName;
    GtkWidget        *ComUserType;
    GtkWidget        *ComUserLanguage;
    GtkWidget        *ButtonPass;
    GtkWidget        *SwitchAutoLogin;
    GtkWidget        *ButtonUserTime;
    GtkWidget        *NewPassEntry;
    GtkWidget        *OldPassEntry;
    GtkWidget        *LabelPassNote;
    GtkWidget        *LevelBar;
    GtkWidget        *ButtonConfirm;
    GtkWidget        *CheckPassEntry;
    GtkWidget        *LabelSpace;
    GtkWidget        *RealNameEntry;
    GtkWidget        *UserNameEntry;
    GtkWidget        *LabelNameNote;
    GtkWidget        *NewUserType;
    GtkWidget        *NewUserLangType;
    int               UserCount;    //Valid user number
    int               CheckPassTimeId;//Check the password format timer
    int               CheckNameTimeId; //Check the Realname format timer
    char              TmpPass[24];
}UserAdmin;
static const char StandardCharacter[] = {"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_"};
int gnCnt;                //计数
int gnCurrentUserIndex;   //代表当前用户标号
GtkWidget *WindowLogin;          //首页窗口

#endif
