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
#include <dbus/dbus-glib.h>

#include "connman-client.h"

#include "advanced.h"

static ConnmanClient *client;
static GtkWidget *interface_notebook;
static struct config_data *current_data;

static void status_update(GtkTreeModel *model, GtkTreePath  *path,
		GtkTreeIter  *iter, gpointer user_data)
{
	struct config_data *data = user_data;
	guint type;
	const char *name = NULL, *_name = NULL, *state = NULL;
	gboolean ethernet_enabled;
	gboolean wifi_enabled;
	gboolean cellular_enabled;
	gboolean offline_mode;

	gtk_tree_model_get(model, iter,
			CONNMAN_COLUMN_STATE, &state,
			CONNMAN_COLUMN_NAME, &name,
			CONNMAN_COLUMN_TYPE, &type,
			CONNMAN_COLUMN_ETHERNET_ENABLED, &ethernet_enabled,
			CONNMAN_COLUMN_WIFI_ENABLED, &wifi_enabled,
			CONNMAN_COLUMN_CELLULAR_ENABLED, &cellular_enabled,
			CONNMAN_COLUMN_OFFLINEMODE, &offline_mode,
			-1);

	if (type == CONNMAN_TYPE_WIFI) {
		if (data->wifi.name)
			_name = gtk_label_get_text(GTK_LABEL(data->wifi.name));

		if (!(name && _name && g_str_equal(name, _name)))
			return;

		if (g_str_equal(state, "failure") == TRUE) {
			gtk_label_set_text(GTK_LABEL(data->wifi.connect_info),
					_("connection failed"));
			gtk_widget_show(data->wifi.connect_info);
			gtk_widget_show(data->wifi.connect);
			gtk_widget_hide(data->wifi.disconnect);
		} else if (g_str_equal(state, "idle") == TRUE) {
			gtk_widget_hide(data->wifi.connect_info);
			gtk_widget_show(data->wifi.connect);
			gtk_widget_hide(data->wifi.disconnect);
		} else {
			gtk_widget_hide(data->wifi.connect_info);
			gtk_widget_hide(data->wifi.connect);
			gtk_widget_show(data->wifi.disconnect);
		}
	} else if (type == CONNMAN_TYPE_CELLULAR) {
		if (data->cellular.name)
			_name = gtk_label_get_text(GTK_LABEL(data->cellular.name));

		if (!(name && _name && g_str_equal(name, _name)))
			return;

		if (g_str_equal(state, "failure") == TRUE) {
			gtk_label_set_text(GTK_LABEL(data->cellular.connect_info),
					_("connection failed"));
			gtk_widget_show(data->cellular.connect_info);
			gtk_widget_show(data->cellular.connect);
			gtk_widget_hide(data->cellular.disconnect);
		} else if (g_str_equal(state, "idle") == TRUE) {
			gtk_widget_hide(data->cellular.connect_info);
			gtk_widget_show(data->cellular.connect);
			gtk_widget_hide(data->cellular.disconnect);
		} else {
			gtk_widget_hide(data->cellular.connect_info);
			gtk_widget_hide(data->cellular.connect);
			gtk_widget_show(data->cellular.disconnect);
		}

	} else if (type == CONNMAN_TYPE_LABEL_ETHERNET) {
		if (!data->ethernet_button)
			return;
		if (ethernet_enabled)
			gtk_button_set_label(GTK_BUTTON(data->ethernet_button), _("Disable"));
		else
			gtk_button_set_label(GTK_BUTTON(data->ethernet_button), _("Enable"));
	} else if (type == CONNMAN_TYPE_LABEL_WIFI) {
		if (!data->wifi_button)
			return;
		if (wifi_enabled) {
			gtk_button_set_label(GTK_BUTTON(data->wifi_button), _("Disable"));
			gtk_widget_set_sensitive(data->scan_button, 1);
		} else {
			gtk_button_set_label(GTK_BUTTON(data->wifi_button), _("Enable"));
			gtk_widget_set_sensitive(data->scan_button, 0);
		}
	} else if (type == CONNMAN_TYPE_LABEL_CELLULAR) {
		if (!data->cellular_button)
			return;
		if (cellular_enabled)
			gtk_button_set_label(GTK_BUTTON(data->cellular_button), _("Disable"));
		else
			gtk_button_set_label(GTK_BUTTON(data->cellular_button), _("Enable"));
	} else if (type == CONNMAN_TYPE_SYSCONFIG) {
		if (!data->offline_button)
			return;
		if (offline_mode)
			gtk_button_set_label(GTK_BUTTON(data->offline_button), _("OnlineMode"));
		else
			gtk_button_set_label(GTK_BUTTON(data->offline_button), _("OfflineMode"));
	}
}

