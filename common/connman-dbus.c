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

	*list = g_slist_append(*list, path);
}

static gint compare_path(gconstpointer a, gconstpointer b)
{
	return g_strcmp0(a, b);
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
	else if (g_str_equal(type, "cellular") == TRUE)
		return CONNMAN_TYPE_CELLULAR;

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

static guint get_state(const GValue *value)
{
	const char *state = value ? g_value_get_string(value) : NULL;

	if (state == NULL)
		return CONNMAN_STATE_UNKNOWN;
	else if (g_str_equal(state, "idle") == TRUE)
		return CONNMAN_STATE_IDLE;
	else if (g_str_equal(state, "carrier") == TRUE)
		return CONNMAN_STATE_CARRIER;
	else if (g_str_equal(state, "association") == TRUE)
		return CONNMAN_STATE_ASSOCIATION;
	else if (g_str_equal(state, "configuration") == TRUE)
		return CONNMAN_STATE_CONFIGURATION;
	else if (g_str_equal(state, "ready") == TRUE)
		return CONNMAN_STATE_READY;
	else if (g_str_equal(state, "failure") == TRUE)
		return CONNMAN_STATE_FAILURE;

	return CONNMAN_STATE_UNKNOWN;
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
	else if (g_str_equal(security, "rsn") == TRUE)
		return CONNMAN_SECURITY_RSN;

	return CONNMAN_SECURITY_UNKNOWN;
}

static void service_changed(DBusGProxy *proxy, const char *property,
					GValue *value, gpointer user_data)
{
	GtkTreeStore *store = user_data;
	const char *path = dbus_g_proxy_get_path(proxy);
	GtkTreeIter iter;
	GHashTable *ipv4;
	const char *method, *addr, *netmask, *gateway;
	GValue *ipv4_method, *ipv4_address, *ipv4_netmask, *ipv4_gateway;


	DBG("store %p proxy %p property %s", store, proxy, property);

	if (property == NULL || value == NULL)
		return;

	if (get_iter_from_path(store, &iter, path) == FALSE)
		return;

	if (g_str_equal(property, "IPv4") == TRUE) {
		ipv4 = g_value_get_boxed (value);
		if (!ipv4)
			return;

		ipv4_method = g_hash_table_lookup (ipv4, "Method");
		method = ipv4_method ? g_value_get_string(ipv4_method) : NULL;

		ipv4_address = g_hash_table_lookup (ipv4, "Address");
		addr = ipv4_address ? g_value_get_string(ipv4_address) : NULL;

		ipv4_netmask = g_hash_table_lookup (ipv4, "Netmask");
		netmask = ipv4_netmask ? g_value_get_string(ipv4_netmask) : NULL;

		ipv4_gateway = g_hash_table_lookup (ipv4, "Gateway");
		gateway = ipv4_gateway ? g_value_get_string(ipv4_gateway) : NULL;

		gtk_tree_store_set(store, &iter,
					CONNMAN_COLUMN_METHOD, method,
					CONNMAN_COLUMN_ADDRESS, addr,
					CONNMAN_COLUMN_NETMASK, netmask,
					CONNMAN_COLUMN_GATEWAY, gateway,
					-1);
	} else if (g_str_equal(property, "State") == TRUE) {
		guint state = get_state(value);
		gtk_tree_store_set(store, &iter,
					CONNMAN_COLUMN_STATE, state, -1);
	} else if (g_str_equal(property, "Favorite") == TRUE) {
		gboolean favorite = g_value_get_boolean(value);
		gtk_tree_store_set(store, &iter,
					CONNMAN_COLUMN_FAVORITE, favorite, -1);
	} else if (g_str_equal(property, "Strength") == TRUE) {
		guint strength = g_value_get_uchar(value);
		gtk_tree_store_set(store, &iter,
					CONNMAN_COLUMN_STRENGTH, strength, -1);
	}
}

static void property_update(GtkTreeStore *store, const GValue *value,
					connman_get_properties_reply callback)
{
	GSList *list, *link, *old_list, *new_list = NULL;
	guint index = 0;

	DBG("store %p", store);

	old_list = g_object_get_data(G_OBJECT(store), "Services");

	dbus_g_type_collection_value_iterate(value, iterate_list, &new_list);

	g_object_set_data(G_OBJECT(store), "Services", new_list);

	for (list = new_list; list; list = list->next) {
		gchar *path = list->data;
		DBusGProxy *proxy;
		GtkTreeIter iter;

		DBG("new path %s", path);

		link = g_slist_find_custom(old_list, path, compare_path);
		if (link != NULL) {
			g_free(link->data);
			old_list = g_slist_delete_link(old_list, link);
		}

		proxy = dbus_g_proxy_new_for_name(connection,
						CONNMAN_SERVICE, path,
						CONNMAN_SERVICE_INTERFACE);
		if (proxy == NULL)
			continue;

		if (get_iter_from_proxy(store, &iter, proxy) == FALSE) {
			gtk_tree_store_insert_with_values(store, &iter, NULL, -1,
					CONNMAN_COLUMN_PROXY, proxy,
					CONNMAN_COLUMN_INDEX, index, -1);

			dbus_g_proxy_add_signal(proxy, "PropertyChanged",
				G_TYPE_STRING, G_TYPE_VALUE, G_TYPE_INVALID);
			dbus_g_proxy_connect_signal(proxy, "PropertyChanged",
				G_CALLBACK(service_changed), store, NULL);
		} else
			gtk_tree_store_set(store, &iter,
					CONNMAN_COLUMN_INDEX, index, -1);

		DBG("getting %s properties", "Services");

		connman_get_properties_async(proxy, callback, store);

		index++;
	}

	for (list = old_list; list; list = list->next) {
		gchar *path = list->data;
		GtkTreeIter iter;

		DBG("old path %s", path);

		if (get_iter_from_path(store, &iter, path) == TRUE)
			gtk_tree_store_remove(store, &iter);

		g_free(path);
	}

	g_slist_free(old_list);
}

static void service_properties(DBusGProxy *proxy, GHashTable *hash,
					GError *error, gpointer user_data)
{
	GtkTreeStore *store = user_data;
	GValue *value;
	const gchar *name, *icon;
	guint type, state, strength, security;
	gboolean favorite;
	GtkTreeIter iter;

	GHashTable *ipv4;
	GValue *ipv4_method, *ipv4_address, *ipv4_netmask, *ipv4_gateway;
	const char *method, *addr, *netmask, *gateway;

	DBG("store %p proxy %p hash %p", store, proxy, hash);

	if (error != NULL || hash == NULL)
		goto done;

	value = g_hash_table_lookup(hash, "Name");
	name = value ? g_value_get_string(value) : NULL;

	value = g_hash_table_lookup(hash, "Type");
	type = get_type(value);
	icon = type2icon(type);

	value = g_hash_table_lookup(hash, "State");
	state = get_state(value);

	value = g_hash_table_lookup(hash, "Favorite");
	favorite = value ? g_value_get_boolean(value) : FALSE;

	value = g_hash_table_lookup(hash, "Strength");
	strength= value ? g_value_get_uchar(value) : 0;

	value = g_hash_table_lookup(hash, "Security");
	security = get_security(value);

	DBG("name %s type %d icon %s", name, type, icon);

	value = g_hash_table_lookup(hash, "IPv4.Configuration");
	ipv4 = g_value_get_boxed (value);

	if (!ipv4)
		goto done;

	ipv4_method = g_hash_table_lookup (ipv4, "Method");
	method = ipv4_method ? g_value_get_string(ipv4_method) : NULL;

	ipv4_address = g_hash_table_lookup (ipv4, "Address");
	addr = ipv4_address ? g_value_get_string(ipv4_address) : NULL;

	ipv4_netmask = g_hash_table_lookup (ipv4, "Netmask");
	netmask = ipv4_netmask ? g_value_get_string(ipv4_netmask) : NULL;

	ipv4_gateway = g_hash_table_lookup (ipv4, "Gateway");
	gateway = ipv4_gateway ? g_value_get_string(ipv4_gateway) : NULL;

	if (get_iter_from_proxy(store, &iter, proxy) == FALSE) {
		gtk_tree_store_insert_with_values(store, &iter, NULL, -1,
					CONNMAN_COLUMN_PROXY, proxy,
					CONNMAN_COLUMN_NAME, name,
					CONNMAN_COLUMN_ICON, icon,
					CONNMAN_COLUMN_TYPE, type,
					CONNMAN_COLUMN_STATE, state,
					CONNMAN_COLUMN_FAVORITE, favorite,
					CONNMAN_COLUMN_STRENGTH, strength,
					CONNMAN_COLUMN_SECURITY, security,
					CONNMAN_COLUMN_METHOD, method,
					CONNMAN_COLUMN_ADDRESS, addr,
					CONNMAN_COLUMN_NETMASK, netmask,
					CONNMAN_COLUMN_GATEWAY, gateway,
					-1);

		dbus_g_proxy_add_signal(proxy, "PropertyChanged",
				G_TYPE_STRING, G_TYPE_VALUE, G_TYPE_INVALID);
		dbus_g_proxy_connect_signal(proxy, "PropertyChanged",
				G_CALLBACK(service_changed), store, NULL);
	} else
		gtk_tree_store_set(store, &iter,
					CONNMAN_COLUMN_NAME, name,
					CONNMAN_COLUMN_ICON, icon,
					CONNMAN_COLUMN_TYPE, type,
					CONNMAN_COLUMN_STATE, state,
					CONNMAN_COLUMN_FAVORITE, favorite,
					CONNMAN_COLUMN_STRENGTH, strength,
					CONNMAN_COLUMN_SECURITY, security,
					CONNMAN_COLUMN_METHOD, method,
					CONNMAN_COLUMN_ADDRESS, addr,
					CONNMAN_COLUMN_NETMASK, netmask,
					CONNMAN_COLUMN_GATEWAY, gateway,
					-1);

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

	if (g_str_equal(property, "Services") == TRUE)
		property_update(store, value, service_properties);
}

static void manager_properties(DBusGProxy *proxy, GHashTable *hash,
					GError *error, gpointer user_data)
{
	GtkTreeStore *store = user_data;
	GValue *value;

	DBG("store %p proxy %p hash %p", store, proxy, hash);

	if (error != NULL || hash == NULL)
		return;

	value = g_hash_table_lookup(hash, "Services");
	if (value != NULL)
		property_update(store, value, service_properties);
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

	DBG("getting manager properties");

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
