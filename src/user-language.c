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
#include "user.h"
#include "user-language.h"
#include "user-share.h"
#include "user-info.h"
#include <fontconfig/fontconfig.h>

G_DEFINE_TYPE (LanguageChooser, language_chooser, GTK_TYPE_DIALOG)
#define IS_CDM_UCS4(c) (((c) >= 0x0300 && (c) <= 0x036F)  || \
                        ((c) >= 0x1DC0 && (c) <= 0x1DFF)  || \
                        ((c) >= 0x20D0 && (c) <= 0x20FF)  || \
                        ((c) >= 0xFE20 && (c) <= 0xFE2F))

#define IS_SOFT_HYPHEN(c) ((c) == 0x00AD)

static char *
normalize_fold_and_unaccent (const char *str)
{
    g_autofree gchar *normalized = NULL;
    gchar *tmp;
    int i = 0, j = 0, ilen;

    if (str == NULL)
        return NULL;

    normalized = g_utf8_normalize (str, -1, G_NORMALIZE_NFKD);
    tmp = g_utf8_casefold (normalized, -1);

    ilen = strlen (tmp);

    while (i < ilen)
    {
        gunichar unichar;
        gchar *next_utf8;
        gint utf8_len;

      /* Get next character of the word as UCS4 */
        unichar = g_utf8_get_char_validated (&tmp[i], -1);

      /* Invalid UTF-8 character or end of original string. */
        if (unichar == (gunichar) -1 ||
          unichar == (gunichar) -2)
        {
            break;
            break;
        }

      /* Find next UTF-8 character */
        next_utf8 = g_utf8_next_char (&tmp[i]);
        utf8_len = next_utf8 - &tmp[i];

        if (IS_CDM_UCS4 (unichar) || IS_SOFT_HYPHEN (unichar))
        {
            i += utf8_len;
            continue;
        }

        if (i != j)
        {
            memmove (&tmp[j], &tmp[i], utf8_len);
        }

      /* Update both indexes */
        i += utf8_len;
        j += utf8_len;
    }

  /* Force proper string end */
    tmp[j] = '\0';

    return tmp;
}

static void language_chooser_dispose (GObject *object)
{
    LanguageChooser *chooser = LANGUAGE_CHOOSER (object);
    
    g_clear_object (&chooser->no_results);
    g_clear_pointer (&chooser->filter_words, g_strfreev);
    g_clear_pointer (&chooser->language, g_free);

    G_OBJECT_CLASS (language_chooser_parent_class)->dispose (object);
}

void language_chooser_class_init (LanguageChooserClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->dispose = language_chooser_dispose;

}

static GtkListBoxRow * more_widget_new (void)
{
    GtkWidget *box, *row;
    GtkWidget *arrow;

    row = gtk_list_box_row_new ();
    box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_container_add (GTK_CONTAINER (row), box);
    gtk_widget_set_tooltip_text (box, _("More…"));

    arrow = gtk_image_new_from_icon_name ("view-more-symbolic", GTK_ICON_SIZE_MENU);
    gtk_style_context_add_class (gtk_widget_get_style_context (arrow), "dim-label");
    gtk_widget_set_margin_top (box, 10);
    gtk_widget_set_margin_bottom (box, 10);
    gtk_box_pack_start (GTK_BOX (box), arrow, TRUE, TRUE, 0);

    return GTK_LIST_BOX_ROW (row);
}

