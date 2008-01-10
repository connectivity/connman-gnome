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

static GtkWidget *interface_notebook;

struct config_data {
	GtkWidget *widget;
	GtkWidget *title;
	GtkWidget *label;

	GtkWidget *window;
	GtkTreeModel *model;
	gchar *index;
};

static void update_status(struct config_data *data, guint type, guint state,
				const gchar *network, const gchar *address)
{
	const char *str;
	gchar *markup, *info = NULL;

	switch (type) {
	case CLIENT_TYPE_80203:
		switch (state) {
		case CLIENT_STATE_CARRIER:
			str = N_("Cable Connected");
			break;
		case CLIENT_STATE_READY:
			str = N_("Not Connected");
			break;
		default:
			str = N_("Cable Unplugged");
			info = g_strdup_printf(_("The cable for %s is "
					"not plugged in."), N_("Ethernet"));
			break;
		}
		break;

	case CLIENT_TYPE_80211:
		if (state == CLIENT_STATE_READY) {
			str = N_("Connected");
			info = g_strdup_printf(_("%s is connected to %s "
					"and has the IP address %s."),
					N_("Wireless"), network, address);
		} else
			str = N_("Not Connected");
		break;

	default:
		if (state == CLIENT_STATE_READY)
			str = N_("Connected");
		else
			str = N_("Not Connected");
		break;
	}

	markup = g_strdup_printf("<b>%s</b>", str);
	gtk_label_set_markup(GTK_LABEL(data->title), markup);
	g_free(markup);

	gtk_label_set_markup(GTK_LABEL(data->label), info);

	g_free(info);
}

static void update_config(struct config_data *data)
{
	GtkTreeIter iter;
	guint type, state;
	gchar *network, *address;

	if (gtk_tree_model_get_iter_from_string(data->model,
						&iter, data->index) == FALSE)
		return;

	gtk_tree_model_get(data->model, &iter,
				CLIENT_COLUMN_TYPE, &type,
				CLIENT_COLUMN_STATE, &state,
				CLIENT_COLUMN_NETWORK_ESSID, &network,
				CLIENT_COLUMN_IPV4_ADDRESS, &address, -1);

	update_status(data, type, state, network, address);

	g_free(network);
	g_free(address);
}

static void advanced_callback(GtkWidget *button, gpointer user_data)
{
}

static struct config_data *create_config(GtkTreeModel *model,
					GtkTreeIter *iter, gpointer user_data)
{
	GtkWidget *mainbox;
	GtkWidget *label;
	GtkWidget *hbox;
	GtkWidget *button;
	struct config_data *data;
	guint type, state;
	gchar *markup, *vendor, *product, *network, *address;

	data = g_try_new0(struct config_data, 1);
	if (data == NULL)
		return NULL;

	gtk_tree_model_get(model, iter,
				CLIENT_COLUMN_TYPE, &type,
				CLIENT_COLUMN_VENDOR, &vendor,
				CLIENT_COLUMN_PRODUCT, &product,
				CLIENT_COLUMN_STATE, &state,
				CLIENT_COLUMN_NETWORK_ESSID, &network,
				CLIENT_COLUMN_IPV4_ADDRESS, &address, -1);

	mainbox = gtk_vbox_new(FALSE, 12);
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

	update_status(data, type, state, network, address);

	g_free(network);
	g_free(address);


	hbox = gtk_hbox_new(FALSE, 12);
	gtk_box_pack_end(GTK_BOX(mainbox), hbox, FALSE, FALSE, 0);

	label = gtk_label_new(NULL);
	gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 1.0);
	gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);

	markup = g_strdup_printf("%s\n<small>%s</small>",
			vendor ? vendor : "", product ? product : "");
	gtk_label_set_markup(GTK_LABEL(label), markup);
	g_free(markup);

	button = gtk_button_new_with_label(_("Advanced..."));
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(button), "clicked",
				G_CALLBACK(advanced_callback), data);

	data->window = user_data;
	data->model = model;
	data->index = gtk_tree_model_get_string_from_iter(model, iter);
	client_set_userdata(data->index, data);

	gtk_widget_show_all(mainbox);

	g_free(product);
	g_free(vendor);

	return data;
}

static void select_callback(GtkTreeSelection *selection, gpointer user_data)
{
	GtkWidget *notebook = interface_notebook;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean selected;
	struct config_data *data;
	guint state;
	gint page;

	selected = gtk_tree_selection_get_selected(selection, &model, &iter);
	if (selected == FALSE) {
		gtk_widget_hide(interface_notebook);
		return;
	}

	gtk_tree_model_get(model, &iter, CLIENT_COLUMN_STATE, &state,
					CLIENT_COLUMN_USERDATA, &data, -1);

	if (state == CLIENT_STATE_UNKNOWN) {
		gtk_widget_hide(interface_notebook);
		return;
	}

	if (data == NULL) {
		data = create_config(model, &iter, user_data);
		if (data == NULL)
			return;

		page = gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
							data->widget, NULL);
	} else {
		update_config(data);

		page = gtk_notebook_page_num(GTK_NOTEBOOK(notebook),
								data->widget);
	}

	gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), page);

	gtk_widget_show(notebook);
}

static void row_changed(GtkTreeModel *model, GtkTreePath  *path,
				GtkTreeIter  *iter, gpointer user_data)
{
	struct config_data *data;

	gtk_tree_model_get(model, iter, CLIENT_COLUMN_USERDATA, &data, -1);

	if (data != NULL)
		update_config(data);
}

static void state_to_icon(GtkTreeViewColumn *column, GtkCellRenderer *cell,
			GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	guint state;

	gtk_tree_model_get(model, iter, CLIENT_COLUMN_STATE, &state, -1);

	switch (state) {
	case CLIENT_STATE_OFF:
	case CLIENT_STATE_ENABLED:
	case CLIENT_STATE_CONNECT:
	case CLIENT_STATE_CONFIG:
	case CLIENT_STATE_CARRIER:
	case CLIENT_STATE_SHUTDOWN:
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
	gchar *markup;
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
	case CLIENT_STATE_ENABLED:
	case CLIENT_STATE_CONNECT:
	case CLIENT_STATE_CONFIG:
		info = N_("Not Connected");
		break;
	case CLIENT_STATE_CARRIER:
		info = N_("Carrier");
		break;
	case CLIENT_STATE_READY:
		info = N_("Connected");
		break;
	case CLIENT_STATE_SHUTDOWN:
		info = N_("Shutdown");
		break;
	default:
		info = N_("Unknown State");
		break;
	}

	markup = g_strdup_printf("<b>%s</b>\n<small>%s</small>", title, info);
	g_object_set(cell, "markup", markup, NULL);
	g_free(markup);
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
	gtk_tree_view_column_pack_end(column, renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func(column, renderer,
						type_to_icon, NULL, NULL);


	interface_notebook = gtk_notebook_new();
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(interface_notebook), FALSE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(interface_notebook), FALSE);
	gtk_widget_set_no_show_all(interface_notebook, TRUE);
	gtk_box_pack_start(GTK_BOX(hbox), interface_notebook, TRUE, TRUE, 0);

	model = client_get_active_model();
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), model);
	g_object_unref(model);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
	g_signal_connect(G_OBJECT(selection), "changed",
					G_CALLBACK(select_callback), window);

	g_signal_connect(G_OBJECT(model), "row-changed",
					G_CALLBACK(row_changed), selection);

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
