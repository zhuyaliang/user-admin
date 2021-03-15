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

#ifndef __USER_LANGUAGE_H__
#define __USER_LANGUAGE_H__

#include <gtk/gtk.h>
#include <act/act.h>

G_BEGIN_DECLS

#define TYPE_LANGUAGE_CHOOSER     (language_chooser_get_type ())
#define LANGUAGE_CHOOSER(object)   (G_TYPE_CHECK_INSTANCE_CAST ((object), \
                                   TYPE_LANGUAGE_CHOOSER, \
                                   LanguageChooser))

typedef struct LanguageChooser 
{
    GtkDialog parent_instance;

    GtkWidget     *done_button;
    GtkWidget     *cancel_button;
    GtkWidget     *no_results;
    GtkWidget     *header;
    GtkListBoxRow *more_item;
    GtkWidget     *search_bar;
    GtkWidget     *language_entry;
    GtkWidget     *language_listbox;
    gboolean       showing_extra;
    gboolean       is_header;
    gchar         *language;
    gchar        **filter_words;
    ActUser       *user;
}LanguageChooser;


typedef struct LanguageChooserClass
{
	GtkDialogClass parent_class;
} LanguageChooserClass;

GType            language_chooser_get_type     (void) G_GNUC_CONST;
LanguageChooser *language_chooser_new          (ActUser         *user);
void             language_chooser_clear_filter (LanguageChooser *chooser);
const gchar     *language_chooser_get_language (LanguageChooser *chooser);
void             language_chooser_set_language (LanguageChooser *chooser,
                                                const gchar     *language);
G_END_DECLS
#endif
