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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <shadow.h>
#include <pwquality.h>
#include <act/act-user.h>
#include "user-share.h"
#include "user-info.h"

#define  GSM_GSETTINGS_SCHEMA "org.mate.interface"
static int file_dp = 0;  /*Log file descriptor*/

int Change;

static int create_log_file(void)
{
    time_t t;
    int    t_stamp;
    char  *file_name;

    if(file_dp > 0)
    {
        return file_dp;
    }    
    t_stamp = time(&t);
    file_name = g_strdup_printf("/tmp/mate-user-admin-%d.log",t_stamp);

    file_dp = open(file_name,O_RDWR|O_CREAT,0666);
    
    g_free(file_name);    
    return file_dp;
}
GdkPixbuf *GetRoundPixbuf (GdkPixbuf *Spixbuf)
{
    GdkPixbuf *Dpixbuf = NULL;
    cairo_surface_t *CairoFace;
    cairo_t   *Cairo;
    int        PixSize;

    PixSize = gdk_pixbuf_get_width (Spixbuf);
    CairoFace = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, PixSize,PixSize);
    Cairo = cairo_create (CairoFace);

    cairo_arc (Cairo, PixSize/2, PixSize/2, PixSize/2, 0, 2 * G_PI);
    cairo_clip (Cairo);
    cairo_new_path (Cairo);
    gdk_cairo_set_source_pixbuf (Cairo,Spixbuf,0,0);
    cairo_paint (Cairo);

    Dpixbuf = gdk_pixbuf_get_from_surface (CairoFace,0,0,PixSize,PixSize);
    cairo_surface_destroy (CairoFace);
    cairo_destroy (Cairo);

    return Dpixbuf;
}
/*Record error information to log file*/
void mate_uesr_admin_log(const char *level,const char *message,...)
{
    int     fd;
    va_list args;
    char   *file_data;
    char    buf[1024]; 
    int     len;

    fd = create_log_file();
    if(fd < 0)
    {
        return;
    }    
    va_start(args,message);
    vsprintf(buf,message, args);
    va_end(args);
    file_data = g_strdup_printf("level:[%s]  message: %s\r\n",level,buf);
    len = write(fd,file_data,strlen(file_data));
    if(len <= 0)
    {
        MessageReport("write log","write log error",ERROR);	
    }
	g_free (file_data);
}    

void close_log_file (void)
{
    if(file_dp > 0 )
    {
        close(file_dp);
        file_dp = 0;
    }    
}    
gboolean GetUseHeader(void)
{
    GSettings *settings;
    const char *key = "gtk-dialogs-use-header";

    settings = g_settings_new (GSM_GSETTINGS_SCHEMA);
    return g_settings_get_boolean (settings, key);
}    
/******************************************************************************
* Function:            MessageReport
*        
* Explain: Prompt information dialog
*          
* Input:  @Title           Message title
*         @Msg             Message content           
*         @nType           Message type
* Output: 
*        
* Author:  zhuyaliang  25/05/2018
******************************************************************************/
int MessageReport(const char *Title,const char *Msg,int nType)
{
    GtkWidget *dialog = NULL;
    int nRet;

    switch(nType)
    {
        case ERROR:
        {
            dialog = gtk_message_dialog_new(GTK_WINDOW(WindowLogin),
                                            GTK_DIALOG_DESTROY_WITH_PARENT,
                                            GTK_MESSAGE_ERROR,
                                            GTK_BUTTONS_OK,
                                            "%s",Title);
            break;
        }
        case WARING:
        {
            dialog = gtk_message_dialog_new(GTK_WINDOW(WindowLogin),
                                            GTK_DIALOG_DESTROY_WITH_PARENT,
                                            GTK_MESSAGE_WARNING,
                                            GTK_BUTTONS_OK,
                                            "%s",Title);
            break;
        }
        case INFOR:
        {
            dialog = gtk_message_dialog_new(GTK_WINDOW(WindowLogin),
                                            GTK_DIALOG_DESTROY_WITH_PARENT,
                                            GTK_MESSAGE_INFO,
                                            GTK_BUTTONS_OK,
                                            "%s",Title);
            break;
        }
        case QUESTION:
        {
            dialog = gtk_message_dialog_new(GTK_WINDOW(WindowLogin),
                                            GTK_DIALOG_DESTROY_WITH_PARENT,
                                            GTK_MESSAGE_QUESTION,
                                            GTK_BUTTONS_YES_NO,
                                            "%s",Title);
            gtk_dialog_add_button (GTK_DIALOG (dialog),_("_Return"),
                                   GTK_RESPONSE_ACCEPT);
            break;
        }
        case QUESTIONNORMAL:
        {
            dialog = gtk_message_dialog_new(GTK_WINDOW(WindowLogin),
                                            GTK_DIALOG_DESTROY_WITH_PARENT,
                                            GTK_MESSAGE_QUESTION,
                                            GTK_BUTTONS_YES_NO,
                                            "%s",Title);
            break;
        }    
        default :
            break;

    }
    gtk_message_dialog_format_secondary_markup(GTK_MESSAGE_DIALOG(dialog),
                                               TYPEMSG,
                                               Msg);
    gtk_window_set_title(GTK_WINDOW(dialog),_("Message"));
    nRet =  gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    return nRet;
}