static void set_offline_callback(GtkWidget *button, gpointer user_data)
{
	struct config_data *data = user_data;
	const gchar *label = gtk_button_get_label(GTK_BUTTON(data->offline_button));
	if (g_str_equal(label, "OnlineMode"))
		connman_client_set_offlinemode(client, 0);
	else if (g_str_equal(label, "OfflineMode"))
		connman_client_set_offlinemode(client, 1);
}

static void add_system_config(GtkWidget *mainbox, GtkTreeIter *iter,
		struct config_data *data)
{
	GtkWidget *vbox;
	GtkWidget *table;
	GtkWidget *label;
	GtkWidget *buttonbox;
	GtkWidget *button;
	gboolean offline_mode;

	vbox = gtk_vbox_new(TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 24);
	gtk_box_pack_start(GTK_BOX(mainbox), vbox, FALSE, FALSE, 0);

	table = gtk_table_new(1, 1, TRUE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 10);
	gtk_table_set_col_spacings(GTK_TABLE(table), 10);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);

	label = gtk_label_new(_("System Configuration"));
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);

	buttonbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(buttonbox), GTK_BUTTONBOX_CENTER);
	gtk_box_pack_start(GTK_BOX(mainbox), buttonbox, FALSE, FALSE, 0);

	gtk_tree_model_get(data->model, iter,
			CONNMAN_COLUMN_OFFLINEMODE, &offline_mode,
			-1);

	button = gtk_button_new();
	data->offline_button = button;
	if (offline_mode)
		gtk_button_set_label(GTK_BUTTON(button), _("OnlineMode"));
	else
		gtk_button_set_label(GTK_BUTTON(button), _("OfflineMode"));

	gtk_container_add(GTK_CONTAINER(buttonbox), button);
	g_signal_connect(G_OBJECT(button), "clicked",
			G_CALLBACK(set_offline_callback), data);
}

static struct config_data *create_config(GtkTreeModel *model,
					GtkTreeIter *iter, gpointer user_data)
{
	GtkWidget *mainbox;
	GtkWidget *label;
	GtkWidget *hbox;
	struct config_data *data;
	DBusGProxy *proxy;
	guint type;
	char *state;

	data = g_try_new0(struct config_data, 1);
	if (data == NULL)
		return NULL;

	data->client = client;

	gtk_tree_model_get(model, iter,
				CONNMAN_COLUMN_PROXY, &proxy,
				CONNMAN_COLUMN_TYPE, &type,
				CONNMAN_COLUMN_STATE, &state,
				-1);

	mainbox = gtk_vbox_new(FALSE, 6);
	data->widget = mainbox;

	label = gtk_label_new(NULL);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
	gtk_box_pack_start(GTK_BOX(mainbox), label, FALSE, FALSE, 0);
	data->title = label;

	label = gtk_label_new(NULL);
	gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
	gtk_box_pack_start(GTK_BOX(mainbox), label, FALSE, FALSE, 0);
	data->label = label;

	data->window = user_data;
	data->model = model;
	data->index = gtk_tree_model_get_string_from_iter(model, iter);
	data->device = g_strdup(dbus_g_proxy_get_path(proxy));
	g_object_unref(proxy);

	switch (type) {
	case CONNMAN_TYPE_ETHERNET:
		add_ethernet_service(mainbox, iter, data);
		break;
	case CONNMAN_TYPE_WIFI:
		add_wifi_service(mainbox, iter, data);
		break;
	case CONNMAN_TYPE_CELLULAR:
		add_cellular_service(mainbox, iter, data);
		break;
	case CONNMAN_TYPE_LABEL_ETHERNET:
		add_ethernet_switch_button(mainbox, iter, data);
		break;
	case CONNMAN_TYPE_LABEL_WIFI:
		add_wifi_switch_button(mainbox, iter, data);
		break;
	case CONNMAN_TYPE_LABEL_CELLULAR:
		add_cellular_switch_button(mainbox, iter, data);
		break;
	case CONNMAN_TYPE_SYSCONFIG:
		add_system_config(mainbox, iter, data);
		break;
	default:
		break;
	}

	hbox = gtk_hbox_new(FALSE, 12);
	gtk_box_pack_end(GTK_BOX(mainbox), hbox, FALSE, FALSE, 0);

	label = gtk_label_new(NULL);
	gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 1.0);
	gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);

	gtk_widget_show_all(mainbox);

	g_signal_connect(G_OBJECT(model), "row-changed",
			G_CALLBACK(status_update), data);

	return data;
}

