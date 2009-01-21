/*
 *
 *  Connection Manager
 *
 *  Copyright (C) 2008  Intel Corporation. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License version 2.1 as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "connman-client.h"

#include "connman-dbus.h"
#include "connman-dbus-glue.h"

#ifdef DEBUG
#define DBG(fmt, arg...) printf("%s:%s() " fmt "\n", __FILE__, __FUNCTION__ , ## arg)
#else
#define DBG(fmt...)
#endif

#ifndef DBUS_TYPE_G_OBJECT_PATH_ARRAY
#define DBUS_TYPE_G_OBJECT_PATH_ARRAY \
	(dbus_g_type_get_collection("GPtrArray", DBUS_TYPE_G_OBJECT_PATH))
#endif

#ifndef DBUS_TYPE_G_DICTIONARY
#define DBUS_TYPE_G_DICTIONARY \
	(dbus_g_type_get_map("GHashTable", G_TYPE_STRING, G_TYPE_VALUE))
#endif

static DBusGConnection *connection = NULL;

typedef gboolean (*IterSearchFunc) (GtkTreeStore *store,
				GtkTreeIter *iter, gconstpointer user_data);

static gboolean iter_search(GtkTreeStore *store,
				GtkTreeIter *iter, GtkTreeIter *parent,
				IterSearchFunc func, gconstpointer user_data)
{
	gboolean cont, found = FALSE;

	if (parent == NULL)
		cont = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store),
									iter);
	else
		cont = gtk_tree_model_iter_children(GTK_TREE_MODEL(store),
								iter, parent);

	while (cont == TRUE) {
		GtkTreeIter child;

		found = func(store, iter, user_data);
		if (found == TRUE)
			break;

		found = iter_search(store, &child, iter, func, user_data);
		if (found == TRUE) {
			*iter = child;
			break;
		}

		cont = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), iter);
	}

	return found;
}

static gboolean compare_proxy(GtkTreeStore *store, GtkTreeIter *iter,
						gconstpointer user_data)
{
	const char *path = user_data;
	DBusGProxy *proxy;
	gboolean found = FALSE;

	gtk_tree_model_get(GTK_TREE_MODEL(store), iter,
					CONNMAN_COLUMN_PROXY, &proxy, -1);

	if (proxy != NULL) {
		found = g_str_equal(path, dbus_g_proxy_get_path(proxy));
		g_object_unref(proxy);
	}

	return found;
}

static gboolean get_iter_from_proxy(GtkTreeStore *store,
					GtkTreeIter *iter, DBusGProxy *proxy)
{
	const char *path;

	if (proxy == NULL)
		return FALSE;

	path = dbus_g_proxy_get_path(proxy);
	if (path == NULL)
		return FALSE;

	return iter_search(store, iter, NULL, compare_proxy, path);
}

static gboolean get_iter_from_path(GtkTreeStore *store,
					GtkTreeIter *iter, const char *path)
{
	if (path == NULL)
		return FALSE;

	return iter_search(store, iter, NULL, compare_proxy, path);
}

DBusGProxy *connman_dbus_get_proxy(GtkTreeStore *store, const gchar *path)
{
	DBusGProxy *proxy;
	GtkTreeIter iter;

	if (iter_search(store, &iter, NULL, compare_proxy, path) == FALSE)
		return NULL;

	gtk_tree_model_get(GTK_TREE_MODEL(store), &iter,
					CONNMAN_COLUMN_PROXY, &proxy, -1);

	return proxy;
}

gboolean connman_dbus_get_iter(GtkTreeStore *store, const gchar *path,
							GtkTreeIter *iter)
{
	return get_iter_from_path(store, iter, path);
}

static void iterate_list(const GValue *value, gpointer user_data)
{
	GSList **list = user_data;
	gchar *path = g_value_dup_boxed(value);

	if (path == NULL)
		return;

	*list = g_slist_prepend(*list, path);
}

static gint compare_path(gconstpointer a, gconstpointer b)
{
	return g_strcmp0(a, b);
}

static void property_update(GtkTreeStore *store, const GValue *value,
			const char *key, connman_get_properties_reply callback)
{
	GSList *list, *link, *old_list, *new_list = NULL;
	const char *iface;

	DBG("store %p", store);

	if (g_str_equal(key, "Connections") == TRUE)
		iface = CONNMAN_CONNECTION_INTERFACE;
	else if (g_str_equal(key, "Devices") == TRUE)
		iface = CONNMAN_DEVICE_INTERFACE;
	else
		iface = CONNMAN_NETWORK_INTERFACE;

	old_list = g_object_get_data(G_OBJECT(store), key);

	dbus_g_type_collection_value_iterate(value, iterate_list, &new_list);

	g_object_set_data(G_OBJECT(store), key, new_list);

	for (list = new_list; list; list = list->next) {
		gchar *path = list->data;
		DBusGProxy *proxy;

		link = g_slist_find_custom(old_list, path, compare_path);
		if (link != NULL) {
			g_free(link->data);
			old_list = g_slist_delete_link(old_list, link);
			continue;
		}

		proxy = dbus_g_proxy_new_for_name(connection,
						CONNMAN_SERVICE, path, iface);
		if (proxy == NULL)
			continue;

		connman_get_properties_async(proxy, callback, store);
	}

	for (list = old_list; list; list = list->next) {
		gchar *path = list->data;
		GtkTreeIter iter;

		if (get_iter_from_path(store, &iter, path) == TRUE)
			gtk_tree_store_remove(store, &iter);

		g_free(path);
	}

	g_slist_free(old_list);
}

static guint get_type(const GValue *value)
{
	const char *type = value ? g_value_get_string(value) : NULL;

	if (type == NULL)
		return CONNMAN_TYPE_UNKNOWN;
	else if (g_str_equal(type, "ethernet") == TRUE)
		return CONNMAN_TYPE_ETHERNET;
	else if (g_str_equal(type, "wifi") == TRUE)
		return CONNMAN_TYPE_WIFI;
	else if (g_str_equal(type, "wimax") == TRUE)
		return CONNMAN_TYPE_WIMAX;
	else if (g_str_equal(type, "bluetooth") == TRUE)
		return CONNMAN_TYPE_BLUETOOTH;

	return CONNMAN_TYPE_UNKNOWN;
}

static const gchar *type2icon(guint type)
{
	switch (type) {
	case CONNMAN_TYPE_ETHERNET:
		return "network-wired";
	case CONNMAN_TYPE_WIFI:
	case CONNMAN_TYPE_WIMAX:
		return "network-wireless";
	case CONNMAN_TYPE_BLUETOOTH:
		return "bluetooth";
	}

	return NULL;
}

static guint get_policy(const GValue *value)
{
	const char *policy = value ? g_value_get_string(value) : NULL;

	if (policy == NULL)
		return CONNMAN_POLICY_UNKNOWN;
	else if (g_str_equal(policy, "ignore") == TRUE)
		return CONNMAN_POLICY_IGNORE;
	else if (g_str_equal(policy, "off") == TRUE)
		return CONNMAN_POLICY_OFF;
	else if (g_str_equal(policy, "auto") == TRUE)
		return CONNMAN_POLICY_AUTO;
	else if (g_str_equal(policy, "manual") == TRUE)
		return CONNMAN_POLICY_MANUAL;

	return CONNMAN_POLICY_UNKNOWN;
}

static guint get_security(const GValue *value)
{
	const char *security = value ? g_value_get_string(value) : NULL;

	if (security == NULL)
		return CONNMAN_SECURITY_UNKNOWN;
	else if (g_str_equal(security, "none") == TRUE)
		return CONNMAN_SECURITY_NONE;
	else if (g_str_equal(security, "wep") == TRUE)
		return CONNMAN_SECURITY_WEP;
	else if (g_str_equal(security, "wpa") == TRUE)
		return CONNMAN_SECURITY_WPA;
	else if (g_str_equal(security, "wpa2") == TRUE)
		return CONNMAN_SECURITY_WPA2;

	return CONNMAN_SECURITY_UNKNOWN;
}

static void network_changed(DBusGProxy *proxy, const char *property,
					GValue *value, gpointer user_data)
{
	GtkTreeStore *store = user_data;
	const char *path = dbus_g_proxy_get_path(proxy);
	GtkTreeIter iter;

	DBG("store %p proxy %p property %s", store, proxy, property);

	if (property == NULL || value == NULL)
		return;

	if (get_iter_from_path(store, &iter, path) == FALSE)
		return;

	if (g_str_equal(property, "Connected") == TRUE) {
		gboolean connected = g_value_get_boolean(value);
		gtk_tree_store_set(store, &iter,
					CONNMAN_COLUMN_ENABLED, connected, -1);
	} else if (g_str_equal(property, "Available") == TRUE) {
		gboolean inrange = g_value_get_boolean(value);
		gtk_tree_store_set(store, &iter,
					CONNMAN_COLUMN_INRANGE, inrange, -1);
	} else if (g_str_equal(property, "Remember") == TRUE) {
		gboolean remember = g_value_get_boolean(value);
		gtk_tree_store_set(store, &iter,
					CONNMAN_COLUMN_REMEMBER, remember, -1);
	} else if (g_str_equal(property, "Strength") == TRUE) {
		guint strength = g_value_get_uchar(value);
		gtk_tree_store_set(store, &iter,
					CONNMAN_COLUMN_STRENGTH, strength, -1);
	}
}

static void network_properties(DBusGProxy *proxy, GHashTable *hash,
					GError *error, gpointer user_data)
{
	GtkTreeStore *store = user_data;
	GValue *value;
	const gchar *device, *name, *secret;
	gboolean connected, inrange, remember;
	guint strength, security;
	GtkTreeIter iter, parent;

	DBG("store %p proxy %p hash %p", store, proxy, hash);

	if (error != NULL || hash == NULL)
		goto done;

	value = g_hash_table_lookup(hash, "Device");
	device = value ? g_value_get_boxed(value) : NULL;

	value = g_hash_table_lookup(hash, "Name");
	name = value ? g_value_get_string(value) : NULL;

	value = g_hash_table_lookup(hash, "Connected");
	connected = value ? g_value_get_boolean(value) : FALSE;

	value = g_hash_table_lookup(hash, "Available");
	inrange = value ? g_value_get_boolean(value) : FALSE;

	value = g_hash_table_lookup(hash, "Remember");
	remember = value ? g_value_get_boolean(value) : FALSE;

	value = g_hash_table_lookup(hash, "Strength");
	strength = value ? g_value_get_uchar(value) : 0;

	value = g_hash_table_lookup(hash, "WiFi.Security");
	security = get_security(value);

	value = g_hash_table_lookup(hash, "WiFi.Passphrase");
	secret = value ? g_value_get_string(value) : NULL;

	DBG("name %s strength %d", name, strength);

	if (get_iter_from_path(store, &parent, device) == FALSE)
		return;

	if (get_iter_from_proxy(store, &iter, proxy) == FALSE) {
		gtk_tree_store_insert_with_values(store, &iter, &parent, -1,
					CONNMAN_COLUMN_PROXY, proxy,
					CONNMAN_COLUMN_NAME, name,
					CONNMAN_COLUMN_ENABLED, connected,
					CONNMAN_COLUMN_INRANGE, inrange,
					CONNMAN_COLUMN_REMEMBER, remember,
					CONNMAN_COLUMN_STRENGTH, strength,
					CONNMAN_COLUMN_SECURITY, security,
					CONNMAN_COLUMN_PASSPHRASE, secret, -1);

		dbus_g_proxy_add_signal(proxy, "PropertyChanged",
				G_TYPE_STRING, G_TYPE_VALUE, G_TYPE_INVALID);
		dbus_g_proxy_connect_signal(proxy, "PropertyChanged",
				G_CALLBACK(network_changed), store, NULL);
	} else
		gtk_tree_store_set(store, &iter,
					CONNMAN_COLUMN_NAME, name,
					CONNMAN_COLUMN_ENABLED, connected,
					CONNMAN_COLUMN_INRANGE, inrange,
					CONNMAN_COLUMN_REMEMBER, remember,
					CONNMAN_COLUMN_STRENGTH, strength,
					CONNMAN_COLUMN_SECURITY, security,
					CONNMAN_COLUMN_PASSPHRASE, secret, -1);

done:
	g_object_unref(proxy);
}

static void device_changed(DBusGProxy *proxy, const char *property,
					GValue *value, gpointer user_data)
{
	GtkTreeStore *store = user_data;
	const char *path = dbus_g_proxy_get_path(proxy);
	GtkTreeIter iter;

	DBG("store %p proxy %p property %s", store, proxy, property);

	if (property == NULL || value == NULL)
		return;

	if (get_iter_from_path(store, &iter, path) == FALSE)
		return;

	if (g_str_equal(property, "Powered") == TRUE) {
		guint policy = get_policy(value);
		gtk_tree_store_set(store, &iter,
					CONNMAN_COLUMN_POLICY, policy, -1);
	} else if (g_str_equal(property, "Powered") == TRUE) {
		gboolean powered = g_value_get_boolean(value);
		gtk_tree_store_set(store, &iter,
					CONNMAN_COLUMN_ENABLED, powered, -1);
	} else if (g_str_equal(property, "Networks") == TRUE)
		property_update(store, value, path, network_properties);
}

static void device_properties(DBusGProxy *proxy, GHashTable *hash,
					GError *error, gpointer user_data)
{
	GtkTreeStore *store = user_data;
	const char *path = dbus_g_proxy_get_path(proxy);
	GValue *value;
	const gchar *name, *icon;
	guint type, policy;
	gboolean powered;
	GtkTreeIter iter;

	DBG("store %p proxy %p hash %p", store, proxy, hash);

	if (error != NULL || hash == NULL)
		goto done;

	value = g_hash_table_lookup(hash, "Name");
	name = value ? g_value_get_string(value) : NULL;

	value = g_hash_table_lookup(hash, "Type");
	type = get_type(value);
	icon = type2icon(type);

	value = g_hash_table_lookup(hash, "Policy");
	policy = get_policy(value);

	value = g_hash_table_lookup(hash, "Powered");
	powered = value ? g_value_get_boolean(value) : FALSE;

	DBG("name %s type %d icon %s", name, type, icon);

	if (get_iter_from_proxy(store, &iter, proxy) == FALSE) {
		gtk_tree_store_insert_with_values(store, &iter, NULL, -1,
					CONNMAN_COLUMN_PROXY, proxy,
					CONNMAN_COLUMN_NAME, name,
					CONNMAN_COLUMN_ICON, icon,
					CONNMAN_COLUMN_TYPE, type,
					CONNMAN_COLUMN_ENABLED, powered,
					CONNMAN_COLUMN_POLICY, policy, -1);

		dbus_g_proxy_add_signal(proxy, "PropertyChanged",
				G_TYPE_STRING, G_TYPE_VALUE, G_TYPE_INVALID);
		dbus_g_proxy_connect_signal(proxy, "PropertyChanged",
				G_CALLBACK(device_changed), store, NULL);
	} else
		gtk_tree_store_set(store, &iter,
					CONNMAN_COLUMN_NAME, name,
					CONNMAN_COLUMN_ICON, icon,
					CONNMAN_COLUMN_TYPE, type,
					CONNMAN_COLUMN_ENABLED, powered,
					CONNMAN_COLUMN_POLICY, policy, -1);

	value = g_hash_table_lookup(hash, "Networks");
	if (value != NULL)
		property_update(store, value, path, network_properties);

done:
	g_object_unref(proxy);
}

static void connection_changed(DBusGProxy *proxy, const char *property,
					GValue *value, gpointer user_data)
{
	GtkTreeStore *store = user_data;
	const char *path = dbus_g_proxy_get_path(proxy);
	GtkTreeIter iter;

	DBG("store %p proxy %p property %s", store, proxy, property);

	if (property == NULL || value == NULL)
		return;

	if (get_iter_from_path(store, &iter, path) == FALSE)
		return;

	if (g_str_equal(property, "Default") == TRUE) {
		gboolean enabled = g_value_get_boolean(value);
		gtk_tree_store_set(store, &iter,
					CONNMAN_COLUMN_ENABLED, enabled, -1);
	} else if (g_str_equal(property, "Strength") == TRUE) {
		guint strength = g_value_get_uchar(value);
		gtk_tree_store_set(store, &iter,
					CONNMAN_COLUMN_STRENGTH, strength, -1);
	}
}

static void connection_properties(DBusGProxy *proxy, GHashTable *hash,
					GError *error, gpointer user_data)
{
	GtkTreeStore *store = user_data;
	GValue *value;
	guint type, strength;
	gboolean enabled;
	const char *device, *address;
	GtkTreeIter iter;

	if (error != NULL || hash == NULL)
		goto done;

	value = g_hash_table_lookup(hash, "Type");
	type = get_type(value);

	value = g_hash_table_lookup(hash, "Strength");
	strength = value ? g_value_get_uchar(value) : 0;

	value = g_hash_table_lookup(hash, "Default");
	enabled = value ? g_value_get_boolean(value) : FALSE;

	value = g_hash_table_lookup(hash, "IPv4.Address");
	address = value ? g_value_get_string(value) : NULL;

	DBG("type %d address %s", type, address);

	if (get_iter_from_proxy(store, &iter, proxy) == FALSE) {
		gtk_tree_store_insert_with_values(store, &iter, NULL, -1,
					CONNMAN_COLUMN_PROXY, proxy,
					CONNMAN_COLUMN_TYPE, type,
					CONNMAN_COLUMN_ENABLED, enabled,
					CONNMAN_COLUMN_STRENGTH, strength,
					CONNMAN_COLUMN_ADDRESS, address, -1);

		dbus_g_proxy_add_signal(proxy, "PropertyChanged",
				G_TYPE_STRING, G_TYPE_VALUE, G_TYPE_INVALID);
		dbus_g_proxy_connect_signal(proxy, "PropertyChanged",
				G_CALLBACK(connection_changed), store, NULL);
	} else
		gtk_tree_store_set(store, &iter,
					CONNMAN_COLUMN_TYPE, type,
					CONNMAN_COLUMN_ENABLED, enabled,
					CONNMAN_COLUMN_STRENGTH, strength, -1);

	value = g_hash_table_lookup(hash, "Device");
	device = value ? g_value_get_boxed(value) : NULL;

	DBG("device %s", device);

	if (get_iter_from_path(store, &iter, device) == TRUE) {
		gtk_tree_store_set(store, &iter,
					CONNMAN_COLUMN_INRANGE, TRUE,
					CONNMAN_COLUMN_ADDRESS, address, -1);
	}

done:
	g_object_unref(proxy);
}

static void manager_changed(DBusGProxy *proxy, const char *property,
					GValue *value, gpointer user_data)
{
	GtkTreeStore *store = user_data;

	DBG("store %p proxy %p property %s", store, proxy, property);

	if (property == NULL || value == NULL)
		return;

	if (g_str_equal(property, "State") == TRUE) {
		ConnmanClientCallback callback;
		gchar *state;

		state = g_object_get_data(G_OBJECT(store), "State");
		g_free(state);

		state = g_value_dup_string(value);
		g_object_set_data(G_OBJECT(store), "State", state);

		callback = g_object_get_data(G_OBJECT(store), "callback");
		if (callback)
			callback(state, NULL);
	} else if (g_str_equal(property, "Connections") == TRUE) {
		property_update(store, value, property, connection_properties);
	} else if (g_str_equal(property, "Devices") == TRUE) {
		property_update(store, value, property, device_properties);
	}
}

static void manager_properties(DBusGProxy *proxy, GHashTable *hash,
					GError *error, gpointer user_data)
{
	GtkTreeStore *store = user_data;
	ConnmanClientCallback callback;
	GValue *value;
	gchar *state;

	DBG("store %p proxy %p hash %p", store, proxy, hash);

	if (error != NULL || hash == NULL)
		return;

	value = g_hash_table_lookup(hash, "State");
	state = value ? g_value_dup_string(value) : NULL;
	g_object_set_data(G_OBJECT(store), "State", state);

	callback = g_object_get_data(G_OBJECT(store), "callback");
	if (callback)
		callback(state, NULL);

	value = g_hash_table_lookup(hash, "Devices");
	if (value != NULL)
		property_update(store, value, "Devices", device_properties);

	value = g_hash_table_lookup(hash, "Connections");
	if (value != NULL)
		property_update(store, value,
					"Connections", connection_properties);
}

DBusGProxy *connman_dbus_create_manager(DBusGConnection *conn,
						GtkTreeStore *store)
{
	DBusGProxy *proxy;

	connection = dbus_g_connection_ref(conn);

	proxy = dbus_g_proxy_new_for_name(connection, CONNMAN_SERVICE,
			CONNMAN_MANAGER_PATH, CONNMAN_MANAGER_INTERFACE);

	DBG("store %p proxy %p", store, proxy);

	dbus_g_proxy_add_signal(proxy, "PropertyChanged",
				G_TYPE_STRING, G_TYPE_VALUE, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(proxy, "PropertyChanged",
				G_CALLBACK(manager_changed), store, NULL);

	connman_get_properties_async(proxy, manager_properties, store);

	return proxy;
}

void connman_dbus_destroy_manager(DBusGProxy *proxy, GtkTreeStore *store)
{
	DBG("store %p proxy %p", store, proxy);

	g_signal_handlers_disconnect_by_func(proxy, manager_changed, store);
	g_object_unref(proxy);

	dbus_g_connection_unref(connection);
}