static GtkWidget *GetHeaderbar(void)
{
    GtkWidget *header;
    
    header = gtk_header_bar_new (); 
    gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (header), FALSE);
    gtk_header_bar_set_title (GTK_HEADER_BAR (header), _("Language"));
    gtk_header_bar_set_has_subtitle (GTK_HEADER_BAR (header), FALSE);

    return header;
}   
static GtkWidget *
padded_label_new (char *text, gboolean narrow)
{
    GtkWidget *widget; 

    widget = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign (widget, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_top (widget, 10); 
    gtk_widget_set_margin_bottom (widget, 10);
    gtk_widget_set_margin_start (widget, narrow ? 10 : 80);
    gtk_widget_set_margin_end (widget, narrow ? 10 : 80);
    gtk_box_pack_start (GTK_BOX (widget), gtk_label_new (text), FALSE, FALSE, 0);

    return widget;
}
static GtkWidget * no_results_widget_new (void)
{
    GtkWidget *widget;

    widget = padded_label_new (_("No languages found"), TRUE);
    gtk_widget_set_sensitive (widget, FALSE);
    return widget;
}
static gint
sort_languages (GtkListBoxRow *a,
                GtkListBoxRow *b,
                gpointer   data)
{
    const gchar *la;
    const gchar *lb;

    if (g_object_get_data (G_OBJECT (a), "locale-id") == NULL)
        return 1;
    if (g_object_get_data (G_OBJECT (b), "locale-id") == NULL)
        return -1;

    la = g_object_get_data (G_OBJECT (a), "locale-name");
    lb = g_object_get_data (G_OBJECT (b), "locale-name");

    return g_strcmp0 (la, lb);
}
static gboolean
match_all (gchar       **words,
           const gchar  *str)
{
    gchar **w;

    for (w = words; *w; ++w)
    {
        if (!strstr (str, *w))
            return FALSE;
    }
    return TRUE;
}

static gboolean language_visible (GtkListBoxRow *row,
                  gpointer   user_data)
{
    LanguageChooser *chooser = user_data;
    g_autofree gchar *locale_name = NULL;
    g_autofree gchar *locale_current_name = NULL;
    g_autofree gchar *locale_untranslated_name = NULL;
    gboolean is_extra;
    gboolean visible;

    if (row == chooser->more_item)
        return !chooser->showing_extra;

    is_extra = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (row), "is-extra"));

    if (!chooser->showing_extra && is_extra)
        return FALSE;

    if (!chooser->filter_words)
        return TRUE;

    locale_name =
        normalize_fold_and_unaccent (g_object_get_data (G_OBJECT (row), "locale-name"));
    visible = match_all (chooser->filter_words, locale_name);
    if (visible)
            return TRUE;

    locale_current_name =
        normalize_fold_and_unaccent (g_object_get_data (G_OBJECT (row), "locale-current-name"));
    visible = match_all (chooser->filter_words, locale_current_name);
    if (visible)
        return TRUE;

    locale_untranslated_name =
        normalize_fold_and_unaccent (g_object_get_data (G_OBJECT (row), "locale-untranslated-name"));
    return match_all (chooser->filter_words, locale_untranslated_name);
}
static void
cc_list_box_update_header_func (GtkListBoxRow *row,
                                GtkListBoxRow *before,
                                gpointer user_data)
{
    GtkWidget *current;

    if (before == NULL)
    {
        gtk_list_box_row_set_header (row, NULL);
        return;
    }

    current = gtk_list_box_row_get_header (row);
    if (current == NULL)
    {
        current = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
        gtk_widget_show (current); 
        gtk_list_box_row_set_header (row, current);
    }
}
static void
insert_language (GHashTable *ht,
                 const char *lang)
{
    g_autofree gchar *label_own_lang = NULL;
    g_autofree gchar *label_current_lang = NULL;
    g_autofree gchar *label_untranslated = NULL;

    label_own_lang = mate_get_language_from_locale (lang, lang);
    label_current_lang = mate_get_language_from_locale (lang, NULL);
    label_untranslated = mate_get_language_from_locale (lang, "C");

    if (g_strcmp0 (label_own_lang, label_untranslated) == 0) 
    {
        if (g_strcmp0 (label_current_lang, label_untranslated) == 0)
            g_hash_table_insert (ht, g_strdup (lang), g_strdup (label_untranslated));
        else
            g_hash_table_insert (ht, g_strdup (lang), g_strdup (label_current_lang));
    } 
    else 
    {
        g_hash_table_insert (ht, g_strdup (lang), g_strdup (label_own_lang));
    }
}
static GHashTable *
cc_common_language_get_initial_languages (void)
{
    GHashTable *ht;

    ht = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

    insert_language (ht, "en_US.UTF-8");
    insert_language (ht, "en_GB.UTF-8");
    insert_language (ht, "de_DE.UTF-8");
    insert_language (ht, "fr_FR.UTF-8");
    insert_language (ht, "es_ES.UTF-8");
    insert_language (ht, "zh_CN.UTF-8");
    insert_language (ht, "ja_JP.UTF-8");
    insert_language (ht, "ru_RU.UTF-8");
    insert_language (ht, "ar_EG.UTF-8");

    return ht;
}
static gboolean
cc_common_language_has_font (const gchar *locale)
{
    const FcCharSet  *charset;
    FcPattern        *pattern;
    FcObjectSet      *object_set;
    FcFontSet        *font_set;
    g_autofree gchar *language_code = NULL;
    gboolean          is_displayable;

    is_displayable = FALSE;
    pattern = NULL;
    object_set = NULL;
    font_set = NULL;

    if (!mate_parse_locale (locale, &language_code, NULL, NULL, NULL))
        return FALSE;

    charset = FcLangGetCharSet ((FcChar8 *) language_code);
    if (!charset) 
    {
        is_displayable = TRUE;
    }
    else 
    {
        pattern = FcPatternBuild (NULL, FC_LANG, FcTypeString, language_code, NULL);
        if (pattern == NULL)
            goto done;

        object_set = FcObjectSetCreate ();

        if (object_set == NULL)
            goto done;

        font_set = FcFontList (NULL, pattern, object_set);

        if (font_set == NULL)
            goto done;

        is_displayable = (font_set->nfont > 0);
    }

done:
    if (font_set != NULL)
        FcFontSetDestroy (font_set);

    if (object_set != NULL)
        FcObjectSetDestroy (object_set);

    if (pattern != NULL)
        FcPatternDestroy (pattern);

    return is_displayable;
}

