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

#include <dbus/dbus-glib.h>

#include <gtk/gtk.h>

#include "common.h"
#include "client.h"

#ifndef DBYS_TYPE_G_OBJECT_PATH_ARRAY
#define DBUS_TYPE_G_OBJECT_PATH_ARRAY \
	(dbus_g_type_get_collection("GPtrArray", DBUS_TYPE_G_OBJECT_PATH))
#endif

static GtkTreeStore *store = NULL;

static client_state_callback state_callback = NULL;
static guint current_state = CLIENT_STATE_UNKNOWN;
static gint current_signal = -1;

static void execute_state_callback(void)
{
	GtkTreeModel *model = GTK_TREE_MODEL(store);
	GtkTreeIter iter;
	gboolean cont;
	guint new_state = CLIENT_STATE_UNKNOWN;
	gint new_signal = -1;

	if (state_callback == NULL)
		return;

	cont = gtk_tree_model_get_iter_first(model, &iter);

	while (cont == TRUE) {
		gboolean active;
		guint type, state, signal;

		gtk_tree_model_get(model, &iter,
					CLIENT_COLUMN_ACTIVE, &active,
					CLIENT_COLUMN_TYPE, &type,
					CLIENT_COLUMN_STATE, &state,
					CLIENT_COLUMN_SIGNAL, &signal, -1);

		if (active == TRUE) {
			switch (new_state) {
			case CLIENT_STATE_CARRIER:
			case CLIENT_STATE_CONNECT:
			case CLIENT_STATE_CONNECTED:
			case CLIENT_STATE_CONFIGURE:
			case CLIENT_STATE_READY:
				break;
			default:
				new_state = state;
				break;
			}

			if (state == CLIENT_STATE_READY) {
				switch (type) {
				case CLIENT_TYPE_80211:
					if ((gint) signal > new_signal)
						new_signal = signal;
					break;
				}
			}
		}

		cont = gtk_tree_model_iter_next(model, &iter);
	}

	if (current_state != new_state || current_signal != new_signal) {
		current_state = new_state;
		current_signal = new_signal;
		state_callback(new_state, new_signal);
	}
}

static guint string_to_type(const char *type)
{
	if (g_ascii_strcasecmp(type, "ethernet") == 0)
		return CLIENT_TYPE_80203;
	else if (g_ascii_strcasecmp(type, "wifi") == 0)
		return CLIENT_TYPE_80211;
	else
		return CLIENT_TYPE_UNKNOWN;
}

static guint string_to_state(const char *state)
{
	if (g_ascii_strcasecmp(state, "off") == 0)
		return CLIENT_STATE_OFF;
	else if (g_ascii_strcasecmp(state, "enabled") == 0)
		return CLIENT_STATE_ENABLED;
	else if (g_ascii_strcasecmp(state, "scanning") == 0)
		return CLIENT_STATE_SCANNING;
	else if (g_ascii_strcasecmp(state, "connect") == 0)
		return CLIENT_STATE_CONNECT;
	else if (g_ascii_strcasecmp(state, "connected") == 0)
		return CLIENT_STATE_CONNECTED;
	else if (g_ascii_strcasecmp(state, "carrier") == 0)
		return CLIENT_STATE_CARRIER;
	else if (g_ascii_strcasecmp(state, "configure") == 0)
		return CLIENT_STATE_CONFIGURE;
	else if (g_ascii_strcasecmp(state, "ready") == 0)
		return CLIENT_STATE_READY;
	else if (g_ascii_strcasecmp(state, "shutdown") == 0)
		return CLIENT_STATE_SHUTDOWN;
	else
		return CLIENT_STATE_UNKNOWN;
}

static guint string_to_policy(const char *policy)
{
	if (g_ascii_strcasecmp(policy, "off") == 0)
		return CLIENT_POLICY_OFF;
	else if (g_ascii_strcasecmp(policy, "ignore") == 0)
		return CLIENT_POLICY_IGNORE;
	else if (g_ascii_strcasecmp(policy, "auto") == 0)
		return CLIENT_POLICY_AUTO;
	else if (g_ascii_strcasecmp(policy, "ask") == 0)
		return CLIENT_POLICY_ASK;
	else
		return CLIENT_POLICY_UNKNOWN;
}