/******************************************************************************
* Function:            SetLableFontType
*        
* Explain: Tips the style of the text
*          
* Input:  @GtkWidget       lable
*         @Color           
*         @FontSize
*         @Word            text
* Output: 
*        
* Author:  zhuyaliang  25/05/2018
******************************************************************************/
void SetLableFontType(GtkWidget  *Lable ,
                      const char *Color,
                      int         FontSzie,
                      const char *Word,
                      gboolean    Blod)
{
    char *LableTypeBuf;

    if(Blod)
    {
        LableTypeBuf = g_strdup_printf ("<span foreground=\'%s\'weight=\'light\'font_desc=\'%d\'><b>%s</b></span>",
                         Color,
                         FontSzie,
                         Word);
    }
    else
    {
        LableTypeBuf = g_strdup_printf("<span foreground=\'%s\'weight=\'light\'font_desc=\'%d\'>%s</span>",
                        Color,
                        FontSzie,
                        Word);

    }
    gtk_label_set_markup(GTK_LABEL(Lable),LableTypeBuf);
    g_free(LableTypeBuf);
}

/******************************************************************************
* Function:             UserListAppend
*
* Explain:  Add information
*
* Input:
*        @list        list
*        @UserIcon    icon path
*        @UserName    real name
*        @Color       color
*        @Index       user lable
*        Iter
* Output:
*
* AUTHOR:  zhuyaliang  09/05/2018
******************************************************************************/
void UserListAppend(GtkWidget   *list,
                    const gchar *UserIcon,
                    const gchar *RealName,
                    const gchar *UserName,
                    int          Index,
                    GtkTreeIter *Iter)
{
    GtkListStore *store;
    GtkTreeIter   iter;
    GdkPixbuf    *face;
    char         *label;

    /*set user icon size 50 * 50 */
    face = SetUserFaceSize (UserIcon, 50);
    
    store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(list)));
    label = g_markup_printf_escaped ("<big><b>%s</b>\n<span color=\'dark grey\'><i>%s</i></span></big>",
                                      RealName,UserName);

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter,COL_USER_FACE, face,  //icon 
                                    INT_COLUMN,Index,     //count
                                    LIST_LABEL,label,     //two name
                                    -1);   
    *Iter = iter;
    g_free (label);
}
/******************************************************************************
* 函数:              SetUserFaceSize   
*        
* 说明:  设置照片尺寸
*        
* 输入:  		
*        @PicName  照片名字
*        @Size     尺寸
*        
* 返回:  
*        
* 作者:  zhuyaliang  09/05/2018
******************************************************************************/ 
GdkPixbuf * SetUserFaceSize (const char  *PicName, int Size)
{
    g_autoptr(GdkPixbuf) Spixbuf = NULL;
    GdkPixbuf *Dpixbuf = NULL;
    
    if(PicName == NULL)
    {
        Spixbuf = gdk_pixbuf_new_from_file_at_size (DEFAULT, Size, Size, NULL);
    }
    else
    {
        Spixbuf = gdk_pixbuf_new_from_file_at_size (PicName, Size, Size, NULL);
    }
    
    Dpixbuf = GetRoundPixbuf(Spixbuf);
    
    return Dpixbuf;
}

