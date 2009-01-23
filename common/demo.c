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

#include <dbus/dbus-glib.h>
#include <gtk/gtk.h>

#include "connman-client.h"

static gboolean option_fullscreen = FALSE;
static ConnmanClient *client;

static GtkWidget *tree_networks = NULL;
static GtkWidget *button_enabled = NULL;
static GtkWidget *button_refresh = NULL;
static GtkWidget *button_connect = NULL;
static GtkWidget *label_status = NULL;
static GtkTreeSelection *selection = NULL;

static void status_callback(const char *status, void *user_data)
{
	gchar *markup;

	if (label_status == NULL)
		return;

	markup = g_strdup_printf("System is %s", status);
	gtk_label_set_markup(GTK_LABEL(label_status), markup);
	g_free(markup);
}

static GtkWidget *create_label(const gchar *str)
{
	GtkWidget *label;
	gchar *tmp;

	label = gtk_label_new(NULL);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);

	tmp = g_strdup_printf("<b>%s</b>", str);
	gtk_label_set_markup(GTK_LABEL(label), tmp);
	g_free(tmp);

	return label;
}

static void changed_callback(GtkComboBox *combo, gpointer user_data)
{
	GtkTreeModel *model = gtk_combo_box_get_model(combo);
	GtkTreeIter iter;
	DBusGProxy *proxy;
	gchar *path;
	gboolean enabled;

	if (gtk_combo_box_get_active_iter(combo, &iter) == FALSE)
		return;

	path = g_object_get_data(G_OBJECT(button_enabled), "device");
	g_free(path);

	gtk_tree_model_get(model, &iter, CONNMAN_COLUMN_PROXY, &proxy,
					CONNMAN_COLUMN_ENABLED, &enabled, -1);

	path = g_strdup(dbus_g_proxy_get_path(proxy));
	g_object_set_data(G_OBJECT(button_enabled), "device", path);

	g_object_unref(proxy);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button_enabled),
								enabled);

	gtk_widget_set_sensitive(button_refresh, enabled);

	model = connman_client_get_network_model(client, path);
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree_networks), model);
	g_object_unref(model);
}

static void toggled_callback(GtkToggleButton *button, gpointer user_data)
{
	gchar *path;
	gboolean active;

	path = g_object_get_data(G_OBJECT(button), "device");
	if (path == NULL)
		return;

	active = gtk_toggle_button_get_active(button);

	connman_client_set_powered(client, path, active);

	gtk_widget_set_sensitive(button_refresh, active);
}

static void refresh_callback(GtkButton *button, gpointer user_data)
{
	gchar *path;

	path = g_object_get_data(G_OBJECT(button_enabled), "device");
	if (path == NULL)
		return;

	connman_client_propose_scan(client, path);
}

static void connect_callback(GtkButton *button, gpointer user_data)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	DBusGProxy *proxy;
	const gchar *path;
	gboolean enabled;

	if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE)
		return;

	gtk_tree_model_get(model, &iter, CONNMAN_COLUMN_PROXY, &proxy,
					CONNMAN_COLUMN_ENABLED, &enabled, -1);

	path = dbus_g_proxy_get_path(proxy);

	if (enabled == FALSE)
		connman_client_connect(client, path);
	else
		connman_client_disconnect(client, path);

	g_object_unref(proxy);

	if (enabled == FALSE)
		g_object_set(button_connect,
				"label", GTK_STOCK_DISCONNECT, NULL);
	else
		g_object_set(button_connect,
				"label", GTK_STOCK_CONNECT, NULL);
}

static GtkWidget *create_left(void)
{
	GtkWidget *mainbox;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *label;
	GtkWidget *combo;
	GtkWidget *button;
	GtkTreeModel *model;
	GtkCellRenderer *renderer;

	mainbox = gtk_vbox_new(FALSE, 24);
	gtk_container_set_border_width(GTK_CONTAINER(mainbox), 8);

	vbox = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(mainbox), vbox, FALSE, FALSE, 0);

	label = create_label("Device");
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	combo = gtk_combo_box_new();
	gtk_box_pack_start(GTK_BOX(vbox), combo, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(combo), "changed",
					G_CALLBACK(changed_callback), NULL);

	gtk_cell_layout_clear(GTK_CELL_LAYOUT(combo));

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), renderer,
					"text", CONNMAN_COLUMN_NAME, NULL);

	button = gtk_check_button_new_with_label("Enabled");
	gtk_box_pack_end(GTK_BOX(vbox), button, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(button), "toggled",
					G_CALLBACK(toggled_callback), NULL);

	button_enabled = button;

	vbox = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(mainbox), vbox, TRUE, TRUE, 0);

	label = create_label("Status");
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	label = gtk_label_new(NULL);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
	gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);

	label_status = label;

	vbox = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(mainbox), vbox, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 12);
	gtk_box_set_homogeneous(GTK_BOX(hbox), TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

	button = gtk_button_new_from_stock(GTK_STOCK_REFRESH);
	gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
	gtk_widget_set_sensitive(button, FALSE);
	g_signal_connect(G_OBJECT(button), "clicked",
					G_CALLBACK(refresh_callback), NULL);

	button_refresh = button;

	button = gtk_button_new_from_stock(GTK_STOCK_DISCONNECT);
	gtk_box_pack_end(GTK_BOX(hbox), button, TRUE, TRUE, 0);
	gtk_widget_set_sensitive(button, FALSE);
	g_signal_connect(G_OBJECT(button), "clicked",
					G_CALLBACK(connect_callback), NULL);

	button_connect = button;

	model = connman_client_get_device_model(client);
	gtk_combo_box_set_model(GTK_COMBO_BOX(combo), model);
	g_object_unref(model);

	gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);

	return mainbox;
}

