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

static const gchar *type2str(guint type)
{
	switch (type) {
	case CONNMAN_TYPE_UNKNOWN:
		return "Unknown";
	case CONNMAN_TYPE_ETHERNET:
		return "Ethernet";
	case CONNMAN_TYPE_WIFI:
		return "WiFi";
	case CONNMAN_TYPE_WIMAX:
		return "WiMAX";
	case CONNMAN_TYPE_BLUETOOTH:
		return "Bluetooth";
	}

	return NULL;
}

static const gchar *state2str(guint state)
{
	switch (state) {
	case CONNMAN_STATE_UNKNOWN:
		return "Unknown";
	case CONNMAN_STATE_IDLE:
		return "Not connected";
	case CONNMAN_STATE_CARRIER:
		return "Carrier";
	case CONNMAN_STATE_ASSOCIATION:
		return "Connecting";
	case CONNMAN_STATE_CONFIGURATION:
		return "Configuration";
	case CONNMAN_STATE_READY:
		return "Connected";
	}

	return NULL;
}

static const gchar *security2str(guint security)
{
	switch (security) {
	case CONNMAN_SECURITY_UNKNOWN:
		return "Unknown";
	case CONNMAN_SECURITY_NONE:
		return "None";
	case CONNMAN_SECURITY_WEP:
		return "WEP";
	case CONNMAN_SECURITY_WPA:
		return "WPA";
	case CONNMAN_SECURITY_WPA2:
		return "WPA2";
	}

	return NULL;
}

static void service_to_text(GtkTreeViewColumn *column, GtkCellRenderer *cell,
			GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	gchar *name;
	guint type, state, security;
	gboolean favorite;
	gchar *markup;
	const gchar *format, *str, *val;

	gtk_tree_model_get(model, iter, CONNMAN_COLUMN_NAME, &name,
					CONNMAN_COLUMN_TYPE, &type,
					CONNMAN_COLUMN_STATE, &state,
					CONNMAN_COLUMN_FAVORITE, &favorite,
					CONNMAN_COLUMN_SECURITY, &security, -1);

	if (favorite == TRUE)
		format = "<b>%s</b>\n"
			"<span size=\"small\">%s service%s%s <i>(%s)</i></span>";
	else
		format = "%s\n<span size=\"small\">%s service%s%s <i>(%s)</i></span>";

	if (name == NULL) {
		if (type == CONNMAN_TYPE_WIFI)
			str = "<i>hidden</i>";
		else
			str = type2str(type);
	} else
		str = name;

	if (state == CONNMAN_STATE_UNKNOWN || state == CONNMAN_STATE_IDLE)
		val = NULL;
	else
		val = state2str(state);

	markup = g_strdup_printf(format, str, type2str(type),
					val ? " - " : "", val ? val : "",
						security2str(security));
	g_object_set(cell, "markup", markup, NULL);
	g_free(markup);

	g_free(name);
}

static void security_to_icon(GtkTreeViewColumn *column, GtkCellRenderer *cell,
			GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	guint security;

	gtk_tree_model_get(model, iter, CONNMAN_COLUMN_SECURITY, &security, -1);

	if (security == CONNMAN_SECURITY_UNKNOWN ||
					security == CONNMAN_SECURITY_NONE)
		g_object_set(cell, "icon-name", NULL, NULL);
	else
		g_object_set(cell, "icon-name",
					GTK_STOCK_DIALOG_AUTHENTICATION, NULL);
}

static void strength_to_value(GtkTreeViewColumn *column, GtkCellRenderer *cell,
			GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	guint type, strength;

	gtk_tree_model_get(model, iter, CONNMAN_COLUMN_TYPE, &type,
					CONNMAN_COLUMN_STRENGTH, &strength, -1);

	g_object_set(cell, "orientation", GTK_PROGRESS_RIGHT_TO_LEFT,
			"text-xalign", 1.0, "xpad", 4, "ypad", 6, NULL);

	if (type != CONNMAN_TYPE_ETHERNET && strength != 0) {
		g_object_set(cell, "value", strength, NULL);
		g_object_set(cell, "visible", TRUE, NULL);
	} else
		g_object_set(cell, "visible", FALSE, NULL);
}

static void drag_data_get(GtkWidget *widget, GdkDragContext *context,
				GtkSelectionData *data, guint info,
					guint time, gpointer user_data)
{
	g_print("drag-data-get\n");
}

static void drag_data_received(GtkWidget *widget, GdkDragContext *context,
				gint x, gint y, GtkSelectionData *data,
				guint info, guint time, gpointer user_data)
{
	GtkTreePath *path;

	g_print("drag-data-received %d,%d\n", x, y);

	if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget),
				x, y, &path, NULL, NULL, NULL) == FALSE)
		return;

	g_print("path %s\n", gtk_tree_path_to_string(path));

	gtk_tree_path_free(path);
}

static gboolean drag_drop(GtkWidget *widget, GdkDragContext *context,
				gint x, gint y, guint time, gpointer user_data)
{
	g_print("drag-drop %d,%d\n", x, y);

	return FALSE;
}

static void method_callback(DBusGProxy *proxy,
				DBusGProxyCall *call, void *user_data)
{
	dbus_g_proxy_end_call(proxy, call, NULL, G_TYPE_INVALID);

	g_print("finished\n");

	g_object_unref(proxy);
}

static void method_call(DBusGProxy *proxy, const char *method)
{
	if (proxy == NULL)
		return;

	g_print("%s <== %s\n", method, dbus_g_proxy_get_path(proxy));

	if (dbus_g_proxy_begin_call(proxy, method, method_callback,
					NULL, NULL, G_TYPE_INVALID) == FALSE) {
		g_print("Can't call method %s\n", method);
		g_object_unref(proxy);
		return;
	}
}

