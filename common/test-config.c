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

#include "marshal.h"
#include "marshal.c"

static GtkWidget *label_systemstate;
static GtkWidget *label_offlinemode;
static GtkWidget *label_available;
static GtkWidget *label_enabled;
static GtkWidget *label_connected;
static GtkWidget *label_default;

static void property_changed(DBusGProxy *proxy, const char *property,
					GValue *value, gpointer user_data)
{
	if (property == NULL || value == NULL)
		return;

	if (g_str_equal(property, "State") == TRUE)
		gtk_label_set_text(GTK_LABEL(label_systemstate),
						g_value_get_string(value));

	if (g_str_equal(property, "OfflineMode") == TRUE)
		gtk_label_set_text(GTK_LABEL(label_offlinemode),
			g_value_get_boolean(value) == TRUE ? "yes" : "no");

	if (g_str_equal(property, "AvailableTechnologies") == TRUE) {
		GString *text = g_string_sized_new(0);
		gchar **list = g_value_get_boxed(value);
		guint i;

		for (i = 0; i < g_strv_length(list); i++)
			g_string_append_printf(text, " %s", *(list + i));

		gtk_label_set_text(GTK_LABEL(label_available),
						g_string_free(text, FALSE));
	}

	if (g_str_equal(property, "EnabledTechnologies") == TRUE) {
		GString *text = g_string_sized_new(0);
		gchar **list = g_value_get_boxed(value);
		guint i;

		for (i = 0; i < g_strv_length(list); i++)
			g_string_append_printf(text, " %s", *(list + i));

		gtk_label_set_text(GTK_LABEL(label_enabled),
						g_string_free(text, FALSE));
	}

	if (g_str_equal(property, "ConnectedTechnologies") == TRUE) {
		GString *text = g_string_sized_new(0);
		gchar **list = g_value_get_boxed(value);
		guint i;

		for (i = 0; i < g_strv_length(list); i++)
			g_string_append_printf(text, " %s", *(list + i));

		gtk_label_set_text(GTK_LABEL(label_connected),
						g_string_free(text, FALSE));
	}

	if (g_str_equal(property, "DefaultTechnology") == TRUE)
		gtk_label_set_text(GTK_LABEL(label_default),
						g_value_get_string(value));
}

static void properties_callback(DBusGProxy *proxy,
				DBusGProxyCall *call, void *user_data)
{
	GHashTable *hash;
	GValue *value;
	GError *error = NULL;

	dbus_g_proxy_end_call(proxy, call, &error,
				dbus_g_type_get_map("GHashTable",
						G_TYPE_STRING, G_TYPE_VALUE),
					&hash, G_TYPE_INVALID);

	if (error != NULL) {
		g_printerr("%s\n", error->message);
		g_error_free(error);
		return;
	}

	value = g_hash_table_lookup(hash, "State");
	property_changed(proxy, "State", value, user_data);

	value = g_hash_table_lookup(hash, "OfflineMode");
	property_changed(proxy, "OfflineMode", value, user_data);

	value = g_hash_table_lookup(hash, "AvailableTechnologies");
	property_changed(proxy, "AvailableTechnologies", value, user_data);

	value = g_hash_table_lookup(hash, "EnabledTechnologies");
	property_changed(proxy, "EnabledTechnologies", value, user_data);

	value = g_hash_table_lookup(hash, "ConnectedTechnologies");
	property_changed(proxy, "ConnectedTechnologies", value, user_data);

	value = g_hash_table_lookup(hash, "DefaultTechnology");
	property_changed(proxy, "DefaultTechnology", value, user_data);
}

