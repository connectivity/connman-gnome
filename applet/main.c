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
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "marshal.h"
#include "marshal.c"

#include "properties.h"
#include "status.h"
#include "agent.h"

static gboolean global_ready = FALSE;
static gint global_strength = -1;

static void service_property_changed(DBusGProxy *proxy, const char *property,
					GValue *value, gpointer user_data)
{
	if (property == NULL || value == NULL)
		return;

	if (g_str_equal(property, "Type") == TRUE) {
		const gchar *type = g_value_get_string(value);

		if (g_strcmp0(type, "ethernet") == 0)
			global_strength = -1;
		else if (g_strcmp0(type, "wifi") == 0)
			global_strength = g_value_get_uchar(value);
	} else if (g_str_equal(property, "State") == TRUE) {
		const gchar *state = g_value_get_string(value);

		if (g_strcmp0(state, "ready") == 0 || g_strcmp0(state, "online") == 0)
			global_ready = TRUE;
		else
			global_ready = FALSE;
	} else if (g_str_equal(property, "Strength") == TRUE) {
		const guchar strength = g_value_get_uchar(value);

		global_strength = strength;
	}

	if (global_ready == TRUE)
		status_ready(global_strength);
	else
		status_offline();
}

static DBusGProxy *service = NULL;

static void update_service(DBusGProxy *proxy, const char *path)
{
	if (path == NULL) {
		status_offline();
		return;
	}

	if (service != NULL) {
		if (g_strcmp0(dbus_g_proxy_get_path(service), path) == 0)
			return;

		properties_destroy(service);
	}

	service = dbus_g_proxy_new_from_proxy(proxy,
					"net.connman.Service", path);

	properties_create(service, service_property_changed, NULL);
}

static void iterate_list(const GValue *value, gpointer user_data)
{
	gchar **path = user_data;

	if (*path == NULL)
		*path = g_value_dup_boxed(value);
}

static void manager_property_changed(DBusGProxy *proxy, const char *property,
					GValue *value, gpointer user_data)
{
	if (property == NULL || value == NULL)
		return;

	if (g_str_equal(property, "Services") == TRUE) {
		gchar *path = NULL;

		dbus_g_type_collection_value_iterate(value,
						iterate_list, &path);
		update_service(proxy, path);
		g_free(path);
	} else if (g_str_equal(property, "State") == TRUE) {
		const gchar *state = g_value_get_string(value);

		if (g_strcmp0(state, "ready") == 0 || g_strcmp0(state, "online") == 0) {
			global_ready = TRUE;
			status_ready(global_strength);
		} else {
			global_ready = FALSE;
			status_offline();
		}
	}
}

static DBusGProxy *manager = NULL;

static void manager_init(DBusGConnection *connection)
{
	manager = dbus_g_proxy_new_for_name(connection, "net.connman",
					"/", "net.connman.Manager");

	properties_create(manager, manager_property_changed, NULL);
	setup_agents();
}

static void manager_cleanup(void)
{
	properties_destroy(manager);
}

static void name_owner_changed(DBusGProxy *proxy, const char *name,
			const char *prev, const char *new, gpointer user_data)
{
	if (g_str_equal(name, "net.connman") == FALSE)
		return;

	if (*new != '\0') {
		status_offline();
		properties_enable(manager);
		setup_agents();
	} else {
		properties_disable(manager);
		status_unavailable();
	}
}

static void open_uri(GtkWindow *parent, const char *uri)
{
	GtkWidget *dialog;
	GdkScreen *screen;
	GError *error = NULL;
	gchar *cmdline;

	screen = gtk_window_get_screen(parent);

	cmdline = g_strconcat("xdg-open ", uri, NULL);

	if (gdk_spawn_command_line_on_screen(screen,
				cmdline, &error) == FALSE) {
		dialog = gtk_message_dialog_new(parent,
				GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
				GTK_BUTTONS_CLOSE, "%s", error->message);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		g_error_free(error);
	}

	g_free(cmdline);
}

static void about_url_hook(GtkAboutDialog *dialog,
				const gchar *url, gpointer data)
{
	open_uri(GTK_WINDOW(dialog), url);
}

static void about_email_hook(GtkAboutDialog *dialog,
				const gchar *email, gpointer data)
{
	gchar *uri;

	uri = g_strconcat("mailto:", email, NULL);
	open_uri(GTK_WINDOW(dialog), uri);
	g_free(uri);
}

static void about_callback(GtkWidget *item, gpointer user_data)
{
	const gchar *authors[] = {
		"Marcel Holtmann <marcel@holtmann.org>",
		NULL
	};

	gtk_about_dialog_set_url_hook(about_url_hook, NULL, NULL);
	gtk_about_dialog_set_email_hook(about_email_hook, NULL, NULL);

	gtk_show_about_dialog(NULL, "version", VERSION,
			"copyright", "Copyright \xc2\xa9 2008 Intel Corporation",
			"comments", _("A connection manager for the GNOME desktop"),
			"authors", authors,
			"translator-credits", _("translator-credits"),
			"logo-icon-name", "network-wireless", NULL);
}

static void settings_callback(GtkWidget *item, gpointer user_data)
{
	const char *command = "connman-properties";

	if (g_spawn_command_line_async(command, NULL) == FALSE)
		g_printerr("Couldn't execute command: %s\n", command);
}

static gboolean menu_callback(GtkMenu *menu)
{
	GtkWidget *item;

	item = gtk_separator_menu_item_new();
	gtk_widget_show(item);

	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	item = gtk_image_menu_item_new_from_stock(GTK_STOCK_PREFERENCES, NULL);
	g_signal_connect(item, "activate", G_CALLBACK(settings_callback), NULL);
	gtk_widget_show(item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ABOUT, NULL);
	g_signal_connect(item, "activate", G_CALLBACK(about_callback), NULL);
	gtk_widget_show(item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	return TRUE;
}


int main(int argc, char *argv[])
{
	DBusGConnection *connection;
	DBusGProxy *proxy;
	GError *error = NULL;

	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);

	gtk_init(&argc, &argv);
	gtk_window_set_default_icon_name("network-wireless");

	g_set_application_name(_("Connection Manager"));

	dbus_g_object_register_marshaller(marshal_VOID__STRING_BOXED,
						G_TYPE_NONE, G_TYPE_STRING,
						G_TYPE_VALUE, G_TYPE_INVALID);

	connection = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);
	if (error != NULL) {
		g_printerr("%s\n", error->message);
		g_error_free(error);
		return 1;
	}

	status_init(menu_callback, NULL);

	proxy = dbus_g_proxy_new_for_name(connection, DBUS_SERVICE_DBUS,
					DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS);

	dbus_g_proxy_add_signal(proxy, "NameOwnerChanged",
					G_TYPE_STRING, G_TYPE_STRING,
					G_TYPE_STRING, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(proxy, "NameOwnerChanged",
				G_CALLBACK(name_owner_changed), NULL, NULL);

	manager_init(connection);

	gtk_main();

	manager_cleanup();

	dbus_g_proxy_disconnect_signal(proxy, "NameOwnerChanged",
					G_CALLBACK(name_owner_changed), NULL);

	g_object_unref(proxy);

	status_cleanup();

	dbus_g_connection_unref(connection);

	return 0;
}