static const char *policy_to_string(guint policy)
{
	switch (policy) {
	case CLIENT_POLICY_OFF:
		return "off";
	case CLIENT_POLICY_IGNORE:
		return "ignore";
	case CLIENT_POLICY_AUTO:
		return "auto";
	case CLIENT_POLICY_ASK:
		return "ask";
	default:
		return "unknown";
	}
}

static guint string_to_ipv4_method(const char *method)
{
	if (g_ascii_strcasecmp(method, "off") == 0)
		return CLIENT_IPV4_METHOD_OFF;
	else if (g_ascii_strcasecmp(method, "static") == 0)
		return CLIENT_IPV4_METHOD_STATIC;
	else if (g_ascii_strcasecmp(method, "dhcp") == 0)
		return CLIENT_IPV4_METHOD_DHCP;
	else
		return CLIENT_IPV4_METHOD_UNKNOWN;
}

static const char *ipv4_method_to_string(guint method)
{
	switch (method) {
	case CLIENT_IPV4_METHOD_OFF:
		return "off";
	case CLIENT_IPV4_METHOD_STATIC:
		return "static";
	case CLIENT_IPV4_METHOD_DHCP:
		return "dhcp";
	default:
		return "unknown";
	}
}

static void handle_ipv4(GHashTable *hash, GtkTreeIter *iter)
{
	GValue *value;
	const char *str;

	value = g_hash_table_lookup(hash, "Method");
	if (value != NULL) {
		str = g_value_get_string(value);
		gtk_tree_store_set(store, iter, CLIENT_COLUMN_IPV4_METHOD,
					string_to_ipv4_method(str), -1);
	}

	value = g_hash_table_lookup(hash, "Address");
	if (value != NULL) {
		str = g_value_get_string(value);
		gtk_tree_store_set(store, iter,
				CLIENT_COLUMN_IPV4_ADDRESS, str, -1);
	}

	value = g_hash_table_lookup(hash, "Netmask");
	if (value != NULL) {
		str = g_value_get_string(value);
		gtk_tree_store_set(store, iter,
				CLIENT_COLUMN_IPV4_NETMASK, str, -1);
	}

	value = g_hash_table_lookup(hash, "Gateway");
	if (value != NULL) {
		str = g_value_get_string(value);
		gtk_tree_store_set(store, iter,
				CLIENT_COLUMN_IPV4_GATEWAY, str, -1);
	}
}

static void ipv4_notify(DBusGProxy *object,
				DBusGProxyCall *call, void *user_data)
{
	GtkTreeModel *model = GTK_TREE_MODEL(store);
	GtkTreeIter iter;
	GError *error = NULL;
	GHashTable *hash = NULL;

	dbus_g_proxy_end_call(object, call, &error,
				dbus_g_type_get_map("GHashTable",
						G_TYPE_STRING, G_TYPE_VALUE),
				&hash, G_TYPE_INVALID);

	if (error != NULL) {
		g_printerr("Getting interface address failed: %s\n",
							error->message);
		g_error_free(error);
		return;
	}

	if (hash == NULL)
		return;

	if (gtk_tree_model_get_iter_from_string(model, &iter,
							user_data) == FALSE)
		return;

	handle_ipv4(hash, &iter);
}

static void append_network(GHashTable *hash, GtkTreeIter *parent)
{
	GtkTreeModel *model = GTK_TREE_MODEL(store);
	GtkTreeIter iter;
	GValue *value;
	const char *str;
	gchar *essid;
	guint signal;
	gboolean security, cont;

	value = g_hash_table_lookup(hash, "ESSID");
	if (value == NULL)
		return;

	str = g_value_get_string(value);
	if (str == NULL)
		return;

	value = g_hash_table_lookup(hash, "Signal");
	signal = (value == NULL) ? 0 : g_value_get_uint(value);

	value = g_hash_table_lookup(hash, "Security");
	security = (value == NULL) ? FALSE : TRUE;

	cont = gtk_tree_model_iter_children(model, &iter, parent);

	while (cont == TRUE) {
		gtk_tree_model_get(model, &iter,
				CLIENT_COLUMN_NETWORK_ESSID, &essid, -1);

		if (strcmp(str, essid) == 0) {
			gtk_tree_store_set(store, &iter,
					CLIENT_COLUMN_SIGNAL, signal, -1);
			g_free(essid);
			return;
		}

		g_free(essid);

		cont = gtk_tree_model_iter_next(model, &iter);
	}

	gtk_tree_store_insert_with_values(store, &iter, parent, -1,
				CLIENT_COLUMN_ACTIVE, TRUE,
				CLIENT_COLUMN_SIGNAL, signal,
				CLIENT_COLUMN_NETWORK_ESSID, str,
				CLIENT_COLUMN_NETWORK_SECURITY, security, -1);
}

