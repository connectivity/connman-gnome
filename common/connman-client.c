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
#include "marshal.c"

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
	GHashTable *services;
	ConnmanClientCallback callback;
	gpointer userdata;
};

G_DEFINE_TYPE(ConnmanClient, connman_client, G_TYPE_OBJECT)

static void name_owner_changed(DBusGProxy *dbus, const char *name,
			const char *prev, const char *new, gpointer user_data)
{
	ConnmanClient *client = user_data;
	ConnmanClientPrivate *priv = CONNMAN_CLIENT_GET_PRIVATE(client);
	GtkTreeIter iter;
	gboolean cont;
	char *state, *oldstate;

	if (g_str_equal(name, CONNMAN_SERVICE) == FALSE)
		return;

	if (*new != '\0') {
		state = "offline";
		goto done;
	}

	DBG("client %p name %s", client, name);

	cont = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(priv->store),
									&iter);

	while (cont == TRUE)
		cont = gtk_tree_store_remove(priv->store, &iter);

	state = "unavailable";

done:
	oldstate = g_object_get_data(G_OBJECT(priv->store), "State");
	g_free(oldstate);

	g_object_set_data(G_OBJECT(priv->store), "State", g_strdup(state));

	if (priv->callback != NULL)
		priv->callback(state, priv->userdata);
}

static DBusGConnection *connection = NULL;

static gint compare_index(GtkTreeModel *model, GtkTreeIter *iter1,
					GtkTreeIter *iter2, gpointer user_data)
{
	guint index1, index2;

	gtk_tree_model_get(model, iter1, CONNMAN_COLUMN_INDEX, &index1, -1);
	gtk_tree_model_get(model, iter2, CONNMAN_COLUMN_INDEX, &index2, -1);

	return (gint) index2 - (gint) index1;
}

static void connman_client_init(ConnmanClient *client)
{
	ConnmanClientPrivate *priv = CONNMAN_CLIENT_GET_PRIVATE(client);

	DBG("client %p", client);

	priv->store = gtk_tree_store_new(_CONNMAN_NUM_COLUMNS,
				G_TYPE_OBJECT,	/* proxy */
				G_TYPE_UINT,	/* index */
				G_TYPE_STRING,	/* name */
				G_TYPE_STRING,	/* icon */
				G_TYPE_UINT,	/* type */
				G_TYPE_UINT,	/* state */
				G_TYPE_BOOLEAN,	/* favorite */
				G_TYPE_UINT,	/* strength */
				G_TYPE_UINT,	/* security */
				G_TYPE_STRING);	/* passphrase */

	gtk_tree_sortable_set_default_sort_func(GTK_TREE_SORTABLE(priv->store),
						compare_index, NULL, NULL);

	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(priv->store),
				GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
							GTK_SORT_DESCENDING);

	g_object_set_data(G_OBJECT(priv->store),
					"State", g_strdup("unavailable"));

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

GtkTreeModel *connman_client_get_connection_model(ConnmanClient *client)
{
	return connman_client_get_model(client);
}

void connman_client_set_policy(ConnmanClient *client, const gchar *device,
							const gchar *policy)
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

	g_value_init(&value, G_TYPE_STRING);
	g_value_set_string(&value, policy);

	connman_set_property(proxy, "Policy", &value, NULL);

	g_object_unref(proxy);
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

static gboolean device_scan(GtkTreeModel *model, GtkTreePath *path,
					GtkTreeIter *iter, gpointer user_data)
{
	DBusGProxy *proxy;

	gtk_tree_model_get(model, iter, CONNMAN_COLUMN_PROXY, &proxy, -1);

	if (proxy == NULL)
		return FALSE;

	if (g_str_equal(dbus_g_proxy_get_interface(proxy),
					CONNMAN_DEVICE_INTERFACE) == FALSE)
		return FALSE;

	connman_propose_scan(proxy, NULL);

	g_object_unref(proxy);

	return FALSE;
}

void connman_client_propose_scan(ConnmanClient *client, const gchar *device)
{
	ConnmanClientPrivate *priv = CONNMAN_CLIENT_GET_PRIVATE(client);
	DBusGProxy *proxy;

	DBG("client %p", client);

	if (device == NULL) {
		gtk_tree_model_foreach(GTK_TREE_MODEL(priv->store),
							device_scan, NULL);
		return;
	}

	proxy = connman_dbus_get_proxy(priv->store, device);
	if (proxy == NULL)
		return;

	connman_propose_scan(proxy, NULL);

	g_object_unref(proxy);
}

