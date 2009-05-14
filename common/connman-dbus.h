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

#include <dbus/dbus-glib.h>
#include <gtk/gtk.h>

#define CONNMAN_SERVICE			"org.moblin.connman"

#define CONNMAN_ERROR_INTERFACE		CONNMAN_SERVICE ".Error"
#define CONNMAN_AGENT_INTERFACE		CONNMAN_SERVICE ".Agent"

#define CONNMAN_MANAGER_INTERFACE	CONNMAN_SERVICE ".Manager"
#define CONNMAN_MANAGER_PATH		"/"

#define CONNMAN_PROFILE_INTERFACE	CONNMAN_SERVICE ".Profile"
#define CONNMAN_SERVICE_INTERFACE	CONNMAN_SERVICE ".Service"
#define CONNMAN_DEVICE_INTERFACE	CONNMAN_SERVICE ".Device"
#define CONNMAN_NETWORK_INTERFACE	CONNMAN_SERVICE ".Network"
#define CONNMAN_CONNECTION_INTERFACE	CONNMAN_SERVICE ".Connection"

DBusGProxy *connman_dbus_create_manager(DBusGConnection *connection,
							GtkTreeStore *store);
void connman_dbus_destroy_manager(DBusGProxy *proxy, GtkTreeStore *store);

DBusGProxy *connman_dbus_get_proxy(GtkTreeStore *store, const gchar *path);
gboolean connman_dbus_get_iter(GtkTreeStore *store, const gchar *path,
							GtkTreeIter *iter);