static void handle_network(GHashTable *hash,
				GtkTreeIter *iter, gboolean secrets)
{
	GValue *value;
	const char *str;

	value = g_hash_table_lookup(hash, "ESSID");
	if (value != NULL) {
		str = g_value_get_string(value);
		gtk_tree_store_set(store, iter,
					CLIENT_COLUMN_NETWORK_ESSID, str, -1);
	}

	value = g_hash_table_lookup(hash, "PSK");
	if (value != NULL) {
		str = g_value_get_string(value);
		gtk_tree_store_set(store, iter,
					CLIENT_COLUMN_NETWORK_PSK, str, -1);
	} else if (secrets == TRUE)
		gtk_tree_store_set(store, iter,
					CLIENT_COLUMN_NETWORK_PSK, NULL, -1);
		
}

static void network_notify(DBusGProxy *object,
				DBusGProxyCall *call, void *user_data)
{
	GtkTreeModel *model = GTK_TREE_MODEL(store);
	GtkTreeIter iter;
	GError *error = NULL;
	GHashTable *hash = NULL;

	dbus_g_proxy_end_call(object, call, &error,
				dbus_g_type_get_map("GHashTable",
						G_TYPE_STRING, G_TYPE_VALUE),
				&hash, G_TYPE_INVALID);

	if (error != NULL) {
		g_printerr("Getting interface network failed: %s\n",
							error->message);
		g_error_free(error);
		return;
	}

	if (hash == NULL)
		return;

	if (gtk_tree_model_get_iter_from_string(model, &iter,
							user_data) == FALSE)
		return;

	handle_network(hash, &iter, TRUE);
}

static void properties_notify(DBusGProxy *object,
				DBusGProxyCall *call, void *user_data)
{
	GtkTreeModel *model = GTK_TREE_MODEL(store);
	GtkTreeIter iter;
	GError *error = NULL;
	GHashTable *hash = NULL;
	GValue *value;
	const char *str;
	guint type = CLIENT_TYPE_UNKNOWN;
	guint state = CLIENT_STATE_UNKNOWN;
	guint signal = 0;
	guint policy = CLIENT_POLICY_UNKNOWN;
	const gchar *driver, *vendor, *product;

	dbus_g_proxy_end_call(object, call, &error,
				dbus_g_type_get_map("GHashTable",
						G_TYPE_STRING, G_TYPE_VALUE),
				&hash, G_TYPE_INVALID);

	if (error != NULL) {
		g_printerr("Getting interface properties failed: %s\n",
							error->message);
		g_error_free(error);
		return;
	}

	if (hash == NULL)
		return;

	if (gtk_tree_model_get_iter_from_string(model, &iter,
							user_data) == FALSE)
		return;

	value = g_hash_table_lookup(hash, "Type");
	if (value == NULL)
		return;

	str = g_value_get_string(value);
	if (str == NULL || g_str_equal(str, "device") == FALSE)
		return;

	value = g_hash_table_lookup(hash, "Subtype");
	if (value != NULL) {
		str = g_value_get_string(value);
		type = string_to_type(str);
	}

	value = g_hash_table_lookup(hash, "Driver");
	driver = value ? g_value_get_string(value) : NULL;

	value = g_hash_table_lookup(hash, "Vendor");
	vendor = value ? g_value_get_string(value) : NULL;

	value = g_hash_table_lookup(hash, "Product");
	product = value ? g_value_get_string(value) : NULL;

	value = g_hash_table_lookup(hash, "State");
	if (value != NULL) {
		str = g_value_get_string(value);
		state = string_to_state(str);
	}

	value = g_hash_table_lookup(hash, "Signal");
	if (value != NULL)
		signal = g_value_get_uint(value);

	value = g_hash_table_lookup(hash, "Policy");
	if (value != NULL) {
		str = g_value_get_string(value);
		policy = string_to_policy(str);
	}

	gtk_tree_store_set(store, &iter, CLIENT_COLUMN_TYPE, type,
					CLIENT_COLUMN_DRIVER, driver,
					CLIENT_COLUMN_VENDOR, vendor,
					CLIENT_COLUMN_PRODUCT, product,
					CLIENT_COLUMN_STATE, state,
					CLIENT_COLUMN_SIGNAL, signal,
					CLIENT_COLUMN_POLICY, policy, -1);

	execute_state_callback();
}

