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

#include <string.h>
#include <signal.h>

#include <gtk/gtk.h>

#include "client.h"

static void state_to_text(GtkTreeViewColumn *column, GtkCellRenderer *cell,
			GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	GtkTreeIter parent;
	gboolean active;
	guint state, signal;
	gchar *markup;
	const char *str;

	gtk_tree_model_get(model, iter, CLIENT_COLUMN_ACTIVE, &active,
					CLIENT_COLUMN_STATE, &state,
					CLIENT_COLUMN_SIGNAL, &signal, -1);

	if (gtk_tree_model_iter_parent(model, &parent, iter) == TRUE) {
		g_object_set(cell, "text",
				active == TRUE ? "Valid" : "", NULL);
		return;
	}

	switch (state) {
	case CLIENT_STATE_OFF:
		str = "Off";
		break;
	case CLIENT_STATE_ENABLED:
		str = "Enabled";
		break;
	case CLIENT_STATE_SCANNING:
		str = "Scanning";
		break;
	case CLIENT_STATE_CONNECT:
		str = "Connect";
		break;
	case CLIENT_STATE_CONNECTED:
		str = "Connected";
		break;
	case CLIENT_STATE_CARRIER:
		str = "Carrier";
		break;
	case CLIENT_STATE_CONFIGURE:
		str = "Configure";
		break;
	case CLIENT_STATE_READY:
		str = "Ready";
		break;
	case CLIENT_STATE_SHUTDOWN:
		str = "Shutdown";
		break;
	default:
		str = "Unknown";
		break;
	}

	markup = g_strdup_printf("%s\n<small>%d %%\n%s</small>", str,
				signal, active == TRUE ? "ON" : "OFF");
	g_object_set(cell, "markup", markup, NULL);
	g_free(markup);
}

static void type_to_text(GtkTreeViewColumn *column, GtkCellRenderer *cell,
			GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	GtkTreeIter parent;
	guint type;
	gchar *driver, *markup;
	const char *str;

	if (gtk_tree_model_iter_parent(model, &parent, iter) == TRUE) {
		g_object_set(cell, "text", NULL, NULL);
		return;
	}

	gtk_tree_model_get(model, iter, CLIENT_COLUMN_TYPE, &type,
					CLIENT_COLUMN_DRIVER, &driver, -1);

	switch (type) {
	case CLIENT_TYPE_80203:
		str = "IEEE 802.03";
		break;
	case CLIENT_TYPE_80211:
		str = "IEEE 802.11";
		break;
	default:
		str = "Unknown";
		break;
	}

	markup = g_strdup_printf("%s\n<small>%s\n</small>", str, driver);
	g_object_set(cell, "markup", markup, NULL);
	g_free(markup);

	g_free(driver);
}

static void policy_to_text(GtkTreeViewColumn *column, GtkCellRenderer *cell,
			GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	GtkTreeIter parent;
	guint policy;
	gchar *markup;
	const char *str;

	if (gtk_tree_model_iter_parent(model, &parent, iter) == TRUE) {
		g_object_set(cell, "text", NULL, NULL);
		return;
	}

	gtk_tree_model_get(model, iter, CLIENT_COLUMN_POLICY, &policy, -1);

	switch (policy) {
	case CLIENT_POLICY_OFF:
		str = "Off";
		break;
	case CLIENT_POLICY_IGNORE:
		str = "Ignore";
		break;
	case CLIENT_POLICY_AUTO:
		str = "Automatic";
		break;
	default:
		str = "Unknown";
		break;
	}

	markup = g_strdup_printf("%s\n<small>\n</small>", str);
	g_object_set(cell, "markup", markup, NULL);
	g_free(markup);
}

static void network_to_text(GtkTreeViewColumn *column, GtkCellRenderer *cell,
			GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	GtkTreeIter parent;
	gchar *essid, *psk, *markup;

	if (gtk_tree_model_iter_parent(model, &parent, iter) == TRUE) {
		gtk_tree_model_get(model, iter,
				CLIENT_COLUMN_NETWORK_ESSID, &essid, -1);
		g_object_set(cell, "text", essid, NULL);
		g_free(essid);
		return;
	}

	gtk_tree_model_get(model, iter, CLIENT_COLUMN_NETWORK_ESSID, &essid,
					CLIENT_COLUMN_NETWORK_PSK, &psk, -1);

	if (essid != NULL)
		markup = g_strdup_printf("%s\n<small>PSK: %s\n</small>",
				essid, psk && *psk != '\0' ? "Yes" : "No");
	else
		markup = g_strdup_printf("\n<small>\n</small>");
	g_object_set(cell, "markup", markup, NULL);
	g_free(markup);

	g_free(essid);
	g_free(psk);
}

