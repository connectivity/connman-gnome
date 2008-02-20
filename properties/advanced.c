/*
 *
 *  Connection Manager
 *
 *  Copyright (C) 2008  Intel Corporation. All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "client.h"
#include "advanced.h"

gboolean separator_function(GtkTreeModel *model,
					GtkTreeIter *iter, gpointer user_data)
{
	gchar *text;
	gboolean result = FALSE;

	gtk_tree_model_get(model, iter, 0, &text, -1);

	if (text && *text == '\0')
		result = TRUE;

	g_free(text);

	return result;
}

#if 0
static void activate_callback(GtkWidget *button, gpointer user_data)
{
	struct config_data *data = user_data;
	GtkTreeModel *model;
	GtkTreeIter iter;
	DBusGProxy *proxy;
	GError *error = NULL;
	const gchar *network, *passphrase;

	gtk_tree_selection_get_selected(data->selection, &model, &iter);

	gtk_tree_model_get(model, &iter, CLIENT_COLUMN_PROXY, &proxy, -1);

	network = gtk_combo_box_get_active_text(GTK_COMBO_BOX(data->network.name));

	passphrase = gtk_entry_get_text(GTK_ENTRY(data->network.passphrase));

	dbus_g_proxy_call(proxy, "SetNetwork", &error,
					G_TYPE_STRING, network,
					G_TYPE_INVALID, G_TYPE_INVALID);

	dbus_g_proxy_call(proxy, "SetPassphrase", &error,
					G_TYPE_STRING, passphrase,
					G_TYPE_INVALID, G_TYPE_INVALID);

	dbus_g_proxy_call(proxy, "Enable", &error,
					G_TYPE_INVALID, G_TYPE_INVALID);

	if (error != NULL) {
		g_printerr("Activate failed: %s\n", error->message);
		g_error_free(error);
	}
}

static void changed_callback(GtkWidget *editable, gpointer user_data)
{
	struct config_data *data = user_data;
	const gchar *text;

	text = gtk_combo_box_get_active_text(GTK_COMBO_BOX(data->network.name));

	gtk_widget_set_sensitive(data->network.activate, *text != '\0');
}
#endif

static void add_network(GtkWidget *mainbox, struct config_data *data)
{
	GtkWidget *table;
	GtkWidget *label;
	GtkWidget *entry;
	GtkWidget *combo;
	GtkWidget *button;

	table = gtk_table_new(3, 5, TRUE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 2);
	gtk_table_set_col_spacings(GTK_TABLE(table), 8);
	gtk_box_pack_start(GTK_BOX(mainbox), table, FALSE, FALSE, 0);

	label = gtk_label_new(_("Network Name:"));
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
	gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);

	combo = gtk_combo_box_entry_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "Guest");
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "");
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "Off");
	gtk_combo_box_set_row_separator_func(GTK_COMBO_BOX(combo),
					separator_function, NULL, NULL);
	gtk_table_attach_defaults(GTK_TABLE(table), combo, 1, 3, 0, 1);
	//data->network.name = combo;

	label = gtk_label_new(_("Passphrase:"));
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
	gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 1, 2);

	entry = gtk_entry_new();
	gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);
	gtk_table_attach_defaults(GTK_TABLE(table), entry, 1, 3, 1, 2);
	//data->network.passphrase = entry;

	button = gtk_button_new_with_label(_("Activate"));
	gtk_widget_set_sensitive(button, FALSE);
	gtk_table_attach_defaults(GTK_TABLE(table), button, 1, 2, 2, 3);
	//data->network.activate = button;

#if 0
	g_signal_connect(G_OBJECT(combo), "changed",
				G_CALLBACK(changed_callback), data);

	g_signal_connect(G_OBJECT(button), "clicked",
				G_CALLBACK(activate_callback), data);
#endif
}

static void set_widgets(struct config_data *data, gboolean label,
					gboolean value, gboolean entry)
{
	int i;

	for (i = 0; i < 3; i++) {
		if (label == TRUE)
			gtk_widget_show(data->ipv4.label[i]);
		else
			gtk_widget_hide(data->ipv4.label[i]);

		if (value == TRUE)
			gtk_widget_show(data->ipv4.value[i]);
		else
			gtk_widget_hide(data->ipv4.value[i]);

		if (entry == TRUE)
			gtk_widget_show(data->ipv4.entry[i]);
		else
			gtk_widget_hide(data->ipv4.entry[i]);
	}
}

static void config_callback(GtkWidget *widget, gpointer user_data)
{
	struct config_data *data = user_data;
	gint active;

	active = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));

	switch (active) {
	case 0:
		set_widgets(data, TRUE, TRUE, FALSE);
		break;
	case 1:
		set_widgets(data, TRUE, FALSE, TRUE);
		break;
	case 3:
		set_widgets(data, FALSE, FALSE, FALSE);
		break;
	}
}

static void add_config(GtkWidget *mainbox, struct config_data *data)
{
	GtkWidget *table;
	GtkWidget *label;
	GtkWidget *entry;
	GtkWidget *combo;

	table = gtk_table_new(5, 5, TRUE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 2);
	gtk_table_set_col_spacings(GTK_TABLE(table), 8);
	gtk_box_pack_start(GTK_BOX(mainbox), table, FALSE, FALSE, 0);

	label = gtk_label_new(_("Configure IPv4:"));
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
	gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);

	combo = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "Using DHCP");
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "Manually");
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "");
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "Off");
	gtk_combo_box_set_row_separator_func(GTK_COMBO_BOX(combo),
					separator_function, NULL, NULL);
	gtk_table_attach_defaults(GTK_TABLE(table), combo, 1, 3, 0, 1);
	data->ipv4.config = combo;

	label = gtk_label_new(_("IP Address:"));
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
	gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
	gtk_widget_set_no_show_all(label, TRUE);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 1, 2);
	data->ipv4.label[0] = label;

	label = gtk_label_new(NULL);
	gtk_label_set_selectable(GTK_LABEL(label), TRUE);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_set_no_show_all(label, TRUE);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 1, 3, 1, 2);
	data->ipv4.value[0] = label;

	entry = gtk_entry_new();
	gtk_widget_set_no_show_all(entry, TRUE);
	gtk_table_attach_defaults(GTK_TABLE(table), entry, 1, 3, 1, 2);
	data->ipv4.entry[0] = entry;

	label = gtk_label_new(_("Subnet Mask:"));
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
	gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
	gtk_widget_set_no_show_all(label, TRUE);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 2, 3);
	data->ipv4.label[1] = label;

	label = gtk_label_new(NULL);
	gtk_label_set_selectable(GTK_LABEL(label), TRUE);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_set_no_show_all(label, TRUE);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 1, 3, 2, 3);
	data->ipv4.value[1] = label;

	entry = gtk_entry_new();
	gtk_widget_set_no_show_all(entry, TRUE);
	gtk_table_attach_defaults(GTK_TABLE(table), entry, 1, 3, 2, 3);
	data->ipv4.entry[1] = entry;

	label = gtk_label_new(_("Router:"));
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
	gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
	gtk_widget_set_no_show_all(label, TRUE);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 3, 4);
	data->ipv4.label[2] = label;

	label = gtk_label_new(NULL);
	gtk_label_set_selectable(GTK_LABEL(label), TRUE);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_set_no_show_all(label, TRUE);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 1, 3, 3, 4);
	data->ipv4.value[2] = label;

	entry = gtk_entry_new();
	gtk_widget_set_no_show_all(entry, TRUE);
	gtk_table_attach_defaults(GTK_TABLE(table), entry, 1, 3, 3, 4);
	data->ipv4.entry[2] = entry;

	gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
	set_widgets(data, TRUE, TRUE, FALSE);

	g_signal_connect(G_OBJECT(combo), "changed",
					G_CALLBACK(config_callback), data);
}

static void delete_callback(GtkWidget *window, GdkEvent *event,
							gpointer user_data)
{
	gtk_widget_hide(window);
}

static void close_callback(GtkWidget *button, gpointer user_data)
{
	GtkWidget *window = user_data;

	gtk_widget_hide(window);
}

void create_advanced_dialog(struct config_data *data, guint type)
{
	GtkWidget *dialog;
	GtkWidget *vbox;
	GtkWidget *notebook;
	GtkWidget *buttonbox;
	GtkWidget *button;
	GtkWidget *widget;

	dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
	gtk_window_set_transient_for(GTK_WINDOW(dialog),
						GTK_WINDOW(data->window));
	gtk_window_set_title(GTK_WINDOW(dialog), _("Advanced Settings"));
	gtk_window_set_position(GTK_WINDOW(dialog),
					GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_default_size(GTK_WINDOW(dialog), 460, 320);
	g_signal_connect(G_OBJECT(dialog), "delete-event",
					G_CALLBACK(delete_callback), NULL);

	vbox = gtk_vbox_new(FALSE, 12);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);
	gtk_container_add(GTK_CONTAINER(dialog), vbox);

	notebook = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

	buttonbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(buttonbox), GTK_BUTTONBOX_END);
	gtk_box_set_spacing(GTK_BOX(buttonbox), 6);
	gtk_box_pack_start(GTK_BOX(vbox), buttonbox, FALSE, FALSE, 0);

	button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	gtk_container_add(GTK_CONTAINER(buttonbox), button);
	g_signal_connect(G_OBJECT(button), "clicked",
					G_CALLBACK(close_callback), dialog);

	button = gtk_button_new_from_stock(GTK_STOCK_OK);
	gtk_container_add(GTK_CONTAINER(buttonbox), button);
	g_signal_connect(G_OBJECT(button), "clicked",
					G_CALLBACK(close_callback), dialog);

	if (type == CLIENT_TYPE_80211) {
		widget = gtk_vbox_new(FALSE, 24);
		gtk_container_set_border_width(GTK_CONTAINER(widget), 24);
		gtk_notebook_append_page(GTK_NOTEBOOK(notebook), widget, NULL);
		gtk_notebook_set_tab_label_text(GTK_NOTEBOOK(notebook),
							widget, _("Wireless"));
		add_network(widget, data);
	}

	widget = gtk_vbox_new(FALSE, 24);
	gtk_container_set_border_width(GTK_CONTAINER(widget), 24);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), widget, NULL);
	gtk_notebook_set_tab_label_text(GTK_NOTEBOOK(notebook),
						widget, _("TCP/IP"));
	add_config(widget, data);

	widget = gtk_label_new(NULL);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), widget, NULL);
	gtk_notebook_set_tab_label_text(GTK_NOTEBOOK(notebook),
						widget, _("DNS"));

	if (type == CLIENT_TYPE_80203) {
		widget = gtk_label_new(NULL);
		gtk_notebook_append_page(GTK_NOTEBOOK(notebook), widget, NULL);
		gtk_notebook_set_tab_label_text(GTK_NOTEBOOK(notebook),
							widget, _("Ethernet"));
	}

	data->dialog = dialog;
}
