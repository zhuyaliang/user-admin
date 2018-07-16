#ifndef __USER_ADMIN_H__
#define __USER_ADMIN_H__

#include <gtk/gtk.h>

//static void RemoveUserData(int UserIndex);
void RemoveUser(GtkWidget *widget, gpointer data);
//static gboolean CheckName(gpointer data);
//static void NewUserSelectType(GtkWidget *widget,gpointer data);
//static void NewUserSelectLanguage(GtkWidget *widget,gpointer window);
//static void CreateNewUser(GtkWidget *widget,gpointer data);
//static void CloseWindow(GtkWidget *widget,gpointer data);
//static void RemoveTime(GtkWidget *widget,gpointer data);

void AddUser(GtkWidget *widget, gpointer data);

#endif
