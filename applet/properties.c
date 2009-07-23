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

#include "properties.h"

struct properties_data {
	property_callback_t callback;
	gpointer user_data;
	DBusGProxy *proxy;
};

static void properties_foreach(gpointer key, gpointer value,
						gpointer user_data)
{
	struct properties_data *data = user_data;

	data->callback(data->proxy, key, value, data->user_data);
}

static void properties_callback(DBusGProxy *proxy,
				DBusGProxyCall *call, void *user_data)
{
	struct properties_data *data = user_data;
	GHashTable *hash;
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

	if (data->callback == NULL)
		return;

	g_hash_table_foreach(hash, properties_foreach, user_data);
}

static void get_properties(DBusGProxy *proxy, property_callback_t callback,
							gpointer user_data)
{
	struct properties_data *data;

	data = g_try_new0(struct properties_data, 1);
	if (data == NULL)
		return;

	data->callback = callback;
	data->user_data = user_data;
	data->proxy = proxy;

	if (dbus_g_proxy_begin_call_with_timeout(proxy, "GetProperties",
					properties_callback, data, g_free,
					120 * 1000, G_TYPE_INVALID) == FALSE) {
		g_printerr("Method call for retrieving properties failed\n");
		g_free(data);
	}
}

void properties_enable(DBusGProxy *proxy)
{
	property_callback_t callback;
	gpointer user_data;

	callback = g_object_get_data(G_OBJECT(proxy), "properties_callback");
	if (callback == NULL)
		return;

	user_data = g_object_get_data(G_OBJECT(proxy), "properties_userdata");

	dbus_g_proxy_connect_signal(proxy, "PropertyChanged",
				G_CALLBACK(callback), user_data, NULL);

	get_properties(proxy, callback, user_data);
}

void properties_create(DBusGProxy *proxy, property_callback_t callback,
							gpointer user_data)
{
	g_object_set_data(G_OBJECT(proxy), "properties_callback", callback);
	g_object_set_data(G_OBJECT(proxy), "properties_userdata", user_data);

	dbus_g_proxy_add_signal(proxy, "PropertyChanged",
				G_TYPE_STRING, G_TYPE_VALUE, G_TYPE_INVALID);

	properties_enable(proxy);
}

void properties_disable(DBusGProxy *proxy)
{
	property_callback_t callback;
	gpointer user_data;

	callback = g_object_get_data(G_OBJECT(proxy), "properties_callback");
	if (callback == NULL)
		return;

	user_data = g_object_get_data(G_OBJECT(proxy), "properties_userdata");

	dbus_g_proxy_disconnect_signal(proxy, "PropertyChanged",
				G_CALLBACK(callback), user_data);
}

void properties_destroy(DBusGProxy *proxy)
{
	properties_disable(proxy);

	g_object_set_data(G_OBJECT(proxy), "properties_callback", NULL);
	g_object_set_data(G_OBJECT(proxy), "properties_userdata", NULL);

	g_object_unref(proxy);
}
