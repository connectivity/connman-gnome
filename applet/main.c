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

#include "status.h"

static void close_callback(GtkWidget *dialog, gpointer user_data)
{
	gtk_widget_destroy(dialog);
}

static void about_url_hook(GtkAboutDialog *dialog,
					const gchar *url, gpointer data)
{
}

static void about_email_hook(GtkAboutDialog *dialog,
					const gchar *email, gpointer data)
{
}

static void about_callback(GtkWidget *item, gpointer user_data)
{
	const gchar *authors[] = {
		"Marcel Holtmann <marcel@holtmann.org>",
		NULL
	};
	GtkWidget *dialog;

	dialog = gtk_about_dialog_new();

	gtk_about_dialog_set_name(GTK_ABOUT_DIALOG(dialog),
						_("Connection Manager"));
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog), VERSION);
	gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(dialog),
				"Copyright \xc2\xa9 2008 Intel Corporation");
	gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dialog),
			_("A connection manager for the GNOME desktop"));
	gtk_about_dialog_set_logo_icon_name(GTK_ABOUT_DIALOG(dialog),
							"stock_internet");

	gtk_about_dialog_set_url_hook(about_url_hook, NULL, NULL);
	gtk_about_dialog_set_email_hook(about_email_hook, NULL, NULL);

	gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(dialog), authors);
	gtk_about_dialog_set_translator_credits(GTK_ABOUT_DIALOG(dialog),
						_("translator-credits"));

	g_signal_connect(dialog, "close", G_CALLBACK(close_callback), NULL);
	g_signal_connect(dialog, "response", G_CALLBACK(close_callback), NULL);

	gtk_widget_show_all(dialog);
}

static void settings_callback(GObject *widget, gpointer user_data)
{
	const char *command = "connman-properties";

	if (g_spawn_command_line_async(command, NULL) == FALSE)
		g_printerr("Couldn't execute command: %s\n", command);
}

static GtkWidget *create_popupmenu(void)
{
	GtkWidget *menu;
	GtkWidget *item;

	menu = gtk_menu_new();

	item = gtk_image_menu_item_new_from_stock(GTK_STOCK_PREFERENCES, NULL);
	g_signal_connect(item, "activate", G_CALLBACK(settings_callback), NULL);
	gtk_widget_show(item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ABOUT, NULL);
	g_signal_connect(item, "activate", G_CALLBACK(about_callback), NULL);
	gtk_widget_show(item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	return menu;
}

static void state_changed(DBusGProxy *manager,
				const char *state, gpointer user_data)
{
	if (g_ascii_strcasecmp(state, "offline") == 0)
		status_offline();
	else if (g_ascii_strcasecmp(state, "prepare") == 0)
		status_prepare();
	else if (g_ascii_strcasecmp(state, "config") == 0)
		status_config();
	else if (g_ascii_strcasecmp(state, "ready") == 0)
		status_ready();
	else
		status_hide();
}

static DBusGProxy *create_manager(DBusGConnection *conn)
{
	DBusGProxy *manager;
	GError *error = NULL;
	const char *state;

	manager = dbus_g_proxy_new_for_name(conn, "org.freedesktop.connman",
					"/", "org.freedesktop.connman.Manager");

	dbus_g_proxy_add_signal(manager, "StateChanged",
					G_TYPE_STRING, G_TYPE_INVALID);

	dbus_g_proxy_connect_signal(manager, "StateChanged",
					G_CALLBACK(state_changed), NULL, NULL);

	dbus_g_proxy_call(manager, "GetState", &error,
			G_TYPE_INVALID, G_TYPE_STRING, &state, G_TYPE_INVALID);

	if (error != NULL) {
		state = "unknown";
		g_error_free(error);
	}

	state_changed(manager, state, NULL);

	return manager;
}

static void name_owner_changed(DBusGProxy *system, const char *name,
			const char *prev, const char *new, gpointer user_data)
{
	if (!strcmp(name, "org.freedesktop.connman") && *new == '\0')
		status_hide();
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
	GtkWidget *popup;
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

	popup = create_popupmenu();

	status_init(NULL, popup);

	system = create_system(conn);
	manager = create_manager(conn);

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = sig_term;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	gtk_main();

	g_object_unref(manager);
	g_object_unref(system);

	status_cleanup();

	dbus_g_connection_unref(conn);

	return 0;
}
