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

#ifndef __CONNMAN_CLIENT_H
#define __CONNMAN_CLIENT_H

#include <gtk/gtk.h>
#include "connman-dbus-glue.h"

G_BEGIN_DECLS

#define CONNMAN_TYPE_CLIENT (connman_client_get_type())
#define CONNMAN_CLIENT(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), \
					CONNMAN_TYPE_CLIENT, ConnmanClient))
#define CONNMAN_CLIENT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), \
					CONNMAN_TYPE_CLIENT, ConnmanClientClass))
#define CONNMAN_IS_CLIENT(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
							CONNMAN_TYPE_CLIENT))
#define CONNMAN_IS_CLIENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), \
							CONNMAN_TYPE_CLIENT))
#define CONNMAN_GET_CLIENT_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), \
					CONNMAN_TYPE_CLIENT, ConnmanClientClass))

typedef struct _ConnmanClient ConnmanClient;
typedef struct _ConnmanClientClass ConnmanClientClass;

struct _ConnmanClient {
	GObject parent;
};

struct _ConnmanClientClass {
	GObjectClass parent_class;
};

struct ipv4_config {
	const gchar *method;
	const gchar *address;
	const gchar *netmask;
	const gchar *gateway;
};

GType connman_client_get_type(void);

ConnmanClient *connman_client_new(void);

GtkTreeModel *connman_client_get_model(ConnmanClient *client);
GtkTreeModel *connman_client_get_device_model(ConnmanClient *client);
GtkTreeModel *connman_client_get_connection_model(ConnmanClient *client);

void connman_client_set_powered(ConnmanClient *client, const gchar *device,
							gboolean powered);
gboolean connman_client_set_ipv4(ConnmanClient *client, const gchar *device,
				struct ipv4_config *ipv4_config);
void connman_client_propose_scan(ConnmanClient *client, const gchar *device);

void connman_client_connect(ConnmanClient *client, const gchar *network);
void connman_client_disconnect(ConnmanClient *client, const gchar *network);
gchar *connman_client_get_security(ConnmanClient *client, const gchar *network);
void connman_client_connect_async(ConnmanClient *client, const gchar *network,
				connman_connect_reply callback, gpointer userdata);
gchar *connman_client_get_passphrase(ConnmanClient *client, const gchar *network);
gboolean connman_client_set_passphrase(ConnmanClient *client, const gchar *network,
						const gchar *passphrase);
void connman_client_set_remember(ConnmanClient *client, const gchar *network,
							gboolean remember);

typedef void (* ConnmanClientCallback) (const char *status, void *user_data);

void connman_client_set_callback(ConnmanClient *client,
			ConnmanClientCallback callback, gpointer user_data);

void connman_client_remove(ConnmanClient *client, const gchar *network);

enum {
	CONNMAN_COLUMN_PROXY,		/* G_TYPE_OBJECT  */
	CONNMAN_COLUMN_INDEX,		/* G_TYPE_UINT    */
	CONNMAN_COLUMN_NAME,		/* G_TYPE_STRING  */
	CONNMAN_COLUMN_ICON,		/* G_TYPE_STRING  */
	CONNMAN_COLUMN_TYPE,		/* G_TYPE_UINT    */
	CONNMAN_COLUMN_STATE,		/* G_TYPE_STRING  */
	CONNMAN_COLUMN_FAVORITE,	/* G_TYPE_BOOLEAN */
	CONNMAN_COLUMN_STRENGTH,	/* G_TYPE_UINT    */
	CONNMAN_COLUMN_SECURITY,	/* G_TYPE_STRING  */
	CONNMAN_COLUMN_PASSPHRASE,	/* G_TYPE_STRING  */
	CONNMAN_COLUMN_METHOD,		/* G_TYPE_STRING  */
	CONNMAN_COLUMN_ADDRESS,		/* G_TYPE_STRING  */
	CONNMAN_COLUMN_NETMASK,		/* G_TYPE_STRING  */
	CONNMAN_COLUMN_GATEWAY,		/* G_TYPE_STRING  */
	_CONNMAN_NUM_COLUMNS
};

enum {
	CONNMAN_TYPE_UNKNOWN,
	CONNMAN_TYPE_ETHERNET,
	CONNMAN_TYPE_WIFI,
	CONNMAN_TYPE_WIMAX,
	CONNMAN_TYPE_BLUETOOTH,
	CONNMAN_TYPE_CELLULAR,
};

enum {
	CONNMAN_POLICY_DHCP,
	CONNMAN_POLICY_MANUAL,
};

enum {
	CONNMAN_STATE_UNKNOWN,
	CONNMAN_STATE_IDLE,
	CONNMAN_STATE_CARRIER,
	CONNMAN_STATE_ASSOCIATION,
	CONNMAN_STATE_CONFIGURATION,
	CONNMAN_STATE_READY,
	CONNMAN_STATE_FAILURE,
};

enum {
	CONNMAN_SECURITY_UNKNOWN,
	CONNMAN_SECURITY_NONE,
	CONNMAN_SECURITY_WEP,
	CONNMAN_SECURITY_WPA,
	CONNMAN_SECURITY_RSN,
};

G_END_DECLS

#endif /* __CONNMAN_CLIENT_H */