static gboolean network_disconnect(GtkTreeModel *model, GtkTreePath *path,
					GtkTreeIter *iter, gpointer user_data)
{
	DBusGProxy *proxy;

	gtk_tree_model_get(model, iter, CONNMAN_COLUMN_PROXY, &proxy, -1);

	if (proxy == NULL)
		return FALSE;

	if (g_str_equal(dbus_g_proxy_get_interface(proxy),
					CONNMAN_NETWORK_INTERFACE) == FALSE)
		return FALSE;

	connman_disconnect(proxy, NULL);

	g_object_unref(proxy);

	return TRUE;
}

void connman_client_connect(ConnmanClient *client, const gchar *network)
{
	ConnmanClientPrivate *priv = CONNMAN_CLIENT_GET_PRIVATE(client);
	DBusGProxy *proxy;

	DBG("client %p", client);

	if (network == NULL)
		return;

	gtk_tree_model_foreach(GTK_TREE_MODEL(priv->store),
						network_disconnect, NULL);

	proxy = connman_dbus_get_proxy(priv->store, network);
	if (proxy == NULL)
		return;

	connman_connect(proxy, NULL);

	g_object_unref(proxy);
}

static void connman_client_disconnect_all(ConnmanClient *client)
{
	ConnmanClientPrivate *priv = CONNMAN_CLIENT_GET_PRIVATE(client);

	gtk_tree_model_foreach(GTK_TREE_MODEL(priv->store),
						network_disconnect, NULL);
}

void connman_client_disconnect(ConnmanClient *client, const gchar *network)
{
	ConnmanClientPrivate *priv = CONNMAN_CLIENT_GET_PRIVATE(client);
	DBusGProxy *proxy;

	DBG("client %p", client);

	if (network == NULL) {
		connman_client_disconnect_all(client);
		return;
	}

	proxy = connman_dbus_get_proxy(priv->store, network);
	if (proxy == NULL)
		return;

	connman_disconnect(proxy, NULL);

	g_object_unref(proxy);
}

guint connman_client_get_security(ConnmanClient *client, const gchar *network)
{
	ConnmanClientPrivate *priv = CONNMAN_CLIENT_GET_PRIVATE(client);
	GtkTreeIter iter;
	guint security;

	DBG("client %p", client);

	if (network == NULL)
		return CONNMAN_SECURITY_UNKNOWN;

	if (connman_dbus_get_iter(priv->store, network, &iter) == FALSE)
		return CONNMAN_SECURITY_UNKNOWN;

	gtk_tree_model_get(GTK_TREE_MODEL(priv->store), &iter,
				CONNMAN_COLUMN_SECURITY, &security, -1);

	return security;
}

gchar *connman_client_get_passphrase(ConnmanClient *client, const gchar *network)
{
	ConnmanClientPrivate *priv = CONNMAN_CLIENT_GET_PRIVATE(client);
	GtkTreeIter iter;
	gchar *passphrase;

	DBG("client %p", client);

	if (network == NULL)
		return NULL;

	if (connman_dbus_get_iter(priv->store, network, &iter) == FALSE)
		return NULL;

	gtk_tree_model_get(GTK_TREE_MODEL(priv->store), &iter,
				CONNMAN_COLUMN_PASSPHRASE, &passphrase, -1);

	return passphrase;
}

void connman_client_set_passphrase(ConnmanClient *client, const gchar *network,
						const gchar *passphrase)
{
	ConnmanClientPrivate *priv = CONNMAN_CLIENT_GET_PRIVATE(client);
	DBusGProxy *proxy;
	GValue value = { 0 };

	DBG("client %p", client);

	if (network == NULL)
		return;

	proxy = connman_dbus_get_proxy(priv->store, network);
	if (proxy == NULL)
		return;

	g_value_init(&value, G_TYPE_STRING);
	g_value_set_string(&value, passphrase);

	connman_set_property(proxy, "Passphrase", &value, NULL);

	g_value_unset(&value);

	g_object_unref(proxy);
}

void connman_client_set_callback(ConnmanClient *client,
			ConnmanClientCallback callback, gpointer user_data)
{
	ConnmanClientPrivate *priv = CONNMAN_CLIENT_GET_PRIVATE(client);
	gchar *state;

	DBG("client %p", client);

	priv->callback = callback;
	priv->userdata = user_data;

	g_object_set_data(G_OBJECT(priv->store), "callback", callback);
	g_object_set_data(G_OBJECT(priv->store), "userdata", user_data);

	state = g_object_get_data(G_OBJECT(priv->store), "State");
	if (state != NULL && priv->callback != NULL)
		priv->callback(state, priv->userdata);
}