static void select_callback(GtkTreeSelection *selection, gpointer user_data)
{
	GtkWidget *notebook = interface_notebook;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean selected;
	struct config_data *data = NULL;
	gint page;
	gint last_page;

	selected = gtk_tree_selection_get_selected(selection, &model, &iter);
	if (selected == FALSE) {
		gtk_widget_hide(interface_notebook);
		return;
	}

	if (current_data) {
		g_signal_handlers_disconnect_by_func(G_OBJECT(model),
				G_CALLBACK(status_update), current_data);
		g_free(current_data);
	}

	data = create_config(model, &iter, user_data);
	if (data == NULL)
		return;


	current_data = data;

	last_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
	page = gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
				data->widget, NULL);

	gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), page);

	gtk_widget_show(notebook);

	if (last_page != -1)
		gtk_notebook_remove_page(GTK_NOTEBOOK(notebook), last_page);
}

static void device_to_text(GtkTreeViewColumn *column, GtkCellRenderer *cell,
		GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	guint type;
	char *markup, *name, *state;
	const char *title;

	gtk_tree_model_get(model, iter, CONNMAN_COLUMN_TYPE, &type,
				CONNMAN_COLUMN_NAME, &name,
				CONNMAN_COLUMN_STATE, &state,
				-1);
	switch (type) {
	case CONNMAN_TYPE_ETHERNET:
		title = N_("Ethernet");
		markup = g_strdup_printf("  %s\n", title);
		break;
	case CONNMAN_TYPE_WIFI:
	case CONNMAN_TYPE_CELLULAR:
		/* Show the AP name */
		title = N_(name);
		if (g_str_equal(state, "association") == TRUE)
			state = "associating...";
		else if (g_str_equal(state, "configuration") == TRUE)
			state = "configurating...";
		else if (g_str_equal(state, "ready") == TRUE ||
				g_str_equal(state, "online") == TRUE)
			state = "connnected";
		else
			state = "";
		markup = g_strdup_printf("  %s\n  %s", title, state);
		break;
	case CONNMAN_TYPE_WIMAX:
		title = N_("WiMAX");
		markup = g_strdup_printf("  %s\n", title);
		break;
	case CONNMAN_TYPE_BLUETOOTH:
		title = N_("Bluetooth");
		markup = g_strdup_printf("  %s\n", title);
		break;
	case CONNMAN_TYPE_LABEL_ETHERNET:
		title = N_("Wired Networks");
		markup = g_strdup_printf("<b>\n%s\n</b>", title);
		break;
	case CONNMAN_TYPE_LABEL_WIFI:
		title = N_("Wireless Networks");
		markup = g_strdup_printf("<b>\n%s\n</b>", title);
		break;
	case CONNMAN_TYPE_LABEL_CELLULAR:
		title = N_("Cellular Networks");
		markup = g_strdup_printf("<b>\n%s\n</b>", title);
		break;
	case CONNMAN_TYPE_SYSCONFIG:
		title = N_("System Configuration");
		markup = g_strdup_printf("<b>\n%s\n</b>", title);
		break;
	default:
		title = N_("Unknown");
		markup = g_strdup_printf("  %s\n", title);
		break;
	}

	g_object_set(cell, "markup", markup, NULL);
	g_free(markup);
}