static void get_properties(DBusGProxy *proxy)
{
	if (dbus_g_proxy_begin_call_with_timeout(proxy, "GetProperties",
					properties_callback, NULL, NULL,
					120 * 1000, G_TYPE_INVALID) == FALSE) {
		g_printerr("Method call for retrieving properties failed\n");
		g_object_unref(proxy);
		return;
	}
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
	GtkWidget *buttonbox;
	GtkWidget *button;
	GtkWidget *hbox;
	GtkWidget *label;

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "Configuration Test");
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window), 420, 280);
	g_signal_connect(G_OBJECT(window), "delete-event",
					G_CALLBACK(delete_callback), NULL);

	mainbox = gtk_vbox_new(FALSE, 12);
	gtk_container_set_border_width(GTK_CONTAINER(mainbox), 12);
	gtk_container_add(GTK_CONTAINER(window), mainbox);

	buttonbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(buttonbox),
						GTK_BUTTONBOX_END);
	gtk_box_pack_end(GTK_BOX(mainbox), buttonbox, FALSE, FALSE, 0);

	button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	gtk_container_add(GTK_CONTAINER(buttonbox), button);
	g_signal_connect(G_OBJECT(button), "clicked",
					G_CALLBACK(close_callback), window);

	hbox = gtk_hbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(mainbox), 24);
	gtk_box_pack_start(GTK_BOX(mainbox), hbox, FALSE, FALSE, 0);
	label = gtk_label_new("System state:");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	label_systemstate = gtk_label_new(NULL);
	gtk_box_pack_end(GTK_BOX(hbox), label_systemstate, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(mainbox), 24);
	gtk_box_pack_start(GTK_BOX(mainbox), hbox, FALSE, FALSE, 0);
	label = gtk_label_new("Offline mode:");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	label_offlinemode = gtk_label_new(NULL);
	gtk_box_pack_end(GTK_BOX(hbox), label_offlinemode, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(mainbox), 24);
	gtk_box_pack_start(GTK_BOX(mainbox), hbox, FALSE, FALSE, 0);
	label = gtk_label_new("Available technologies:");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	label_available = gtk_label_new(NULL);
	gtk_box_pack_end(GTK_BOX(hbox), label_available, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(mainbox), 24);
	gtk_box_pack_start(GTK_BOX(mainbox), hbox, FALSE, FALSE, 0);
	label = gtk_label_new("Enabled technologies:");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	label_enabled = gtk_label_new(NULL);
	gtk_box_pack_end(GTK_BOX(hbox), label_enabled, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(mainbox), 24);
	gtk_box_pack_start(GTK_BOX(mainbox), hbox, FALSE, FALSE, 0);
	label = gtk_label_new("Connected technologies:");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	label_connected = gtk_label_new(NULL);
	gtk_box_pack_end(GTK_BOX(hbox), label_connected, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(mainbox), 24);
	gtk_box_pack_start(GTK_BOX(mainbox), hbox, FALSE, FALSE, 0);
	label = gtk_label_new("Default technology:");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	label_default = gtk_label_new(NULL);
	gtk_box_pack_end(GTK_BOX(hbox), label_default, FALSE, FALSE, 0);

	gtk_widget_show_all(window);

	return window;
}

int main(int argc, char *argv[])
{
	DBusGConnection *connection;
	DBusGProxy *proxy;
	GError *error = NULL;

	gtk_init(&argc, &argv);

	gtk_window_set_default_icon_name("network-wireless");

	dbus_g_object_register_marshaller(marshal_VOID__STRING_BOXED,
						G_TYPE_NONE, G_TYPE_STRING,
						G_TYPE_VALUE, G_TYPE_INVALID);

	connection = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);
	if (error != NULL) {
		g_printerr("%s\n", error->message);
		g_error_free(error);
	}

	proxy = dbus_g_proxy_new_for_name(connection, "org.moblin.connman",
					"/", "org.moblin.connman.Manager");

	create_window();

	dbus_g_proxy_add_signal(proxy, "PropertyChanged",
				G_TYPE_STRING, G_TYPE_VALUE, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(proxy, "PropertyChanged",
				G_CALLBACK(property_changed), NULL, NULL);

	get_properties(proxy);

	gtk_main();

	dbus_g_proxy_disconnect_signal(proxy, "PropertyChanged",
					G_CALLBACK(property_changed), NULL);

	g_object_unref(proxy);

	dbus_g_connection_unref(connection);

	return 0;
}