static void update_element(DBusGProxy *proxy,
				GtkTreeModel *model, GtkTreeIter *iter)
{
	DBusGProxyCall *call;
	gchar *index;

	index = gtk_tree_model_get_string_from_iter(model, iter);

	call = dbus_g_proxy_begin_call_with_timeout(proxy,
					"GetProperties", properties_notify,
					index, g_free, 2000, G_TYPE_INVALID);

	index = gtk_tree_model_get_string_from_iter(model, iter);

	call = dbus_g_proxy_begin_call_with_timeout(proxy,
					"GetNetwork", network_notify,
					index, g_free, 2000, G_TYPE_INVALID);

	index = gtk_tree_model_get_string_from_iter(model, iter);

	call = dbus_g_proxy_begin_call_with_timeout(proxy,
					"GetIPv4", ipv4_notify,
					index, g_free, 2000, G_TYPE_INVALID);
}

static void state_changed(DBusGProxy *proxy, const char *state,
							gpointer user_data)
{
	GtkTreeModel *model = GTK_TREE_MODEL(store);
	GtkTreeIter iter;

	if (gtk_tree_model_get_iter_from_string(model, &iter,
							user_data) == FALSE)
		return;

	gtk_tree_store_set(store, &iter, CLIENT_COLUMN_STATE,
						string_to_state(state), -1);

	execute_state_callback();
}

static void signal_changed(DBusGProxy *proxy, unsigned int signal,
							gpointer user_data)
{
	GtkTreeModel *model = GTK_TREE_MODEL(store);
	GtkTreeIter iter;

	if (gtk_tree_model_get_iter_from_string(model, &iter,
							user_data) == FALSE)
		return;

	gtk_tree_store_set(store, &iter, CLIENT_COLUMN_SIGNAL, signal, -1);

	execute_state_callback();
}

static void policy_changed(DBusGProxy *proxy, const char *policy,
							gpointer user_data)
{
	GtkTreeModel *model = GTK_TREE_MODEL(store);
	GtkTreeIter iter;

	if (gtk_tree_model_get_iter_from_string(model, &iter,
							user_data) == FALSE)
		return;

	gtk_tree_store_set(store, &iter, CLIENT_COLUMN_POLICY,
						string_to_policy(policy), -1);
}

static void network_found(DBusGProxy *proxy, GHashTable *hash,
							gpointer user_data)
{
	GtkTreeModel *model = GTK_TREE_MODEL(store);
	GtkTreeIter iter;

	if (gtk_tree_model_get_iter_from_string(model, &iter,
							user_data) == FALSE)
		return;

	append_network(hash, &iter);
}

static void network_changed(DBusGProxy *proxy, GHashTable *hash,
							gpointer user_data)
{
	GtkTreeModel *model = GTK_TREE_MODEL(store);
	GtkTreeIter iter;
	DBusGProxyCall *call;
	gchar *index;

	if (gtk_tree_model_get_iter_from_string(model, &iter,
							user_data) == FALSE)
		return;

	handle_network(hash, &iter, FALSE);

	index = gtk_tree_model_get_string_from_iter(model, &iter);

	call = dbus_g_proxy_begin_call_with_timeout(proxy,
					"GetNetwork", network_notify,
					index, g_free, 2000, G_TYPE_INVALID);
}

static void ipv4_changed(DBusGProxy *system, GHashTable *hash,
							gpointer user_data)
{
	GtkTreeModel *model = GTK_TREE_MODEL(store);
	GtkTreeIter iter;

	if (gtk_tree_model_get_iter_from_string(model, &iter,
							user_data) == FALSE)
		return;

	handle_ipv4(hash, &iter);
}