static DBusGProxy *get_proxy(GtkTreeSelection *selection)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	DBusGProxy *proxy = NULL;

	if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE)
		return NULL;

	gtk_tree_model_get(model, &iter, CONNMAN_COLUMN_PROXY, &proxy, -1);

	return proxy;
}

static void connect_callback(GtkWidget *widget, gpointer user_data)
{
	DBusGProxy *proxy = get_proxy(user_data);

	method_call(proxy, "Connect");
}

static void disconnect_callback(GtkWidget *widget, gpointer user_data)
{
	DBusGProxy *proxy = get_proxy(user_data);

	method_call(proxy, "Disconnect");
}

static void remove_callback(GtkWidget *widget, gpointer user_data)
{
	DBusGProxy *proxy = get_proxy(user_data);

	method_call(proxy, "Remove");
}

static void show_popup(GtkWidget *widget,
				GdkEventButton *event, gpointer user_data)
{
	GtkWidget *menu;
	GtkWidget *item;

	menu = gtk_menu_new();

	item = gtk_menu_item_new_with_label("Connect");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate",
				G_CALLBACK(connect_callback), user_data);

	item = gtk_menu_item_new_with_label("Disconnect");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate",
				G_CALLBACK(disconnect_callback), user_data);

	item = gtk_menu_item_new_with_label("Remove");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate",
				G_CALLBACK(remove_callback), user_data);

	gtk_widget_show_all(menu);

	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
					(event != NULL) ? event->button : 0,
					gdk_event_get_time((GdkEvent*) event));
}

static gboolean button_pressed(GtkWidget *widget,
				GdkEventButton *event, gpointer user_data)
{
	GtkTreeSelection *selection;
	GtkTreePath *path;

	if (event->type != GDK_BUTTON_PRESS)
		return FALSE;

	if (event->button != 3)
		return FALSE;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));

	if (gtk_tree_selection_count_selected_rows(selection) != 1)
		return FALSE;

	if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget),
					(gint) event->x, (gint) event->y,
					&path, NULL, NULL, NULL) == FALSE)
		return FALSE;

	gtk_tree_selection_unselect_all(selection);
	gtk_tree_selection_select_path(selection, path);

	gtk_tree_path_free(path);

	show_popup(widget, event, selection);

	return TRUE;
}

static gboolean popup_callback(GtkWidget *widget, gpointer user_data)
{
	GtkTreeSelection *selection;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));

	if (gtk_tree_selection_count_selected_rows(selection) != 1)
		return FALSE;

	show_popup(widget, NULL, selection);

	return TRUE;
}

static void select_callback(GtkTreeSelection *selection, gpointer user_data)
{
	GtkTreeView *tree = user_data;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean selected, favorite = FALSE;
	const GtkTargetEntry row_targets[] = {
		{ "resorting", GTK_TARGET_SAME_WIDGET, 0 }
	};

	selected = gtk_tree_selection_get_selected(selection, &model, &iter);
	if (selected == TRUE)
		gtk_tree_model_get(model, &iter,
				CONNMAN_COLUMN_FAVORITE, &favorite, -1);

	if (favorite == TRUE) {
		gtk_tree_view_enable_model_drag_source(tree, GDK_BUTTON1_MASK,
				row_targets, G_N_ELEMENTS(row_targets),
							GDK_ACTION_MOVE);
	gtk_tree_view_enable_model_drag_dest(tree,
				row_targets, G_N_ELEMENTS(row_targets),
							GDK_ACTION_MOVE);
	} else {
		gtk_tree_view_unset_rows_drag_source(tree);
		gtk_tree_view_unset_rows_drag_dest(tree);
	}
}

static GtkWidget *create_tree(void)
{
	GtkWidget *tree;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;

	tree = gtk_tree_view_new();
	gtk_tree_view_set_show_expanders(GTK_TREE_VIEW(tree), FALSE);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), FALSE);
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(tree), TRUE);
	gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(tree),
					GTK_TREE_VIEW_GRID_LINES_NONE);

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_expand(column, TRUE);
	gtk_tree_view_column_set_spacing(column, 6);
	gtk_tree_view_column_set_min_width(column, 250);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_add_attribute(column, renderer,
					"icon-name", CONNMAN_COLUMN_ICON);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer,
					service_to_text, NULL, NULL);

	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func(column, renderer,
					security_to_icon, NULL, NULL);

	renderer = gtk_cell_renderer_progress_new();
	gtk_cell_renderer_set_fixed_size(renderer, 100, -1);
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func(column, renderer,
					strength_to_value, NULL, NULL);

	model = connman_client_get_model(client);
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), model);
	g_object_unref(model);

	gtk_tree_view_set_search_column(GTK_TREE_VIEW(tree),
						CONNMAN_COLUMN_NAME);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);

	g_signal_connect(G_OBJECT(selection), "changed",
					G_CALLBACK(select_callback), tree);

	g_signal_connect(G_OBJECT(tree), "button-press-event",
					G_CALLBACK(button_pressed), NULL);
	g_signal_connect(G_OBJECT(tree), "popup-menu",
					G_CALLBACK(popup_callback), NULL);

	g_signal_connect(G_OBJECT(tree), "drag-drop",
					G_CALLBACK(drag_drop), NULL);
	g_signal_connect(G_OBJECT(tree), "drag-data-get",
					G_CALLBACK(drag_data_get), selection);
	g_signal_connect(G_OBJECT(tree), "drag-data-received",
					G_CALLBACK(drag_data_received), NULL);

	return tree;
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
	gtk_window_set_default_size(GTK_WINDOW(window), 480, 400);
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
