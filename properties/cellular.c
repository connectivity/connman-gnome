/*
 *
 *  Connection Manager
 *
 *  Copyright (C) 2011  Intel Corporation. All rights reserved.
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
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "connman-client.h"

#include "advanced.h"

static void connect_reply_cb(DBusGProxy *proxy, GError *error,
		gpointer user_data)
{
	if (error)
		g_error_free(error);
}

static void connect_callback(GtkWidget *editable, gpointer user_data)
{
	struct config_data *data = user_data;

	connman_client_connect_async(data->client, data->device, connect_reply_cb, data);
}


static void disconnect_callback(GtkWidget *editable, gpointer user_data)
{
	struct config_data *data = user_data;

	connman_client_disconnect(data->client, data->device);
}

static void switch_callback(GtkWidget *editable, gpointer user_data)
{
	struct config_data *data = user_data;
	const gchar *label = gtk_button_get_label(GTK_BUTTON(data->cellular_button));

	if (g_str_equal(label, "Disable"))
		connman_client_set_powered(data->client, data->device, FALSE);
	else
		connman_client_set_powered(data->client, data->device, TRUE);
}

void add_cellular_switch_button(GtkWidget *mainbox, GtkTreeIter *iter,
				struct config_data *data)
{
	GtkWidget *vbox;
	GtkWidget *table;
	GtkWidget *label;
	GtkWidget *buttonbox;
	GtkWidget *button;
	gboolean cellular_enabled;

	gtk_tree_model_get(data->model, iter,
			CONNMAN_COLUMN_POWERED, &cellular_enabled,
			-1);

	vbox = gtk_vbox_new(TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 24);
	gtk_box_pack_start(GTK_BOX(mainbox), vbox, FALSE, FALSE, 0);

	table = gtk_table_new(1, 1, TRUE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 10);
	gtk_table_set_col_spacings(GTK_TABLE(table), 10);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);

	label = gtk_label_new(_("Configure Cellular Networks."));
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);

	buttonbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(buttonbox), GTK_BUTTONBOX_CENTER);
	gtk_box_pack_start(GTK_BOX(mainbox), buttonbox, FALSE, FALSE, 0);

	button = gtk_button_new();
	data->cellular_button = button;

	if (cellular_enabled)
		gtk_button_set_label(GTK_BUTTON(button), _("Disable"));
	else
		gtk_button_set_label(GTK_BUTTON(button), _("Enable"));

	gtk_container_add(GTK_CONTAINER(buttonbox), button);
	g_signal_connect(G_OBJECT(button), "clicked",
			G_CALLBACK(switch_callback), data);
}

void add_cellular_service(GtkWidget *mainbox, GtkTreeIter *iter, struct config_data *data)
{
	GtkWidget *vbox;
	GtkWidget *table;
	GtkWidget *label;
	GtkWidget *button;

	const char *name, *icon, *state;
	guint strength;

	gtk_tree_model_get(data->model, iter,
			CONNMAN_COLUMN_NAME, &name,
			CONNMAN_COLUMN_ICON, &icon,
			CONNMAN_COLUMN_STATE, &state,
			CONNMAN_COLUMN_STRENGTH, &strength,
			-1);

	if (g_str_equal(state, "failure") == TRUE)
		connman_client_remove(data->client, data->device);

	vbox = gtk_vbox_new(TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 24);
	gtk_box_pack_start(GTK_BOX(mainbox), vbox, FALSE, FALSE, 0);

	table = gtk_table_new(4, 8, TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);
	data->table = table;

	label = gtk_label_new(_("Access Point:"));
	gtk_table_attach_defaults(GTK_TABLE(table), label, 3, 4, 0, 1);

	label = gtk_label_new(_(name));
	gtk_table_attach_defaults(GTK_TABLE(table), label, 4, 5, 0, 1);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	data->cellular.name = label;

	label = gtk_label_new(_(""));
	gtk_table_attach_defaults(GTK_TABLE(table), label, 3, 5, 2, 3);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
	gtk_widget_hide(label);
	data->cellular.connect_info = label;

	button = gtk_button_new_with_label(_("Connect"));
	gtk_table_attach_defaults(GTK_TABLE(table), button, 3, 5, 3, 4);
	g_signal_connect(G_OBJECT(button), "clicked",
			G_CALLBACK(connect_callback), data);
	gtk_widget_set_no_show_all(button, TRUE);
	data->cellular.connect = button;

	button = gtk_button_new_with_label(_("Disconnect"));
	gtk_table_attach_defaults(GTK_TABLE(table), button, 3, 5, 3, 4);
	g_signal_connect(G_OBJECT(button), "clicked",
			G_CALLBACK(disconnect_callback), data);
	data->cellular.disconnect = button;
	gtk_widget_set_no_show_all(button, TRUE);

	if (g_str_equal(state, "failure") == TRUE
			|| g_str_equal(state, "idle") == TRUE) {
		gtk_widget_show(data->cellular.connect);
		gtk_widget_hide(data->cellular.disconnect);
	} else {
		gtk_widget_hide(data->cellular.connect);
		gtk_widget_show(data->cellular.disconnect);
	}
}
