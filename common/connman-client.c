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

#include "connman-dbus.h"
#include "connman-dbus-glue.h"
#include "connman-client.h"

#include "marshal.h"

#ifdef DEBUG
#define DBG(fmt, arg...) printf("%s:%s() " fmt "\n", __FILE__, __FUNCTION__ , ## arg)
#else
#define DBG(fmt...)
#endif

#define CONNMAN_CLIENT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), \
				CONNMAN_TYPE_CLIENT, ConnmanClientPrivate))

typedef struct _ConnmanClientPrivate ConnmanClientPrivate;

struct _ConnmanClientPrivate {
	GtkTreeStore *store;
	DBusGProxy *dbus;
	DBusGProxy *manager;
	ConnmanClientCallback callback;
};

G_DEFINE_TYPE(ConnmanClient, connman_client, G_TYPE_OBJECT)

static void name_owner_changed(DBusGProxy *dbus, const char *name,
			const char *prev, const char *new, gpointer user_data)
{
	ConnmanClient *client = user_data;
	ConnmanClientPrivate *priv = CONNMAN_CLIENT_GET_PRIVATE(client);
	GtkTreeIter iter;
	gboolean cont;

	if (g_str_equal(name, CONNMAN_SERVICE) == FALSE || *new != '\0')
		return;

	DBG("client %p name %s", client, name);

	cont = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(priv->store),
									&iter);

	while (cont == TRUE)
		cont = gtk_tree_store_remove(priv->store, &iter);
}

static DBusGConnection *connection = NULL;

static void connman_client_init(ConnmanClient *client)
{
	ConnmanClientPrivate *priv = CONNMAN_CLIENT_GET_PRIVATE(client);

	DBG("client %p", client);

	priv->store = gtk_tree_store_new(_CONNMAN_NUM_COLUMNS, G_TYPE_OBJECT,
			G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT,
			G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN,
			G_TYPE_UINT, G_TYPE_UINT, G_TYPE_STRING);

	g_object_set_data(G_OBJECT(priv->store), "State", g_strdup("offline"));

	priv->dbus = dbus_g_proxy_new_for_name(connection, DBUS_SERVICE_DBUS,
				DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS);

	dbus_g_proxy_add_signal(priv->dbus, "NameOwnerChanged",
					G_TYPE_STRING, G_TYPE_STRING,
					G_TYPE_STRING, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(priv->dbus, "NameOwnerChanged",
				G_CALLBACK(name_owner_changed), client, NULL);

	priv->manager = connman_dbus_create_manager(connection, priv->store);
}

static void connman_client_finalize(GObject *client)
{
	ConnmanClientPrivate *priv = CONNMAN_CLIENT_GET_PRIVATE(client);

	DBG("client %p", client);

	g_signal_handlers_disconnect_by_func(priv->dbus,
					name_owner_changed, client);
	g_object_unref(priv->dbus);

	connman_dbus_destroy_manager(priv->manager, priv->store);

	g_object_unref(priv->store);

	G_OBJECT_CLASS(connman_client_parent_class)->finalize(client);
}

static void connman_client_class_init(ConnmanClientClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;
	GError *error = NULL;

	DBG("class %p", klass);

	g_type_class_add_private(klass, sizeof(ConnmanClientPrivate));

	object_class->finalize = connman_client_finalize;

	dbus_g_object_register_marshaller(marshal_VOID__STRING_BOXED,
						G_TYPE_NONE, G_TYPE_STRING,
						G_TYPE_VALUE, G_TYPE_INVALID);

	connection = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);

	if (error != NULL) {
		g_printerr("Connecting to system bus failed: %s\n",
							error->message);
		g_error_free(error);
	}
}

static ConnmanClient *connman_client = NULL;

ConnmanClient *connman_client_new(void)
{
	if (connman_client != NULL)
		return g_object_ref(connman_client);

	connman_client = CONNMAN_CLIENT(g_object_new(CONNMAN_TYPE_CLIENT,
									NULL));

	DBG("client %p", connman_client);

	return connman_client;
}