/******************************************************************************
* Function:              UpdateInterface 
*        
* Explain: Switching user information
*        
* Input:  @Cnt  user index.       
*        
* Output:  
*        
* Author:  zhuyaliang  09/05/2018
******************************************************************************/
void UpdateInterface(ActUser *user,UserAdmin *ua)
{
    const char      *lang_id;
    g_autofree char *lang_cc = NULL;
    g_autofree const char *text = NULL;
    int        passtype;
    gboolean   is_authorized;
    gboolean   self_selected;

    /*Some options change when switching users, 
      causing a signal response, requiring flags, 
      and ignoring signal processing when Chang = 1*/
    Change = 1;

    is_authorized = g_permission_get_allowed (G_PERMISSION (ua->Permission));
    self_selected = act_user_get_uid (user) == geteuid ();

    /*Switching icon*/
    user_face_update (ua->face, GetUserIcon(user), GetRealName(user));

    mate_uesr_admin_log("Info","mate-user-admin Current user name %s",GetRealName(user));
    gtk_combo_box_set_active(GTK_COMBO_BOX(ua->ComUserType),
                             GetUserType(user));
    lang_id = GetUserLang(user);
    if(lang_id == NULL)
    {
        gtk_button_set_label(GTK_BUTTON(ua->ButtonLanguage),
                             _("No Settings"));
    }
    else
    {
        lang_cc = mate_get_language_from_locale(lang_id,NULL);
        gtk_button_set_label(GTK_BUTTON(ua->ButtonLanguage),
                             lang_cc);
    }
    gtk_button_set_label(GTK_BUTTON(ua->ButtonPass),
                         GetPasswordModeText(user,&passtype));

    gtk_switch_set_state(GTK_SWITCH(ua->SwitchAutoLogin),
                         GetUserAutoLogin(user));

    text = GetLoginTimeText(user);
    gtk_button_set_label (GTK_BUTTON(ua->ButtonUserTime),
                          text);

    if(self_selected == 0)
    {
        gtk_widget_set_sensitive(GTK_WIDGET (ua->face),is_authorized);
        gtk_widget_set_sensitive(ua->ButtonUserTime, is_authorized);
        gtk_widget_set_sensitive(ua->ButtonUserGroup,is_authorized);
    }
    else if(is_authorized == 0 && self_selected == 1)
    {
        gtk_widget_set_sensitive(GTK_WIDGET (ua->face),self_selected);
        gtk_widget_set_sensitive(ua->ButtonUserTime, self_selected);
        gtk_widget_set_sensitive(ua->ButtonUserGroup,self_selected);
    }
    Change = 0;
}
static pwquality_settings_t * get_pwq (void)
{
    static pwquality_settings_t *settings;

    if (settings == NULL)
    {
        char *err = NULL;
        settings = pwquality_default_settings ();
        pwquality_set_int_value (settings, PWQ_SETTING_MIN_LENGTH , 8);
        if (pwquality_read_config (settings, NULL, (gpointer)&err) < 0)
       {
            return NULL;
        }
    }

    return settings;
}

static int pw_min_length (void)
{
    int value = 0;
    if (pwquality_get_int_value (get_pwq (), PWQ_SETTING_MIN_LENGTH , &value) < 0)
    {
        return -1;
    }
    return value;
}
static const gchar *pw_error_hint (gint error)
{
    switch (error) {
    case PWQ_ERROR_SAME_PASSWORD:
        return _("The new password needs to be different from the old one");
    case PWQ_ERROR_CASE_CHANGES_ONLY:
        return _("Try changing some letters and numbers");
    case PWQ_ERROR_TOO_SIMILAR:
        return _("Try changing the password a bit more");
    case PWQ_ERROR_USER_CHECK:
        return _("A password without your user name would be stronger");
    case PWQ_ERROR_GECOS_CHECK:
        return _("Try to avoid using your name in the password");
    case PWQ_ERROR_BAD_WORDS:
        return _("Try to avoid some of the words included in the password");
    case PWQ_ERROR_ROTATED:
        return _("Try changing the password a bit more");
    case PWQ_ERROR_CRACKLIB_CHECK:
        return _("Try to avoid common words");
    case PWQ_ERROR_PALINDROME:
        return _("Try to avoid reordering existing words");
    case PWQ_ERROR_MIN_DIGITS:
        return _("Try to use more numbers");
    case PWQ_ERROR_MIN_UPPERS:
        return _("Try to use more uppercase letters");
    case PWQ_ERROR_MIN_LOWERS:
        return _("Try to use more lowercase letters");
    case PWQ_ERROR_MIN_OTHERS:
        return _("Try to use more special characters, like punctuation");
    case PWQ_ERROR_MIN_CLASSES:
        return _("Try to use a mixture of letters, numbers and punctuation");
    case PWQ_ERROR_MAX_CONSECUTIVE:
        return _("Try to avoid repeating the same character");
    case PWQ_ERROR_MAX_CLASS_REPEAT:
        return _("Try to avoid repeating the same type of character: you need to mix up letters, numbers and punctuation.");
    case PWQ_ERROR_MAX_SEQUENCE:
        return _("Try to avoid sequences like 1234 or abcd");
    case PWQ_ERROR_MIN_LENGTH:
        return _("Password length needs more than 8 characters");
    case PWQ_ERROR_EMPTY_PASSWORD:
        return ("     ");
    default:
        return NULL;
    }
}

