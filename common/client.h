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

enum {
	CLIENT_COLUMN_ACTIVE,
	CLIENT_COLUMN_PROXY,
	CLIENT_COLUMN_TYPE,
	CLIENT_COLUMN_DRIVER,
	CLIENT_COLUMN_VENDOR,
	CLIENT_COLUMN_PRODUCT,
	CLIENT_COLUMN_STATE,
	CLIENT_COLUMN_SIGNAL,
	CLIENT_COLUMN_POLICY,
	CLIENT_COLUMN_IPV4_METHOD,
	CLIENT_COLUMN_IPV4_ADDRESS,
	CLIENT_COLUMN_IPV4_NETMASK,
	CLIENT_COLUMN_IPV4_GATEWAY,
};

enum {
	CLIENT_TYPE_UNKNOWN,
	CLIENT_TYPE_80203,
	CLIENT_TYPE_80211,
};

enum {
	CLIENT_STATE_UNKNOWN,
	CLIENT_STATE_OFFLINE,
	CLIENT_STATE_READY,
};

enum {
	CLIENT_POLICY_UNKNOWN,
	CLIENT_POLICY_OFF,
	CLIENT_POLICY_IGNORE,
	CLIENT_POLICY_AUTO,
};

enum {
	CLIENT_IPV4_METHOD_UNKNOWN,
	CLIENT_IPV4_METHOD_OFF,
	CLIENT_IPV4_METHOD_STATIC,
	CLIENT_IPV4_METHOD_DHCP,
};

gboolean client_init(GError **error);
void client_cleanup(void);

GtkTreeModel *client_get_model(void);

void client_set_policy(const gchar *index, guint policy);
void client_set_ipv4(const gchar *index, guint method);