static void ipv4_to_text(GtkTreeViewColumn *column, GtkCellRenderer *cell,
			GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	GtkTreeIter parent;
	guint state, method;
	gchar *address, *netmask, *gateway, *markup;
	const char *str;

	if (gtk_tree_model_iter_parent(model, &parent, iter) == TRUE) {
		g_object_set(cell, "text", NULL, NULL);
		return;
	}

	gtk_tree_model_get(model, iter, CLIENT_COLUMN_STATE, &state,
				CLIENT_COLUMN_IPV4_METHOD, &method,
				CLIENT_COLUMN_IPV4_ADDRESS, &address,
				CLIENT_COLUMN_IPV4_NETMASK, &netmask,
				CLIENT_COLUMN_IPV4_GATEWAY, &gateway, -1);

	switch (method) {
	case CLIENT_IPV4_METHOD_OFF:
		str = "Off";
		break;
	case CLIENT_IPV4_METHOD_STATIC:
		str = "Manually";
		break;
	case CLIENT_IPV4_METHOD_DHCP:
		str = "DHCP";
		break;
	default:
		str = "Unknown";
		break;
	}

	if (address != NULL && state == CLIENT_STATE_READY)
		markup = g_strdup_printf("%s\n<small>%s/%s\ngw %s</small>",
					str, address, netmask, gateway);
	else
		markup = g_strdup_printf("%s\n<small>\n</small>", str);
	g_object_set(cell, "markup", markup, NULL);
	g_free(markup);

	g_free(address);
	g_free(netmask);
	g_free(gateway);
}

static void select_callback(GtkTreeSelection *selection, gpointer user_data)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkWidget *widget;
	gboolean selected;
	gchar *network = NULL, *passphrase = NULL;

	selected = gtk_tree_selection_get_selected(selection, &model, &iter);
	if (selected == TRUE)
		gtk_tree_model_get(model, &iter,
				CLIENT_COLUMN_NETWORK_ESSID, &network,
				CLIENT_COLUMN_NETWORK_PSK, &passphrase, -1);

	widget = g_object_get_data(G_OBJECT(selection), "network");
	if (widget != NULL) {
		gtk_combo_box_remove_text(GTK_COMBO_BOX(widget), 0);
		gtk_combo_box_insert_text(GTK_COMBO_BOX(widget), 0,
						network ? network : "");
		gtk_combo_box_set_active(GTK_COMBO_BOX(widget), 0);
	}

	widget = g_object_get_data(G_OBJECT(selection), "passphrase");
	if (widget != NULL)
		gtk_entry_set_text(GTK_ENTRY(widget),
						passphrase ? passphrase : "");

	g_free(network);
	g_free(passphrase);
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
				"State", gtk_cell_renderer_text_new(),
					state_to_text, NULL, NULL);

	gtk_tree_view_insert_column_with_data_func(GTK_TREE_VIEW(tree), -1,
				"Type", gtk_cell_renderer_text_new(),
					type_to_text, NULL, NULL);

#if 0
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1,
				"Driver", gtk_cell_renderer_text_new(),
					"text", CLIENT_COLUMN_DRIVER, NULL);

	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1,
				"Vendor", gtk_cell_renderer_text_new(),
					"text", CLIENT_COLUMN_VENDOR, NULL);

	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1,
				"Product", gtk_cell_renderer_text_new(),
					"text", CLIENT_COLUMN_PRODUCT, NULL);
#endif

	gtk_tree_view_insert_column_with_data_func(GTK_TREE_VIEW(tree), -1,
				"Policy", gtk_cell_renderer_text_new(),
					policy_to_text, NULL, NULL);

	gtk_tree_view_insert_column_with_data_func(GTK_TREE_VIEW(tree), -1,
				"Network", gtk_cell_renderer_text_new(),
					network_to_text, NULL, NULL);\

	gtk_tree_view_insert_column_with_data_func(GTK_TREE_VIEW(tree), -1,
				"IPv4", gtk_cell_renderer_text_new(),
					ipv4_to_text, NULL, NULL);

	model = client_get_model();
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), model);
	g_object_unref(model);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);

	g_signal_connect(G_OBJECT(selection), "changed",
					G_CALLBACK(select_callback), NULL);

	gtk_tree_view_expand_all(GTK_TREE_VIEW(tree));

	return tree;
}

static void policy_off(GtkWidget *button, gpointer user_data)
{
	GtkTreeSelection *selection = user_data;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *index;

	if (gtk_tree_selection_get_selected(selection,
						&model, &iter) == FALSE)
		return;

	index = gtk_tree_model_get_string_from_iter(model, &iter);

	client_set_policy(index, CLIENT_POLICY_OFF);
}

static void policy_ignore(GtkWidget *button, gpointer user_data)
{
	GtkTreeSelection *selection = user_data;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *index;

	if (gtk_tree_selection_get_selected(selection,
						&model, &iter) == FALSE)
		return;

	index = gtk_tree_model_get_string_from_iter(model, &iter);

	client_set_policy(index, CLIENT_POLICY_IGNORE);
}

