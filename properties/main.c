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

#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <dbus/dbus-glib.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#ifndef DBYS_TYPE_G_OBJECT_PATH_ARRAY
#define DBUS_TYPE_G_OBJECT_PATH_ARRAY \
	(dbus_g_type_get_collection("GPtrArray", DBUS_TYPE_G_OBJECT_PATH))
#endif

static GtkListStore *interface_store;

enum {
	COLUMN_PROXY,
	COLUMN_PATH,
	COLUMN_TYPE,
	COLUMN_STATE,
};

enum {
	TYPE_UNKNOWN,
	TYPE_80203,
	TYPE_80211,
};

static void state_to_icon(GtkTreeViewColumn *column, GtkCellRenderer *cell,
			GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	guint state;

	gtk_tree_model_get(model, iter, COLUMN_STATE, &state, -1);

	g_object_set(cell, "icon-name", GTK_STOCK_NO, NULL);
}

static void type_to_text(GtkTreeViewColumn *column, GtkCellRenderer *cell,
			GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	guint type;
	gchar *title, *text;

	gtk_tree_model_get(model, iter, COLUMN_TYPE, &type, -1);

	switch (type) {
	case TYPE_80203:
		title = "Ethernet";
		break;
	case TYPE_80211:
		title = "Wireless";
		break;
	default:
		title = "Unknown";
		break;
	}

	text = g_strdup_printf("<b>%s</b>\n<small>%s</small>",
						title, "Not Connected");

	g_object_set(cell, "markup", text, NULL);

	g_free(text);
}

static void type_to_icon(GtkTreeViewColumn *column, GtkCellRenderer *cell,
			GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	guint type;

	gtk_tree_model_get(model, iter, COLUMN_TYPE, &type, -1);

	switch (type) {
	case TYPE_80203:
		g_object_set(cell, "icon-name", "network-wired",
						"stock-size", 5, NULL);
		break;
	case TYPE_80211:
		g_object_set(cell, "icon-name", "network-wireless",
						"stock-size", 5, NULL);
		break;
	default:
		g_object_set(cell, "icon-name", NULL, NULL);
		break;
	}
}

static GtkWidget *create_interfaces(void)
{
	GtkWidget *mainbox;
	GtkWidget *hbox;
	GtkWidget *scrolled;
	GtkWidget *tree;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

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
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(tree), TRUE);
	gtk_widget_set_size_request(tree, 160, -1);
	gtk_container_add(GTK_CONTAINER(scrolled), tree);


	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, "Interface");
	gtk_tree_view_column_set_expand(column, TRUE);
	gtk_tree_view_column_set_spacing(column, 4);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func(column, renderer,
						state_to_icon, NULL, NULL);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer,
						type_to_text, NULL, NULL);

	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func(column, renderer,
						type_to_icon, NULL, NULL);


	gtk_tree_view_set_model(GTK_TREE_VIEW(tree),
					GTK_TREE_MODEL(interface_store));

	return mainbox;
}

static void delete_callback(GtkWidget *window, GdkEvent *event,
							gpointer user_data)
{
	gtk_widget_destroy(GTK_WIDGET(window));

	gtk_main_quit();
}

static void close_callback(GtkWidget *button, gpointer user_data)
{
	GtkWidget *window = user_data;

	gtk_widget_destroy(GTK_WIDGET(window));

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
	gtk_window_set_default_size(GTK_WINDOW(window), 480, 360);
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

	widget = create_interfaces();
	gtk_notebook_prepend_page(GTK_NOTEBOOK(notebook), widget, NULL);
	gtk_notebook_set_tab_label_text(GTK_NOTEBOOK(notebook),
						widget, _("Interfaces"));

	gtk_widget_show_all(window);

	return window;
}

static void add_interface(DBusGConnection *conn, const char *path)
{
	DBusGProxy *proxy;
	GHashTable *hash = NULL;
	GValue *value;
	const char *str;
	guint type;

	proxy = dbus_g_proxy_new_for_name(conn, "org.freedesktop.connman",
				path, "org.freedesktop.DBus.Properties");

	dbus_g_proxy_call(proxy, "GetAll", NULL,
			G_TYPE_STRING, "org.freedesktop.connman.Interface",
			G_TYPE_INVALID,
				dbus_g_type_get_map("GHashTable",
						G_TYPE_STRING, G_TYPE_VALUE),
				&hash, G_TYPE_INVALID);

	if (hash == NULL)
		goto done;

	value = g_hash_table_lookup(hash, "Type");
	str = g_value_get_string(value);

	if (g_ascii_strcasecmp(str, "80203") == 0)
		type = TYPE_80203;
	else if (g_ascii_strcasecmp(str, "80211") == 0)
		type = TYPE_80211;
	else
		type = TYPE_UNKNOWN;

	gtk_list_store_insert_with_values(interface_store, NULL, -1,
			COLUMN_PATH, path,
			COLUMN_TYPE, type, -1);

done:
	g_object_unref(proxy);
}

static DBusGProxy *create_manager(DBusGConnection *conn)
{
	DBusGProxy *manager;
	GError *error = NULL;
	GPtrArray *array = NULL;

	manager = dbus_g_proxy_new_for_name(conn, "org.freedesktop.connman",
					"/", "org.freedesktop.connman.Manager");

	dbus_g_proxy_call(manager, "ListInterfaces", &error, G_TYPE_INVALID,
			DBUS_TYPE_G_OBJECT_PATH_ARRAY, &array, G_TYPE_INVALID);

	if (error != NULL) {
		g_printerr("Getting interface list failed: %s\n",
							error->message);
		g_error_free(error);
	} else {
		int i;

		for (i = 0; i < array->len; i++) {
			gchar *path = g_ptr_array_index(array, i);
			add_interface(conn, path);
			g_free(path);
		}
	}

	return manager;
}

static void name_owner_changed(DBusGProxy *system, const char *name,
			const char *prev, const char *new, gpointer user_data)
{
	if (!strcmp(name, "org.freedesktop.connman") && *new == '\0') {
	}
}

static DBusGProxy *create_system(DBusGConnection *conn)
{
	DBusGProxy *system;

	system = dbus_g_proxy_new_for_name(conn, DBUS_SERVICE_DBUS,
					DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS);

	dbus_g_proxy_add_signal(system, "NameOwnerChanged",
		G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INVALID);

	dbus_g_proxy_connect_signal(system, "NameOwnerChanged",
				G_CALLBACK(name_owner_changed), NULL, NULL);

	return system;
}

static void sig_term(int sig)
{
	gtk_main_quit();
}

int main(int argc, char *argv[])
{
	DBusGConnection *conn;
	DBusGProxy *system, *manager;
	GError *error = NULL;
	struct sigaction sa;

	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);

	gtk_init(&argc, &argv);

	gtk_window_set_default_icon_name("stock_internet");

	conn = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);
	if (error != NULL) {
		g_printerr("Connecting to system bus failed: %s\n",
							error->message);
		g_error_free(error);
		exit(EXIT_FAILURE);
	}

	interface_store = gtk_list_store_new(4, DBUS_TYPE_G_PROXY,
				G_TYPE_STRING, G_TYPE_UINT, G_TYPE_UINT);

	create_window();

	system = create_system(conn);
	manager = create_manager(conn);

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = sig_term;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	gtk_main();

	g_object_unref(manager);
	g_object_unref(system);

	dbus_g_connection_unref(conn);

	return 0;
}