GtkTreeModel *connman_client_get_model(ConnmanClient *client)
{
	ConnmanClientPrivate *priv = CONNMAN_CLIENT_GET_PRIVATE(client);
	GtkTreeModel *model;

	DBG("client %p", client);

	model = g_object_ref(priv->store);

	return model;
}

static gboolean device_filter(GtkTreeModel *model,
					GtkTreeIter *iter, gpointer user_data)
{
	DBusGProxy *proxy;
	gboolean active;

	gtk_tree_model_get(model, iter, CONNMAN_COLUMN_PROXY, &proxy, -1);

	if (proxy == NULL)
		return FALSE;

	active = g_str_equal(CONNMAN_DEVICE_INTERFACE,
					dbus_g_proxy_get_interface(proxy));

	g_object_unref(proxy);

	return active;
}

GtkTreeModel *connman_client_get_device_model(ConnmanClient *client)
{
	ConnmanClientPrivate *priv = CONNMAN_CLIENT_GET_PRIVATE(client);
	GtkTreeModel *model;

	DBG("client %p", client);

	model = gtk_tree_model_filter_new(GTK_TREE_MODEL(priv->store), NULL);

	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(model),
						device_filter, NULL, NULL);

	return model;
}

static gboolean device_network_filter(GtkTreeModel *model,
					GtkTreeIter *iter, gpointer user_data)
{
	DBusGProxy *proxy;
	gboolean active;

	gtk_tree_model_get(model, iter, CONNMAN_COLUMN_PROXY, &proxy, -1);

	if (proxy == NULL)
		return FALSE;

	active = g_str_equal(CONNMAN_DEVICE_INTERFACE,
					dbus_g_proxy_get_interface(proxy));
	if (active == FALSE)
		active = g_str_equal(CONNMAN_NETWORK_INTERFACE,
					dbus_g_proxy_get_interface(proxy));

	g_object_unref(proxy);

	return active;
}

GtkTreeModel *connman_client_get_device_network_model(ConnmanClient *client)
{
	ConnmanClientPrivate *priv = CONNMAN_CLIENT_GET_PRIVATE(client);
	GtkTreeModel *model;

	DBG("client %p", client);

	model = gtk_tree_model_filter_new(GTK_TREE_MODEL(priv->store), NULL);

	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(model),
					device_network_filter, NULL, NULL);

	return model;
}

static gboolean network_filter(GtkTreeModel *model,
					GtkTreeIter *iter, gpointer user_data)
{
	gchar *device = user_data;
	DBusGProxy *proxy;
	gboolean active;

	gtk_tree_model_get(model, iter, CONNMAN_COLUMN_PROXY, &proxy, -1);

	if (proxy == NULL)
		return FALSE;

	if (g_str_equal(CONNMAN_NETWORK_INTERFACE,
				dbus_g_proxy_get_interface(proxy)) == TRUE)
		active = g_str_has_prefix(dbus_g_proxy_get_interface(proxy),
								device);
	else
		active = FALSE;

	g_object_unref(proxy);

	return active;
}

GtkTreeModel *connman_client_get_network_model(ConnmanClient *client,
							const gchar *device)
{
	ConnmanClientPrivate *priv = CONNMAN_CLIENT_GET_PRIVATE(client);
	GtkTreeModel *model;

	DBG("client %p", client);

	model = gtk_tree_model_filter_new(GTK_TREE_MODEL(priv->store), NULL);

	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(model),
				network_filter, g_strdup(device), g_free);

	return model;
}

static gboolean connection_filter(GtkTreeModel *model,
					GtkTreeIter *iter, gpointer user_data)
{
	DBusGProxy *proxy;
	gboolean active;

	gtk_tree_model_get(model, iter, CONNMAN_COLUMN_PROXY, &proxy, -1);

	if (proxy == NULL)
		return FALSE;

	active = g_str_equal(CONNMAN_CONNECTION_INTERFACE,
					dbus_g_proxy_get_interface(proxy));

	g_object_unref(proxy);

	return active;
}