static void signal_closure(gpointer user_data, GClosure *closure)
{
	g_free(user_data);
}

static void add_element(DBusGProxy *manager, const char *path)
{
	GtkTreeModel *model = GTK_TREE_MODEL(store);
	GtkTreeIter iter;
	DBusGProxy *proxy;
	gchar *index;
	gboolean cont;

	cont = gtk_tree_model_get_iter_first(model, &iter);

	while (cont == TRUE) {
		const char *object_path;

		gtk_tree_model_get(model, &iter,
					CLIENT_COLUMN_PROXY, &proxy, -1);
		if (proxy == NULL)
			continue;

		object_path = dbus_g_proxy_get_path(proxy);

		g_object_unref(proxy);

		if (strcmp(object_path, path) == 0) {
			gtk_tree_store_set(store, &iter,
					CLIENT_COLUMN_ACTIVE, TRUE,
					CLIENT_COLUMN_STATE,
						CLIENT_STATE_UNKNOWN, -1);
			update_element(proxy, model, &iter);
			return;
		}

		cont = gtk_tree_model_iter_next(model, &iter);
	}

	proxy = dbus_g_proxy_new_from_proxy(manager, CONNMAN_ELEMENT, path);

	gtk_tree_store_insert_with_values(store, &iter, NULL, -1,
					CLIENT_COLUMN_ACTIVE, TRUE,
					CLIENT_COLUMN_PROXY, proxy,
					CLIENT_COLUMN_STATE,
						CLIENT_STATE_UNKNOWN, -1);

	index = gtk_tree_model_get_string_from_iter(model, &iter);

	dbus_g_proxy_add_signal(proxy, "StateChanged",
					G_TYPE_STRING, G_TYPE_INVALID);

	dbus_g_proxy_connect_signal(proxy, "StateChanged",
			G_CALLBACK(state_changed), index, signal_closure);

	index = gtk_tree_model_get_string_from_iter(model, &iter);

	dbus_g_proxy_add_signal(proxy, "SignalChanged",
					G_TYPE_UINT, G_TYPE_INVALID);

	dbus_g_proxy_connect_signal(proxy, "SignalChanged",
			G_CALLBACK(signal_changed), index, signal_closure);

	index = gtk_tree_model_get_string_from_iter(model, &iter);

	dbus_g_proxy_add_signal(proxy, "PolicyChanged",
					G_TYPE_STRING, G_TYPE_INVALID);

	dbus_g_proxy_connect_signal(proxy, "PolicyChanged",
			G_CALLBACK(policy_changed), index, signal_closure);

	index = gtk_tree_model_get_string_from_iter(model, &iter);

	dbus_g_proxy_add_signal(proxy, "NetworkFound",
				dbus_g_type_get_map("GHashTable",
					G_TYPE_STRING, G_TYPE_VALUE),
							G_TYPE_INVALID);

	dbus_g_proxy_connect_signal(proxy, "NetworkFound",
			G_CALLBACK(network_found), index, signal_closure);

	index = gtk_tree_model_get_string_from_iter(model, &iter);

	dbus_g_proxy_add_signal(proxy, "NetworkChanged",
				dbus_g_type_get_map("GHashTable",
					G_TYPE_STRING, G_TYPE_VALUE),
							G_TYPE_INVALID);

	dbus_g_proxy_connect_signal(proxy, "NetworkChanged",
			G_CALLBACK(network_changed), index, signal_closure);

	index = gtk_tree_model_get_string_from_iter(model, &iter);

	dbus_g_proxy_add_signal(proxy, "IPv4Changed",
				dbus_g_type_get_map("GHashTable",
					G_TYPE_STRING, G_TYPE_VALUE),
							G_TYPE_INVALID);

	dbus_g_proxy_connect_signal(proxy, "IPv4Changed",
			G_CALLBACK(ipv4_changed), index, signal_closure);

	update_element(proxy, model, &iter);
}

static void element_added(DBusGProxy *manager, const char *path,
							gpointer user_data)
{
	add_element(manager, path);

	execute_state_callback();
}