static GtkWidget *
language_widget_new (const gchar *locale_id,
                     const gchar *current_locale_id,
                     gboolean     is_extra)
{
    GtkWidget *row;
    GtkWidget *check;
    GtkWidget *box;
    gchar     *locale_name;
    gchar     *locale_current_name;
    gchar     *locale_untranslated_name;

    locale_name = mate_get_language_from_locale (locale_id,locale_id);
    locale_current_name = mate_get_language_from_locale (locale_id, NULL);
    locale_untranslated_name = mate_get_language_from_locale (locale_id, "C");

    row = gtk_list_box_row_new ();
    box = padded_label_new (locale_name, is_extra);
    gtk_container_add (GTK_CONTAINER (row), box);

    check = gtk_image_new ();
    gtk_image_set_from_icon_name (GTK_IMAGE (check), "object-select-symbolic", GTK_ICON_SIZE_MENU);
    gtk_widget_set_opacity (check, 0.0);
    g_object_set (check, "icon-size", GTK_ICON_SIZE_MENU, NULL);
    gtk_box_pack_start (GTK_BOX (box), check, FALSE, FALSE, 0);
    gtk_box_reorder_child (GTK_BOX (box), check, 0);

    check = gtk_image_new ();
    gtk_image_set_from_icon_name (GTK_IMAGE (check), "object-select-symbolic", GTK_ICON_SIZE_MENU);
    gtk_widget_set_opacity (check, 0.0);
    g_object_set (check, "icon-size", GTK_ICON_SIZE_MENU, NULL);
    gtk_box_pack_start (GTK_BOX (box), check, FALSE, FALSE, 0);
    if (g_strcmp0 (locale_id, current_locale_id) == 0)
        gtk_widget_set_opacity (check, 1.0);

    g_object_set_data (G_OBJECT (row), "check", check);
    g_object_set_data_full (G_OBJECT (row), 
                           "locale-id", 
                            g_strdup (locale_id), 
                            g_free);
    g_object_set_data_full (G_OBJECT (row), 
                           "locale-name", 
                            locale_name, 
                            g_free);
    g_object_set_data_full (G_OBJECT (row), 
                           "locale-current-name", 
                            locale_current_name, 
                            g_free);
    g_object_set_data_full (G_OBJECT (row), 
                           "locale-untranslated-name", 
                            locale_untranslated_name, 
                            g_free);
    g_object_set_data (G_OBJECT (row), "is-extra", GUINT_TO_POINTER (is_extra));

    return row;
}
static void
add_languages (LanguageChooser *chooser,
               gchar            **locale_ids,
               GHashTable        *initial)
{
    gchar *locale_id;
    gboolean is_initial;
    GtkWidget *widget;
    
    while (*locale_ids) 
    {
        locale_id = *locale_ids;
        locale_ids ++;
        if (!cc_common_language_has_font (locale_id))
                continue;

        is_initial = (g_hash_table_lookup (initial, locale_id) != NULL);
        widget = language_widget_new (locale_id, chooser->language, !is_initial);
        gtk_container_add (GTK_CONTAINER (chooser->language_listbox), widget);
    }

    gtk_container_add (GTK_CONTAINER (chooser->language_listbox), GTK_WIDGET (chooser->more_item));

    gtk_widget_show_all (chooser->language_listbox);
}
static void
add_all_languages (LanguageChooser *chooser)
{
    gchar **locale_ids;
    GHashTable *initial;

    locale_ids = mate_get_all_locales ();
    initial = cc_common_language_get_initial_languages ();
    add_languages (chooser, locale_ids, initial);
    g_hash_table_destroy (initial);
    g_strfreev (locale_ids);
}
static void
filter_changed (LanguageChooser *chooser)
{
    g_autofree gchar *filter_contents = NULL;

    g_clear_pointer (&chooser->filter_words, g_strfreev);

    filter_contents =
        normalize_fold_and_unaccent (gtk_entry_get_text (GTK_ENTRY (chooser->language_entry)));
    if (!filter_contents) 
    {
        gtk_list_box_invalidate_filter (GTK_LIST_BOX (chooser->language_listbox));
        gtk_list_box_set_placeholder (GTK_LIST_BOX (chooser->language_listbox), NULL);
        return;
    }
    chooser->filter_words = g_strsplit_set (g_strstrip (filter_contents), " ", 0);
    gtk_list_box_set_placeholder (GTK_LIST_BOX (chooser->language_listbox), chooser->no_results);
    gtk_list_box_invalidate_filter (GTK_LIST_BOX (chooser->language_listbox));
}
static void
show_more (LanguageChooser *chooser, gboolean visible)
{
    gint width, height;

    gtk_window_get_size (GTK_WINDOW (chooser), &width, &height);
    gtk_widget_set_size_request (GTK_WIDGET (chooser), width, height);

    gtk_search_bar_set_search_mode (GTK_SEARCH_BAR (chooser->search_bar), visible);
    gtk_widget_grab_focus (visible ? chooser->language_entry : chooser->language_listbox);

    chooser->showing_extra = visible;

    gtk_list_box_invalidate_filter (GTK_LIST_BOX (chooser->language_listbox));
}