static void select_callback(GtkTreeSelection *selection, gpointer user_data)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean selected, enabled;

	selected = gtk_tree_selection_get_selected(selection, &model, &iter);

	if (selected == TRUE) {
		gtk_tree_model_get(model, &iter,
					CONNMAN_COLUMN_ENABLED, &enabled, -1);

		if (enabled == TRUE)
			g_object_set(button_connect,
					"label", GTK_STOCK_DISCONNECT, NULL);
		else
			g_object_set(button_connect,
					"label", GTK_STOCK_CONNECT, NULL);
	}

	gtk_widget_set_sensitive(button_connect, selected);
}

static void status_to_icon(GtkTreeViewColumn *column, GtkCellRenderer *cell,
			GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	gboolean enabled;

	gtk_tree_model_get(model, iter, CONNMAN_COLUMN_ENABLED, &enabled, -1);

	if (enabled == TRUE)
		g_object_set(cell, "icon-name", GTK_STOCK_ABOUT, NULL);

	g_object_set(cell, "visible", enabled, NULL);
}

static void security_to_icon(GtkTreeViewColumn *column, GtkCellRenderer *cell,
			GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	guint security;

	gtk_tree_model_get(model, iter, CONNMAN_COLUMN_SECURITY, &security, -1);

	if (security == CONNMAN_SECURITY_NONE)
		g_object_set(cell, "icon-name", NULL, NULL);
	else
		g_object_set(cell, "icon-name",
					GTK_STOCK_DIALOG_AUTHENTICATION, NULL);
}

static GtkWidget *create_right(void)
{
	GtkWidget *mainbox;
	GtkWidget *vbox;
	GtkWidget *label;
	GtkWidget *scrolled;
	GtkWidget *tree;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;

	mainbox = gtk_vbox_new(FALSE, 24);
	gtk_container_set_border_width(GTK_CONTAINER(mainbox), 8);

	vbox = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(mainbox), vbox, TRUE, TRUE, 0);

	label = create_label("Networks");
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
				GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled),
							GTK_SHADOW_OUT);
	gtk_container_add(GTK_CONTAINER(vbox), scrolled);

	tree = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), FALSE);
	gtk_tree_view_set_show_expanders(GTK_TREE_VIEW(tree), FALSE);
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(tree), TRUE);
	gtk_container_add(GTK_CONTAINER(scrolled), tree);

	gtk_tree_view_insert_column_with_data_func(GTK_TREE_VIEW(tree), 0,
				NULL, gtk_cell_renderer_pixbuf_new(),
					status_to_icon, NULL, NULL);
	column = gtk_tree_view_get_column(GTK_TREE_VIEW(tree), 0);
	gtk_tree_view_column_set_min_width(column, 24);

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_expand(GTK_TREE_VIEW_COLUMN(column), TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_add_attribute(column, renderer,
					"text", CONNMAN_COLUMN_NAME);

	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_end(column, renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func(column, renderer,
					security_to_icon, NULL, NULL);

	tree_networks = tree;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
	g_signal_connect(G_OBJECT(selection), "changed",
				G_CALLBACK(select_callback), NULL);

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
	GtkWidget *hbox;
	GtkWidget *notebook;
	GtkWidget *buttonbox;
	GtkWidget *button;
	GtkWidget *widget;

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window), 580, 360);
	g_signal_connect(G_OBJECT(window), "delete-event",
					G_CALLBACK(delete_callback), NULL);

	vbox = gtk_vbox_new(FALSE, 12);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	notebook = gtk_notebook_new();
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), FALSE);
	gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

	buttonbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(buttonbox), GTK_BUTTONBOX_END);
	gtk_box_pack_start(GTK_BOX(vbox), buttonbox, FALSE, FALSE, 0);

	button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	gtk_container_add(GTK_CONTAINER(buttonbox), button);
	g_signal_connect(G_OBJECT(button), "clicked",
					G_CALLBACK(close_callback), window);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 8);

	widget = create_right();
	gtk_widget_set_size_request(widget, 280, -1);
	gtk_box_pack_end(GTK_BOX(hbox), widget, TRUE, TRUE, 0);

	widget = create_left();
	gtk_widget_set_size_request(widget, 260, -1);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), hbox, NULL);

	return window;
}

static GOptionEntry options[] = {
	{ "fullscreen", 'f', 0, G_OPTION_ARG_NONE, &option_fullscreen,
					"Start up in fullscreen mode" },
	{ NULL },
};

int main(int argc, char *argv[])
{
	GError *error = NULL;
	GtkWidget *window;

	if (gtk_init_with_args(&argc, &argv, NULL,
				options, NULL, &error) == FALSE) {
		if (error != NULL) {
			g_printerr("%s\n", error->message);
			g_error_free(error);
		} else
			g_printerr("An unknown error occurred\n");

		gtk_exit(1);
	}

	g_set_application_name("Connection Manager Demo");

	gtk_window_set_default_icon_name("network-wireless");

	client = connman_client_new();

	window = create_window();

	connman_client_set_callback(client, status_callback, NULL);

	if (option_fullscreen == TRUE)
		gtk_window_fullscreen(GTK_WINDOW(window));

	gtk_widget_show_all(window);

	gtk_main();

	g_object_unref(client);

	return 0;
}
