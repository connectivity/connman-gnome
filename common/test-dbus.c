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

static GMainLoop *mainloop;
static DBusGConnection *connection;

static void print_properties(GHashTable *hash)
{
	GValue *value;

	value = g_hash_table_lookup(hash, "State");
	if (value != NULL)
		g_print("State: %s\n", g_value_get_string(value));

	value = g_hash_table_lookup(hash, "OfflineMode");
	if (value != NULL)
		g_print("OfflineMode: %d\n", g_value_get_boolean(value));

	value = g_hash_table_lookup(hash, "Technologies");
	if (value != NULL) {
		gchar **list = g_value_get_boxed(value);
		guint i;

		g_print("Technologies:");
		for (i = 0; i < g_strv_length(list); i++)
			g_print(" %s", *(list + i));
		g_print("\n");
	}
}

static void properties_callback(DBusGProxy *proxy,
				DBusGProxyCall *call, void *user_data)
{
	GHashTable *hash;
	GError *error = NULL;

	dbus_g_proxy_end_call(proxy, call, &error,
				dbus_g_type_get_map("GHashTable",
						G_TYPE_STRING, G_TYPE_VALUE),
					&hash, G_TYPE_INVALID);

	if (error != NULL) {
		g_printerr("%s\n", error->message);
		g_error_free(error);
	} else
		print_properties(hash);

	g_object_unref(proxy);

	g_main_loop_quit(mainloop);
}

static void get_properties(const char *path, const char *interface)
{
	DBusGProxy *proxy;

	proxy = dbus_g_proxy_new_for_name(connection, "org.moblin.connman",
							path, interface);

	if (dbus_g_proxy_begin_call_with_timeout(proxy, "GetProperties",
					properties_callback, NULL, NULL,
					120 * 1000, G_TYPE_INVALID) == FALSE) {
		g_printerr("Method call for retrieving properties failed\n");
		g_object_unref(proxy);
		return;
	}
}

int main(int argc, char *argv[])
{
	GError *error = NULL;

	g_type_init();

	mainloop = g_main_loop_new(NULL, FALSE);

	connection = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);
	if (error != NULL) {
		g_printerr("%s\n", error->message);
		g_error_free(error);
	}

	get_properties("/", "org.moblin.connman.Manager");

	g_main_loop_run(mainloop);

	dbus_g_connection_unref(connection);

	g_main_loop_unref(mainloop);

	return 0;
}