static void set_locale_id (LanguageChooser *chooser,
                           const gchar     *locale_id)
{
    g_autoptr(GList) children = NULL;
    GList      *l;
    GtkWidget  *row;
    GtkWidget  *check;
    const char *language;
    gboolean    is_extra;
    gtk_widget_set_sensitive (chooser->done_button, FALSE);

    children = gtk_container_get_children (GTK_CONTAINER (chooser->language_listbox));
    for (l = children; l; l = l->next) 
    {
        row = l->data;
        check = g_object_get_data (G_OBJECT (row), "check");
        language = g_object_get_data (G_OBJECT (row), "locale-id");
        if (check == NULL || language == NULL)
                continue;

        if (g_strcmp0 (locale_id, language) == 0) 
        {
            gtk_widget_set_opacity (check, 1.0);
            gtk_widget_set_sensitive (chooser->done_button, TRUE);

            is_extra = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (row), "is-extra"));
            if (!chooser->showing_extra && is_extra) 
            {
                g_object_set_data (G_OBJECT (row), "is-extra", GINT_TO_POINTER (FALSE));
                gtk_list_box_invalidate_filter (GTK_LIST_BOX (chooser->language_listbox));
            }
        } 
        else 
        {
            gtk_widget_set_opacity (check, 0.0);
        }
    }

    g_free (chooser->language);
    chooser->language = g_strdup (locale_id);
}

static void
row_activated (GtkListBox        *box,
               GtkListBoxRow     *row,
               LanguageChooser *chooser)
{
    gchar *new_locale_id;

    if (row == NULL)
        return;

    if (row == chooser->more_item) 
    {
        show_more (chooser, TRUE);
        return;
    }
    new_locale_id = g_object_get_data (G_OBJECT (row), "locale-id");
    if (g_strcmp0 (new_locale_id, chooser->language) == 0) 
    {
        gtk_dialog_response (GTK_DIALOG (chooser),
                             gtk_dialog_get_response_for_widget (GTK_DIALOG (chooser),
                                                                 chooser->done_button));
    } 
    else 
    {
        set_locale_id (chooser, new_locale_id);
    }
}
static void activate_default (GtkWindow       *window,
                              LanguageChooser *chooser)
{
    GtkWidget *focus;
    gchar *locale_id;

    focus = gtk_window_get_focus (window);
    if (!focus)
        return;

    locale_id = g_object_get_data (G_OBJECT (focus), "locale-id");
    if (g_strcmp0 (locale_id, chooser->language) == 0)
        return;

    g_signal_stop_emission_by_name (window, "activate-default");
    gtk_widget_activate (focus);
}