static void type_to_icon(GtkTreeViewColumn *column, GtkCellRenderer *cell,
			GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	guint type, strength;
	char *name;

	gtk_tree_model_get(model, iter, CONNMAN_COLUMN_TYPE, &type,
					CONNMAN_COLUMN_STRENGTH, &strength,
					-1);

	switch (type) {
	case CONNMAN_TYPE_WIFI:
		name = g_strdup_printf("connman-signal-0%d", (strength-1)/20+1);
		g_object_set(cell, "icon-name", name,
						"stock-size", 4, NULL);
		break;
	case CONNMAN_TYPE_LABEL_ETHERNET:
		g_object_set(cell, "icon-name", "network-wired",
						"stock-size", 4, NULL);
		break;
	case CONNMAN_TYPE_LABEL_WIFI:
		g_object_set(cell, "icon-name", "network-wireless",
						"stock-size", 4, NULL);
		break;
	case CONNMAN_TYPE_LABEL_CELLULAR:
		g_object_set(cell, "icon-name", "network-cellular",
						"stock-size", 4, NULL);
		break;
	default:
		g_object_set(cell, "icon-name", NULL, NULL);
		break;
	}
}

static GtkWidget *create_interfaces(GtkWidget *window)
{
	GtkWidget *mainbox;
	GtkWidget *hbox;
	GtkWidget *scrolled;
	GtkWidget *tree;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *selection;
	GtkTreeModel *model;

	mainbox = gtk_vbox_new(FALSE, 18);
	gtk_container_set_border_width(GTK_CONTAINER(mainbox), 12);

	hbox = gtk_hbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX(mainbox), hbox, TRUE, TRUE, 0);

	scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
				GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled),
							GTK_SHADOW_OUT);
	gtk_box_pack_start(GTK_BOX(hbox), scrolled, FALSE, TRUE, 0);

	tree = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), FALSE);
	gtk_tree_view_set_show_expanders(GTK_TREE_VIEW(tree), FALSE);
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(tree), TRUE);
	gtk_widget_set_size_request(tree, 220, -1);
	gtk_container_add(GTK_CONTAINER(scrolled), tree);


	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_expand(column, TRUE);
	gtk_tree_view_column_set_spacing(column, 4);
	gtk_tree_view_column_set_alignment(column, 0);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func(column, renderer,
						type_to_icon, NULL, NULL);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func(column, renderer,
						device_to_text, NULL, NULL);

	interface_notebook = gtk_notebook_new();
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(interface_notebook), FALSE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(interface_notebook), FALSE);
	gtk_widget_set_no_show_all(interface_notebook, TRUE);
	gtk_box_pack_start(GTK_BOX(hbox), interface_notebook, TRUE, TRUE, 0);

	model = connman_client_get_device_model(client);
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), model);
	g_object_unref(model);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
	g_signal_connect(G_OBJECT(selection), "changed",
					G_CALLBACK(select_callback), window);

	return mainbox;
}

static gboolean delete_callback(GtkWidget *window, GdkEvent *event,
							gpointer user_data)
{
	gtk_widget_destroy(window);

	gtk_main_quit();

	return FALSE;
}

static void close_callback(GtkWidget *button, gpointer user_data)
{
	GtkWidget *window = user_data;

	gtk_widget_destroy(window);

	gtk_main_quit();
}

static GtkWidget *create_window(void)
{
	GtkWidget *window;
	GtkWidget *vbox;
	GtkWidget *notebook;
	GtkWidget *buttonbox;
	GtkWidget *button;
	GtkWidget *widget;

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), _("Connection Preferences"));
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window), 580, 380);
	g_signal_connect(G_OBJECT(window), "delete-event",
					G_CALLBACK(delete_callback), NULL);

	vbox = gtk_vbox_new(FALSE, 12);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	notebook = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

	buttonbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(buttonbox), GTK_BUTTONBOX_END);
	gtk_box_pack_start(GTK_BOX(vbox), buttonbox, FALSE, FALSE, 0);

	button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	gtk_container_add(GTK_CONTAINER(buttonbox), button);
	g_signal_connect(G_OBJECT(button), "clicked",
					G_CALLBACK(close_callback), window);

	widget = create_interfaces(window);
	gtk_notebook_prepend_page(GTK_NOTEBOOK(notebook), widget, NULL);
	gtk_notebook_set_tab_label_text(GTK_NOTEBOOK(notebook),
						widget, _("Services"));

	gtk_widget_show_all(window);

	return window;
}

int main(int argc, char *argv[])
{
	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);

	current_data = NULL;

	gtk_init(&argc, &argv);

	gtk_window_set_default_icon_name("network-wireless");

	client = connman_client_new();

	create_window();

	gtk_main();

	g_object_unref(client);

	return 0;
}