static gboolean disable_element(GtkTreeModel *model, GtkTreePath *path,
					GtkTreeIter *iter, gpointer user_data)
{
	DBusGProxy *proxy;
	const char *object_path;

	if (user_data == NULL) {
		gtk_tree_store_set(GTK_TREE_STORE(model), iter,
					CLIENT_COLUMN_ACTIVE, FALSE, -1);
		return FALSE;
	}

	gtk_tree_model_get(model, iter, CLIENT_COLUMN_PROXY, &proxy, -1);
	if (proxy == NULL)
		return FALSE;

	object_path = dbus_g_proxy_get_path(proxy);

	g_object_unref(proxy);

	if (strcmp(object_path, user_data) == 0) {
		gtk_tree_store_set(GTK_TREE_STORE(model), iter,
					CLIENT_COLUMN_ACTIVE, FALSE, -1);
		return TRUE;
	}

	return FALSE;
}

static void element_removed(DBusGProxy *manager, const char *path,
							gpointer user_data)
{
	gtk_tree_model_foreach(GTK_TREE_MODEL(store),
					disable_element, (gpointer) path);

	execute_state_callback();
}

static DBusGProxy *create_manager(DBusGConnection *conn)
{
	DBusGProxy *manager;
	GError *error = NULL;
	GPtrArray *array = NULL;

	manager = dbus_g_proxy_new_for_name(conn, CONNMAN_SERVICE,
							"/", CONNMAN_MANAGER);

	dbus_g_proxy_call(manager, "ListElements", &error, G_TYPE_INVALID,
			DBUS_TYPE_G_OBJECT_PATH_ARRAY, &array, G_TYPE_INVALID);

	if (error == NULL) {
		int i;

		for (i = 0; i < array->len; i++) {
			gchar *path = g_ptr_array_index(array, i);
			add_element(manager, path);
			g_free(path);
		}
	} else
		g_error_free(error);

	dbus_g_proxy_add_signal(manager, "ElementAdded",
				DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);

	dbus_g_proxy_connect_signal(manager, "ElementAdded",
				G_CALLBACK(element_added), NULL, NULL);

	dbus_g_proxy_add_signal(manager, "ElementRemoved",
				DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);

	dbus_g_proxy_connect_signal(manager, "ElementRemoved",
				G_CALLBACK(element_removed), NULL, NULL);

	return manager;
}