/******************************************************************************
* Function:            GetPassStrength
*        
* Explain: Get the strength of the new password
*          
* Input:   @password        new password
*          @old_password    old  
*          @username        name
*          @message         Return password message
*
* Output: strength
*        
* Author:  zhuyaliang  15/05/2018
******************************************************************************/
int GetPassStrength (const char  *password,
                     const char  *old_password,
                     const char  *username,
                     const char **message)
{
    int rv, level, length = 0;
    double strength = 0.0;
    void *auxerror;

    rv = pwquality_check (get_pwq (),
                          password, 
                          old_password, 
                          username,
                          &auxerror);

    if (password != NULL)
        length = strlen (password);
    strength = 0.01 * rv;

    if (rv < 0)
    {
        level = (length > 0) ? 1 : 0;
    }
    else if (strength < 0.50)
    {
        level = 2;
    }
    else if (strength < 0.75)
    {
        level = 3;
    }
    else if (strength < 0.90)
    {
        level = 4;
    }
    else
    {
        level = 5;
    }

    if ( length < pw_min_length())
    {        
    	*message = pw_error_hint (PWQ_ERROR_MIN_LENGTH);
         level = 0;
    }
    else
    {        
        *message = pw_error_hint (rv);
    }    
    return level;
}
/******************************************************************************
* Function:              AutoGenera 
*        
* Explain: Automatic production of new ciphers
*        
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  15/05/2018
******************************************************************************/
void AutoGenera (GtkEntry            *entry,
                 GtkEntryIconPosition icon_pos,
                 GdkEvent            *event,
                 gpointer             data)
{

    char *NewPassWord;

    gtk_entry_set_visibility(GTK_ENTRY(entry),TRUE);
    pwquality_generate (get_pwq (), 0, &NewPassWord);
    gtk_entry_set_text(GTK_ENTRY(entry),NewPassWord);
    gtk_entry_set_text(GTK_ENTRY(data),NewPassWord);

}

/******************************************************************************
* Function:              SetComboUserType 
*        
* Explain: create drop-down select boxes,Standard and Administrators
*        
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  09/05/2018
******************************************************************************/
GtkWidget *SetComboUserType(const char *s1,const char *s2)
{
    GtkListStore    *Store;
    GtkTreeIter     Iter;
    GtkCellRenderer *Renderer;
    GtkWidget *ComboUser;

    Store = gtk_list_store_new(1,G_TYPE_STRING);
    gtk_list_store_append(Store,&Iter);
    gtk_list_store_set(Store,&Iter,0,s1,-1);
    gtk_list_store_append(Store,&Iter);
    gtk_list_store_set(Store,&Iter,0,s2,-1);

    ComboUser = gtk_combo_box_new_with_model(GTK_TREE_MODEL(Store));
    g_object_unref(G_OBJECT(Store));
    Renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(ComboUser),Renderer,TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(ComboUser),Renderer,"text",0,NULL);

    return ComboUser;
}

GtkWidget *SetButtonIcon(const gchar *button_text,const gchar *icon_name)
{
    GtkWidget *button;
    GtkWidget *image;
    GtkStyleContext *Context;

    button = gtk_button_new_with_mnemonic (button_text);
    image  = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image (GTK_BUTTON (button), image);
    gtk_button_set_use_underline (GTK_BUTTON (button), TRUE);
    Context = gtk_widget_get_style_context (button);
    gtk_style_context_add_class (Context, "text-button");
    gtk_widget_set_can_default (button, TRUE);

    return button;
}

GtkWidget* dialog_add_button_with_icon_name (GtkDialog   *dialog,
                                             const gchar *button_text,
                                             const gchar *icon_name,
                                             gint         response_id)
{
	GtkWidget *button;

	button = gtk_button_new_with_mnemonic (button_text);
	gtk_button_set_image (GTK_BUTTON (button), gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_BUTTON));

	gtk_button_set_use_underline (GTK_BUTTON (button), TRUE);
	gtk_style_context_add_class (gtk_widget_get_style_context (button), "text-button");
	gtk_widget_set_can_default (button, TRUE);
	gtk_widget_show (button);
	gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button, response_id);

	return button;
}
