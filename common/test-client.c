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

static ConnmanClient *client;

static void proxy_to_text(GtkTreeViewColumn *column, GtkCellRenderer *cell,
			GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	DBusGProxy *proxy;
	gchar *markup;

	gtk_tree_model_get(model, iter, CONNMAN_COLUMN_PROXY, &proxy, -1);

	markup = g_strdup_printf("<b>%s</b>\n"
					"<span size=\"xx-small\">%s\n\n</span>",
					dbus_g_proxy_get_interface(proxy),
						dbus_g_proxy_get_path(proxy));
	g_object_set(cell, "markup", markup, NULL);
	g_free(markup);

	g_object_unref(proxy);
}

static void name_to_text(GtkTreeViewColumn *column, GtkCellRenderer *cell,
			GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	gchar *name, *icon;
	guint type;
	gchar *markup;

	gtk_tree_model_get(model, iter, CONNMAN_COLUMN_NAME, &name,
					CONNMAN_COLUMN_ICON, &icon,
					CONNMAN_COLUMN_TYPE, &type, -1);

	markup = g_strdup_printf("Name: %s\nIcon: %s\nType: %d",
							name, icon, type);
	g_object_set(cell, "markup", markup, NULL);
	g_free(markup);

	g_free(icon);
	g_free(name);
}

static void status_to_text(GtkTreeViewColumn *column, GtkCellRenderer *cell,
			GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	gboolean enabled, inrange, remember;
	gchar *markup;

	gtk_tree_model_get(model, iter, CONNMAN_COLUMN_ENABLED, &enabled,
					CONNMAN_COLUMN_INRANGE, &inrange,
					CONNMAN_COLUMN_REMEMBER, &remember, -1);

	markup = g_strdup_printf("Enabled: %d\n"
					"InRange: %d\nRemember: %d",
						enabled, inrange, remember);
	g_object_set(cell, "markup", markup, NULL);
	g_free(markup);
}

static void network_to_text(GtkTreeViewColumn *column, GtkCellRenderer *cell,
			GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	guint strength, security;
	gchar *secret;
	gchar *markup;

	gtk_tree_model_get(model, iter, CONNMAN_COLUMN_STRENGTH, &strength,
					CONNMAN_COLUMN_SECURITY, &security,
					CONNMAN_COLUMN_PASSPHRASE, &secret, -1);

	markup = g_strdup_printf("Strength: %d\nSecurity: %d\nSecret: %s",
						strength, security, secret);
	g_object_set(cell, "markup", markup, NULL);
	g_free(markup);

	g_free(secret);
}

static GtkWidget *create_tree(void)
{
	GtkWidget *tree;
	GtkTreeModel *model;
	GtkTreeSelection *selection;

	tree = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), TRUE);
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(tree), TRUE);

	gtk_tree_view_insert_column_with_data_func(GTK_TREE_VIEW(tree), -1,
				"Proxy", gtk_cell_renderer_text_new(),
					proxy_to_text, NULL, NULL);

	gtk_tree_view_insert_column_with_data_func(GTK_TREE_VIEW(tree), -1,
				"Name", gtk_cell_renderer_text_new(),
					name_to_text, NULL, NULL);

	gtk_tree_view_insert_column_with_data_func(GTK_TREE_VIEW(tree), -1,
				"Status", gtk_cell_renderer_text_new(),
					status_to_text, NULL, NULL);

	gtk_tree_view_insert_column_with_data_func(GTK_TREE_VIEW(tree), -1,
				"Network", gtk_cell_renderer_text_new(),
					network_to_text, NULL, NULL);

	model = connman_client_get_model(client);
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), model);
	g_object_unref(model);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);

	gtk_tree_view_expand_all(GTK_TREE_VIEW(tree));

	return tree;
}

static gboolean delete_callback(GtkWidget *window, GdkEvent *event,
							gpointer user_data)
{
	gtk_widget_destroy(GTK_WIDGET(window));

	gtk_main_quit();

	return FALSE;
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
	GtkWidget *mainbox;
	GtkWidget *tree;
	GtkWidget *scrolled;
	GtkWidget *buttonbox;
	GtkWidget *button;
	GtkTreeSelection *selection;

	tree = create_tree();
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "Client Test");
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
	g_signal_connect(G_OBJECT(window), "delete-event",
					G_CALLBACK(delete_callback), NULL);

	mainbox = gtk_vbox_new(FALSE, 12);
	gtk_container_set_border_width(GTK_CONTAINER(mainbox), 12);
	gtk_container_add(GTK_CONTAINER(window), mainbox);

	scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
				GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled),
							GTK_SHADOW_OUT);
	gtk_box_pack_start(GTK_BOX(mainbox), scrolled, TRUE, TRUE, 0);

	buttonbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(buttonbox),
						GTK_BUTTONBOX_END);
	gtk_box_pack_end(GTK_BOX(mainbox), buttonbox, FALSE, FALSE, 0);

	button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	gtk_container_add(GTK_CONTAINER(buttonbox), button);
	g_signal_connect(G_OBJECT(button), "clicked",
					G_CALLBACK(close_callback), window);

	gtk_container_add(GTK_CONTAINER(scrolled), tree);

	gtk_widget_show_all(window);

	return window;
}

int main(int argc, char *argv[])
{
	gtk_init(&argc, &argv);

	client = connman_client_new();

	gtk_window_set_default_icon_name("network-wireless");

	create_window();

	gtk_main();

	g_object_unref(client);

	return 0;
}
