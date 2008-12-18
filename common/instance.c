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

#include "instance.h"

#define CONNMAN_SERVICE    "org.moblin.connman"
#define CONNMAN_INSTANCE   CONNMAN_SERVICE ".Instance"

static DBusGConnection *connection;

static GtkWindow *instance_window;

static gboolean instance_present(GObject *self, GError **error)
{
	gtk_window_present(instance_window);

	return TRUE;
}

#include "instance-glue.h"

void instance_register(GtkWindow *window)
{
	instance_window = window;

	dbus_g_object_type_install_info(GTK_TYPE_WINDOW,
					&dbus_glib_instance_object_info);

	dbus_g_connection_register_g_object(connection, "/", G_OBJECT(window));
}

gboolean instance_init(const gchar *name)
{
	DBusGProxy *proxy;
	GError *error = NULL;
	guint result;

	connection = dbus_g_bus_get(DBUS_BUS_SESSION, &error);
	if (error != NULL) {
		g_printerr("Can't get session bus: %s", error->message);
		g_error_free(error);
		return FALSE;
	}

	proxy = dbus_g_proxy_new_for_name(connection, DBUS_SERVICE_DBUS,
					DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS);

	if (dbus_g_proxy_call(proxy, "RequestName", NULL,
			G_TYPE_STRING, name, G_TYPE_UINT, 0, G_TYPE_INVALID,
			G_TYPE_UINT, &result, G_TYPE_INVALID) == FALSE) {
		g_printerr("Can't get unique name on session bus");
		g_object_unref(proxy);
		dbus_g_connection_unref(connection);
		return FALSE;
	}

	g_object_unref(proxy);

	if (result != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
		proxy = dbus_g_proxy_new_for_name(connection, name, "/",
						CONNMAN_SERVICE ".Instance");

		dbus_g_proxy_call_no_reply(proxy, "Present",
					G_TYPE_INVALID, G_TYPE_INVALID);

		g_object_unref(G_OBJECT(proxy));
		dbus_g_connection_unref(connection);
		return FALSE;
	}

	return TRUE;
}

void instance_cleanup(void)
{
	dbus_g_connection_unref(connection);
}
