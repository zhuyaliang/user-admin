#ifndef __USER_SHARE_H__
#define __USER_SHARE_H__

#include "user.h"

#define TYPEMSG    "<span foreground='red'font_desc='13'>%s </span>"
#define ERROR      1
#define WARING     2
#define INFOR      3
#define QUESTION   4

int Change;
 int GetCurrentLangIndex(const char *Lang);
int MessageReport(const char *Title,const char *Msg,int nType);
void SetLableFontType(GtkWidget *Lable ,const char *Color,int FontSzie,const char *Word);
void UserListAppend(GtkWidget *list,
                    const gchar *UserIcon,
                    const gchar *UserName,
                    const gchar *Color,
                    int Index,
                    GtkTreeIter *iter);
GdkPixbuf *SetUserFaceSize(const char  *PicName, int Size);
void OpenNote(GtkWidget *label,const char *note,UserAdmin *ua);
void OffNote(GtkWidget *label,UserAdmin *ua);
GtkWidget *SetComboLanguageType(void);
GtkWidget *SetComboUserType(const char *s1,const char *s2);
void UpdateInterface(int Cnt,UserAdmin *ua);
gboolean CheckPassword(gpointer data);
void SelectSetPassMode(GtkRadioButton *button,gpointer  user_data);
void AutoGenera (GtkEntry            *entry,
               GtkEntryIconPosition icon_pos,
               GdkEvent            *event,
               gpointer             user_data);

int GetPassStrength (const char  *password,
                   const char  *old_password,
                   const char  *username,
                   const char **message);



#endif