GtkTreeModel *connman_client_get_connection_model(ConnmanClient *client)
{
	ConnmanClientPrivate *priv = CONNMAN_CLIENT_GET_PRIVATE(client);
	GtkTreeModel *model;

	DBG("client %p", client);

	model = gtk_tree_model_filter_new(GTK_TREE_MODEL(priv->store), NULL);

	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(model),
						connection_filter, NULL, NULL);

	return model;
}

void connman_client_set_powered(ConnmanClient *client, const gchar *device,
							gboolean powered)
{
	ConnmanClientPrivate *priv = CONNMAN_CLIENT_GET_PRIVATE(client);
	DBusGProxy *proxy;
	GValue value = { 0 };

	DBG("client %p", client);

	if (device == NULL)
		return;

	proxy = connman_dbus_get_proxy(priv->store, device);
	if (proxy == NULL)
		return;

	g_value_init(&value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&value, powered);

	connman_set_property(proxy, "Powered", &value, NULL);

	g_object_unref(proxy);
}

void connman_client_propose_scan(ConnmanClient *client, const gchar *device)
{
	ConnmanClientPrivate *priv = CONNMAN_CLIENT_GET_PRIVATE(client);
	DBusGProxy *proxy;

	DBG("client %p", client);

	if (device == NULL)
		return;

	proxy = connman_dbus_get_proxy(priv->store, device);
	if (proxy == NULL)
		return;

	connman_propose_scan(proxy, NULL);

	g_object_unref(proxy);
}

static gboolean network_disconnect(GtkTreeModel *model, GtkTreePath *path,
					GtkTreeIter *iter, gpointer user_data)
{
	ConnmanClient *client = user_data;
	DBusGProxy *proxy;
	gboolean enabled;

	gtk_tree_model_get(model, iter, CONNMAN_COLUMN_PROXY, &proxy,
					CONNMAN_COLUMN_ENABLED, &enabled, -1);

	if (proxy == NULL)
		return FALSE;

	if (g_str_equal(dbus_g_proxy_get_interface(proxy),
					CONNMAN_NETWORK_INTERFACE) == FALSE)
		return FALSE;

	if (enabled == TRUE)
		connman_client_disconnect(client, dbus_g_proxy_get_path(proxy));

	g_object_unref(proxy);

	return enabled;
}

void connman_client_connect(ConnmanClient *client, const gchar *network)
{
	ConnmanClientPrivate *priv = CONNMAN_CLIENT_GET_PRIVATE(client);
	DBusGProxy *proxy;

	DBG("client %p", client);

	if (network == NULL)
		return;

	gtk_tree_model_foreach(GTK_TREE_MODEL(priv->store),
						network_disconnect, client);

	proxy = connman_dbus_get_proxy(priv->store, network);
	if (proxy == NULL)
		return;

	connman_connect(proxy, NULL);

	g_object_unref(proxy);
}

void connman_client_disconnect(ConnmanClient *client, const gchar *network)
{
	ConnmanClientPrivate *priv = CONNMAN_CLIENT_GET_PRIVATE(client);
	DBusGProxy *proxy;

	DBG("client %p", client);

	if (network == NULL)
		return;

	proxy = connman_dbus_get_proxy(priv->store, network);
	if (proxy == NULL)
		return;

	connman_disconnect(proxy, NULL);

	g_object_unref(proxy);
}

void connman_client_set_callback(ConnmanClient *client,
					ConnmanClientCallback callback)
{
	ConnmanClientPrivate *priv = CONNMAN_CLIENT_GET_PRIVATE(client);
	gchar *state;

	DBG("client %p", client);

	priv->callback = callback;

	g_object_set_data(G_OBJECT(priv->store), "callback", callback);

	state = g_object_get_data(G_OBJECT(priv->store), "State");
	if (state != NULL && priv->callback != NULL)
		priv->callback(state, NULL);
}
