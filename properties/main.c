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

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "client.h"
#include "helper.h"

static GtkWidget *interface_notebook;

static void state_to_icon(GtkTreeViewColumn *column, GtkCellRenderer *cell,
			GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	guint state;

	gtk_tree_model_get(model, iter, CLIENT_COLUMN_STATE, &state, -1);

	switch (state) {
	case CLIENT_STATE_OFF:
	case CLIENT_STATE_CARRIER:
		g_object_set(cell, "icon-name", GTK_STOCK_NO, NULL);
		break;
	case CLIENT_STATE_READY:
		g_object_set(cell, "icon-name", GTK_STOCK_YES, NULL);
		break;
	default:
		g_object_set(cell, "icon-name", GTK_STOCK_DIALOG_ERROR, NULL);
		break;
	}
}

static void type_to_text(GtkTreeViewColumn *column, GtkCellRenderer *cell,
			GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	guint type, state;
	gchar *text;
	const char *title, *info;

	gtk_tree_model_get(model, iter, CLIENT_COLUMN_TYPE, &type,
					CLIENT_COLUMN_STATE, &state, -1);

	switch (type) {
	case CLIENT_TYPE_80203:
		title = N_("Ethernet");
		break;
	case CLIENT_TYPE_80211:
		title = N_("Wireless");
		break;
	default:
		title = N_("Unknown");
		break;
	}

	switch (state) {
	case CLIENT_STATE_OFF:
	case CLIENT_STATE_CARRIER:
		info = N_("Not Connected");
		break;
	case CLIENT_STATE_READY:
		info = N_("Connected");
		break;
	default:
		info = N_("Unknown State");
		break;
	}

	text = g_strdup_printf("<b>%s</b>\n<small>%s</small>", title, info);

	g_object_set(cell, "markup", text, NULL);

	g_free(text);
}

static void type_to_icon(GtkTreeViewColumn *column, GtkCellRenderer *cell,
			GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	guint type;

	gtk_tree_model_get(model, iter, CLIENT_COLUMN_TYPE, &type, -1);

	switch (type) {
	case CLIENT_TYPE_80203:
		g_object_set(cell, "icon-name", "network-wired",
						"stock-size", 5, NULL);
		break;
	case CLIENT_TYPE_80211:
		g_object_set(cell, "icon-name", "network-wireless",
						"stock-size", 5, NULL);
		break;
	default:
		g_object_set(cell, "icon-name", NULL, NULL);
		break;
	}
}

static void select_callback(GtkTreeSelection *selection, gpointer user_data)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkWidget *widget;
	gboolean selected;
	guint state;

	selected = gtk_tree_selection_get_selected(selection, &model, &iter);

	if (selected == FALSE) {
		gtk_widget_hide(interface_notebook);
		return;
	}

	gtk_tree_model_get(model, &iter, CLIENT_COLUMN_STATE, &state,
					CLIENT_COLUMN_USERDATA, &widget, -1);

	if (state == CLIENT_STATE_UNKNOWN) {
		gtk_widget_hide(interface_notebook);
		return;
	}

	gtk_widget_show(interface_notebook);
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
	gtk_widget_set_size_request(tree, 180, -1);
	gtk_container_add(GTK_CONTAINER(scrolled), tree);


	column = gtk_tree_view_column_new();
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


	model = client_get_active_model();
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), model);
	g_object_unref(model);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);

	interface_notebook = gtk_notebook_new();
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(interface_notebook), FALSE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(interface_notebook), FALSE);
	gtk_widget_set_no_show_all(interface_notebook, TRUE);
	gtk_box_pack_start(GTK_BOX(hbox), interface_notebook, TRUE, TRUE, 0);

	g_object_set_data(G_OBJECT(interface_notebook),
						"window", window);
	g_object_set_data(G_OBJECT(interface_notebook),
						"selection", selection);

	g_signal_connect(G_OBJECT(selection), "changed",
					G_CALLBACK(select_callback), window);

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
						widget, _("Interfaces"));

	gtk_widget_show_all(window);

	return window;
}

static void sig_term(int sig)
{
	gtk_main_quit();
}

int main(int argc, char *argv[])
{
	struct sigaction sa;

	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);

	gtk_init(&argc, &argv);

	gtk_window_set_default_icon_name("stock_internet");

	client_init(NULL);

	create_window();

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = sig_term;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	gtk_main();

	client_cleanup();

	return 0;
}