static void name_owner_changed(DBusGProxy *system, const char *name,
			const char *prev, const char *new, gpointer user_data)
{
	if (strcmp(name, CONNMAN_SERVICE) == 0 && *new == '\0')
		gtk_tree_model_foreach(GTK_TREE_MODEL(store),
						disable_element, NULL);

	execute_state_callback();
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

static DBusGConnection *connection;
static DBusGProxy *system, *manager;

gboolean client_init(GError **error)
{
	GError *dbus_error = NULL;

	connection = dbus_g_bus_get(DBUS_BUS_SYSTEM, &dbus_error);
	if (dbus_error != NULL) {
		g_error_free(dbus_error);
		return FALSE;
	}

	store = gtk_tree_store_new(17, G_TYPE_BOOLEAN, DBUS_TYPE_G_PROXY,
				G_TYPE_POINTER, G_TYPE_UINT, G_TYPE_STRING,
				G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT,
				G_TYPE_UINT, G_TYPE_UINT, G_TYPE_STRING,
				G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_UINT,
				G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

	system = create_system(connection);

	manager = create_manager(connection);

	return TRUE;
}

void client_cleanup(void)
{
	g_object_unref(manager);

	g_object_unref(system);

	g_object_unref(store);

	dbus_g_connection_unref(connection);
}

void client_set_userdata(const gchar *index, gpointer user_data)
{
	GtkTreeModel *model = GTK_TREE_MODEL(store);
	GtkTreeIter iter;

	if (gtk_tree_model_get_iter_from_string(model, &iter, index) == FALSE)
		return;

	gtk_tree_store_set(store, &iter, CLIENT_COLUMN_USERDATA,
							user_data, -1);
}

void client_set_state_callback(client_state_callback callback)
{
	state_callback = callback;

	execute_state_callback();
}

void client_propose_scanning(void)
{
	GtkTreeModel *model = GTK_TREE_MODEL(store);
	GtkTreeIter iter;
	gboolean cont;

	cont = gtk_tree_model_get_iter_first(model, &iter);

	while (cont == TRUE) {
		DBusGProxy *proxy;
		gboolean active;

		gtk_tree_model_get(model, &iter,
					CLIENT_COLUMN_ACTIVE, &active,
					CLIENT_COLUMN_PROXY, &proxy, -1);
		if (proxy == NULL || active == FALSE)
			continue;

		dbus_g_proxy_call(proxy, "Scan", NULL,
					G_TYPE_INVALID, G_TYPE_INVALID);

		g_object_unref(proxy);

		cont = gtk_tree_model_iter_next(model, &iter);
	}
}

GtkTreeModel *client_get_model(void)
{
	return gtk_tree_model_filter_new(GTK_TREE_MODEL(store), NULL);
}

GtkTreeModel *client_get_active_model(void)
{
	GtkTreeModel *model;

	model = gtk_tree_model_filter_new(GTK_TREE_MODEL(store), NULL);
	gtk_tree_model_filter_set_visible_column(GTK_TREE_MODEL_FILTER(model),
							CLIENT_COLUMN_ACTIVE);

	return model;
}

void client_set_policy(const gchar *index, guint policy)
{
	GtkTreeModel *model = GTK_TREE_MODEL(store);
	GtkTreeIter iter;
	DBusGProxy *proxy;

	if (gtk_tree_model_get_iter_from_string(model, &iter, index) == FALSE)
		return;

	gtk_tree_model_get(model, &iter, CLIENT_COLUMN_PROXY, &proxy, -1);
	if (proxy == NULL)
		return;

	dbus_g_proxy_call_no_reply(proxy, "SetPolicy", G_TYPE_STRING,
				policy_to_string(policy), G_TYPE_INVALID);

	g_object_unref(proxy);
}

static void value_free(GValue *value)
{
	g_value_unset(value);
	g_free(value);
}

void client_set_ipv4(const gchar *index, guint method)
{
	GtkTreeModel *model = GTK_TREE_MODEL(store);
	GtkTreeIter iter;
	DBusGProxy *proxy;
	GHashTable *hash = NULL;
	GValue *value;

	if (gtk_tree_model_get_iter_from_string(model, &iter, index) == FALSE)
		return;

	gtk_tree_model_get(model, &iter, CLIENT_COLUMN_PROXY, &proxy, -1);
	if (proxy == NULL)
		return;

	hash = g_hash_table_new_full(g_str_hash, g_str_equal,
					g_free, (GDestroyNotify) value_free);

	value = g_new0(GValue, 1);
	g_value_init(value, G_TYPE_STRING);
	g_value_set_string(value, ipv4_method_to_string(method));
	g_hash_table_insert(hash, g_strdup("Method"), value);

	dbus_g_proxy_call_no_reply(proxy, "SetIPv4",
					dbus_g_type_get_map("GHashTable",
						G_TYPE_STRING, G_TYPE_VALUE),
					hash, G_TYPE_INVALID);

	g_object_unref(proxy);
}

void client_set_network(const gchar *index, const gchar *network,
						const gchar *passphrase)
{
	GtkTreeModel *model = GTK_TREE_MODEL(store);
	GtkTreeIter iter;
	DBusGProxy *proxy;
	GHashTable *hash = NULL;
	GValue *value;

	if (gtk_tree_model_get_iter_from_string(model, &iter, index) == FALSE)
		return;

	gtk_tree_model_get(model, &iter, CLIENT_COLUMN_PROXY, &proxy, -1);
	if (proxy == NULL)
		return;

	hash = g_hash_table_new_full(g_str_hash, g_str_equal,
					g_free, (GDestroyNotify) value_free);

	value = g_new0(GValue, 1);
	g_value_init(value, G_TYPE_STRING);
	g_value_set_string(value, network);
	g_hash_table_insert(hash, g_strdup("ESSID"), value);

	if (passphrase != NULL) {
		value = g_new0(GValue, 1);
		g_value_init(value, G_TYPE_STRING);
		g_value_set_string(value, passphrase);
		g_hash_table_insert(hash, g_strdup("PSK"), value);
	}

	dbus_g_proxy_call_no_reply(proxy, "SetNetwork",
					dbus_g_type_get_map("GHashTable",
						G_TYPE_STRING, G_TYPE_VALUE),
					hash, G_TYPE_INVALID);

	g_object_unref(proxy);
}