void language_chooser_init (LanguageChooser *chooser)
{
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *Scrolled;

    gtk_window_set_default_size (GTK_WINDOW (chooser), 400, 350);
   
    chooser->header = GetHeaderbar();
    chooser->cancel_button = gtk_button_new_with_label(_("Cancel"));
    gtk_header_bar_pack_start (GTK_HEADER_BAR (chooser->header), 
                               chooser->cancel_button);
    chooser->done_button = gtk_button_new_with_label(_("done"));
    gtk_header_bar_pack_end (GTK_HEADER_BAR (chooser->header),
                             chooser->done_button);

    gtk_window_set_titlebar (GTK_WINDOW (chooser), chooser->header);
    
    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0); 
   
    chooser->language_entry = gtk_search_entry_new (); 
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign (hbox, GTK_ALIGN_CENTER);
    gtk_box_pack_start (GTK_BOX (hbox), chooser->language_entry, FALSE, FALSE, 0); 
    
    chooser->search_bar = gtk_search_bar_new (); 
    gtk_search_bar_connect_entry (GTK_SEARCH_BAR (chooser->search_bar), 
                                  GTK_ENTRY (chooser->language_entry));
    gtk_search_bar_set_show_close_button (GTK_SEARCH_BAR (chooser->search_bar), FALSE);
    gtk_container_add (GTK_CONTAINER (chooser->search_bar), hbox);
    gtk_box_pack_start (GTK_BOX (vbox), chooser->search_bar, FALSE, FALSE, 0);
    Scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (Scrolled),
                                    GTK_POLICY_NEVER,
                                    GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (Scrolled),
                                         GTK_SHADOW_IN);

    chooser->language_listbox = gtk_list_box_new();
    gtk_box_pack_start (GTK_BOX (vbox), chooser->language_listbox, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(Scrolled),vbox);
    chooser->more_item = more_widget_new ();
    chooser->no_results = g_object_ref_sink (no_results_widget_new ());
    gtk_widget_show_all (chooser->no_results);

    gtk_list_box_set_sort_func (GTK_LIST_BOX (chooser->language_listbox),
                                sort_languages, chooser, NULL);
    gtk_list_box_set_filter_func (GTK_LIST_BOX (chooser->language_listbox),
                                  language_visible, chooser, NULL);
    gtk_list_box_set_selection_mode (GTK_LIST_BOX (chooser->language_listbox),
                                     GTK_SELECTION_NONE);
    gtk_list_box_set_header_func (GTK_LIST_BOX (chooser->language_listbox),
                                  cc_list_box_update_header_func, NULL, NULL);
    add_all_languages (chooser);

    g_signal_connect_swapped (chooser->language_entry, "search-changed",
                              G_CALLBACK (filter_changed), chooser);

    g_signal_connect (chooser->language_listbox, "row-activated",
                      G_CALLBACK (row_activated), chooser);


    gtk_list_box_invalidate_filter (GTK_LIST_BOX (chooser->language_listbox));

    gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (chooser))),
                        Scrolled,
                        TRUE, TRUE, 8);

    g_signal_connect (chooser, "activate-default",
                          G_CALLBACK (activate_default), chooser);
}
LanguageChooser *language_chooser_new (const gchar *name)
{   
    LanguageChooser *chooser;
    g_autofree gchar *title = NULL;

    chooser = g_object_new (TYPE_LANGUAGE_CHOOSER,
                           "use-header-bar", 1,
                            NULL);

    title = g_strdup_printf (_("Current User - %s"),
                             name);
    gtk_header_bar_set_subtitle (GTK_HEADER_BAR (chooser->header),title);

    return chooser;
}
void language_chooser_clear_filter (LanguageChooser *chooser)
{
    gtk_entry_set_text (GTK_ENTRY (chooser->language_entry), "");
    show_more (chooser, FALSE);
}

const gchar *language_chooser_get_language (LanguageChooser *chooser)
{
    return chooser->language;
}

void language_chooser_set_language (LanguageChooser *chooser,
                                    const gchar     *language)
{
    set_locale_id (chooser, language);
}