static void policy_auto(GtkWidget *button, gpointer user_data)
{
	GtkTreeSelection *selection = user_data;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *index;

	if (gtk_tree_selection_get_selected(selection,
						&model, &iter) == FALSE)
		return;

	index = gtk_tree_model_get_string_from_iter(model, &iter);

	client_set_policy(index, CLIENT_POLICY_AUTO);
}

static void dhcp_callback(GtkWidget *button, gpointer user_data)
{
	GtkTreeSelection *selection = user_data;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *index;

	if (gtk_tree_selection_get_selected(selection,
						&model, &iter) == FALSE)
		return;

	index = gtk_tree_model_get_string_from_iter(model, &iter);

	client_set_ipv4(index, CLIENT_IPV4_METHOD_DHCP);
}

static void static_callback(GtkWidget *button, gpointer user_data)
{
	GtkTreeSelection *selection = user_data;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *index;

	if (gtk_tree_selection_get_selected(selection,
						&model, &iter) == FALSE)
		return;

	index = gtk_tree_model_get_string_from_iter(model, &iter);

	client_set_ipv4(index, CLIENT_IPV4_METHOD_STATIC);
}

static void network_callback(GtkWidget *button, gpointer user_data)
{
	GtkTreeSelection *selection = user_data;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkWidget *widget;
	gchar *index, *network;
	const gchar *passphrase;

	if (gtk_tree_selection_get_selected(selection,
						&model, &iter) == FALSE)
		return;

	index = gtk_tree_model_get_string_from_iter(model, &iter);

	widget = g_object_get_data(G_OBJECT(selection), "network");
	if (widget == NULL)
		return;

	network = gtk_combo_box_get_active_text(GTK_COMBO_BOX(widget));

	widget = g_object_get_data(G_OBJECT(selection), "passphrase");
	if (widget == NULL)
		return;

	passphrase = gtk_entry_get_text(GTK_ENTRY(widget));

	client_set_network(index, network, passphrase);

	g_free(network);
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
	GtkWidget *mainbox;
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *tree;
	GtkWidget *scrolled;
	GtkWidget *buttonbox;
	GtkWidget *button;
	GtkWidget *combo;
	GtkWidget *entry;
	GtkTreeSelection *selection;

	tree = create_tree();
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "Client Test");
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);
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

	hbox = gtk_hbox_new(FALSE, 8);
	gtk_box_pack_start(GTK_BOX(mainbox), hbox, FALSE, FALSE, 0);

	buttonbox = gtk_vbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(buttonbox),
						GTK_BUTTONBOX_START);
	gtk_box_pack_start(GTK_BOX(hbox), buttonbox, FALSE, FALSE, 0);

	button = gtk_button_new_with_label("Policy Off");
	gtk_container_add(GTK_CONTAINER(buttonbox), button);
	g_signal_connect(G_OBJECT(button), "clicked",
				G_CALLBACK(policy_off), selection);

	button = gtk_button_new_with_label("Policy Ignore");
	gtk_container_add(GTK_CONTAINER(buttonbox), button);
	g_signal_connect(G_OBJECT(button), "clicked",
				G_CALLBACK(policy_ignore), selection);

	button = gtk_button_new_with_label("Policy Auto");
	gtk_container_add(GTK_CONTAINER(buttonbox), button);
	g_signal_connect(G_OBJECT(button), "clicked",
				G_CALLBACK(policy_auto), selection);

	buttonbox = gtk_vbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(buttonbox),
						GTK_BUTTONBOX_START);
	gtk_box_pack_start(GTK_BOX(hbox), buttonbox, FALSE, FALSE, 0);

	button = gtk_button_new_with_label("DHCP");
	gtk_container_add(GTK_CONTAINER(buttonbox), button);
	g_signal_connect(G_OBJECT(button), "clicked",
				G_CALLBACK(dhcp_callback), selection);

	button = gtk_button_new_with_label("Static IP");
	gtk_container_add(GTK_CONTAINER(buttonbox), button);
	g_signal_connect(G_OBJECT(button), "clicked",
				G_CALLBACK(static_callback), selection);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);

	combo = gtk_combo_box_entry_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "");
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "Guest");
	gtk_box_pack_start(GTK_BOX(vbox), combo, FALSE, FALSE, 0);
	g_object_set_data(G_OBJECT(selection), "network", combo);

	entry = gtk_entry_new();
	gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);
	gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 0);
	g_object_set_data(G_OBJECT(selection), "passphrase", entry);

	button = gtk_button_new_with_label("Set Network");
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(button), "clicked",
				G_CALLBACK(network_callback), selection);

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

static void sig_term(int sig)
{
	gtk_main_quit();
}

int main(int argc, char *argv[])
{
	struct sigaction sa;

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
