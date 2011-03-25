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
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "connman-client.h"

#include "advanced.h"

static void update_wifi_ipv4(struct config_data *data, guint policy);

static void changed_callback(GtkWidget *editable, gpointer user_data)
{
	struct config_data *data = user_data;
	gint active;

	active = gtk_combo_box_get_active(GTK_COMBO_BOX(data->policy.config));

	update_wifi_ipv4(data, active);
}

static void connect_reply_cb(DBusGProxy *proxy, GError *error,
		gpointer user_data)
{
	if (error)
		g_error_free(error);
}

static void connect_callback(GtkWidget *editable, gpointer user_data)
{
	struct config_data *data = user_data;

	gboolean ret;
	gint active;

	if (data->wifi.passphrase) {
		char *passphrase;
		passphrase = (gchar *)gtk_entry_get_text(GTK_ENTRY(data->wifi.passphrase));
		ret = connman_client_set_passphrase(data->client, data->device,
				passphrase);

		if (ret == FALSE) {
			return;
		}
	}

	active = gtk_combo_box_get_active(GTK_COMBO_BOX(data->policy.config));
	data->ipv4_config.method = active ? "manual" : "dhcp";
	data->ipv4_config.address = active ? gtk_entry_get_text(GTK_ENTRY(data->ipv4.entry[0])) : NULL;
	data->ipv4_config.netmask = active ? gtk_entry_get_text(GTK_ENTRY(data->ipv4.entry[1])) : NULL;
	data->ipv4_config.gateway = active ? gtk_entry_get_text(GTK_ENTRY(data->ipv4.entry[2])) : NULL;

	ret = connman_client_set_ipv4(data->client, data->device, &data->ipv4_config);
	if (ret == FALSE) {
		return;
	}

	connman_client_connect_async(data->client, data->device, connect_reply_cb, data);
}


static void disconnect_callback(GtkWidget *editable, gpointer user_data)
{
	struct config_data *data = user_data;

	connman_client_disconnect(data->client, data->device);
}

static void switch_callback(GtkWidget *editable, gpointer user_data)
{
	struct config_data *data = user_data;
	const gchar *label = gtk_button_get_label(GTK_BUTTON(data->wifi_button));

	if (g_str_equal(label, "Disable"))
		connman_client_disable_technology(data->client, data->device, "wifi");
	else
		connman_client_enable_technology(data->client, data->device, "wifi");
}

static void scan_reply_cb(DBusGProxy *proxy, GError *error,
				   gpointer user_data)
{
	GtkWidget *button = user_data;
	gtk_widget_set_sensitive(button, 1);

	if (error)
		g_error_free(error);
}

static void scan_callback(GtkWidget *button, gpointer user_data)
{
	struct config_data *data = user_data;
	gtk_widget_set_sensitive(button, 0);
	connman_client_request_scan(data->client, "", scan_reply_cb, button);
}

void add_wifi_switch_button(GtkWidget *mainbox, GtkTreeIter *iter,
				struct config_data *data)
{
	GtkWidget *vbox;
	GtkWidget *table;
	GtkWidget *label;
	GtkWidget *buttonbox;
	GtkWidget *button;
	gboolean wifi_enabled;

	gtk_tree_model_get(data->model, iter,
			CONNMAN_COLUMN_WIFI_ENABLED, &wifi_enabled,
			-1);

	vbox = gtk_vbox_new(TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 24);
	gtk_box_pack_start(GTK_BOX(mainbox), vbox, FALSE, FALSE, 0);

	table = gtk_table_new(1, 1, TRUE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 10);
	gtk_table_set_col_spacings(GTK_TABLE(table), 10);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);

	label = gtk_label_new(_("Configure Wifi Networks."));
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);

	buttonbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(buttonbox), GTK_BUTTONBOX_CENTER);
	gtk_box_pack_start(GTK_BOX(mainbox), buttonbox, FALSE, FALSE, 0);

	button = gtk_button_new();
	data->wifi_button = button;

	if (wifi_enabled)
		gtk_button_set_label(GTK_BUTTON(button), _("Disable"));
	else
		gtk_button_set_label(GTK_BUTTON(button), _("Enable"));

	gtk_container_add(GTK_CONTAINER(buttonbox), button);
	g_signal_connect(G_OBJECT(button), "clicked",
			G_CALLBACK(switch_callback), data);

	button = gtk_button_new_with_label(_("Scan"));
	data->scan_button = button;

	if (!wifi_enabled)
		gtk_widget_set_sensitive(button, 0);

	gtk_container_add(GTK_CONTAINER(buttonbox), button);
	g_signal_connect(G_OBJECT(button), "clicked",
			G_CALLBACK(scan_callback), data);
}

static void wifi_ipconfig(GtkWidget *table, struct config_data *data, GtkTreeIter *iter)
{
	GtkWidget *entry;
	GtkWidget *label;
	GtkWidget *combo;
	DBusGProxy *proxy;

	struct ipv4_config ipv4_config = {
		.method  = NULL,
		.address = NULL,
		.netmask = NULL,
		.gateway = NULL,
	};

	gtk_tree_model_get(data->model, iter,
			CONNMAN_COLUMN_PROXY, &proxy,
			CONNMAN_COLUMN_METHOD, &ipv4_config.method,
			CONNMAN_COLUMN_ADDRESS, &ipv4_config.address,
			CONNMAN_COLUMN_NETMASK, &ipv4_config.netmask,
			CONNMAN_COLUMN_GATEWAY, &ipv4_config.gateway,
			-1);
	label = gtk_label_new(_("Configuration:"));
	gtk_table_attach_defaults(GTK_TABLE(table), label, 1, 2, 3, 4);
	data->ipv4.label[0] = label;

	combo = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "DHCP");
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "Manual");

	gtk_combo_box_set_row_separator_func(GTK_COMBO_BOX(combo),
			separator_function, NULL, NULL);
	gtk_table_attach_defaults(GTK_TABLE(table), combo, 2, 4, 3, 4);
	data->policy.config = combo;

	label = gtk_label_new(_("IP address:"));
	gtk_table_attach_defaults(GTK_TABLE(table), label, 1, 2, 4, 5);
	data->ipv4.label[0] = label;

	entry = gtk_entry_new();
	gtk_entry_set_max_length (GTK_ENTRY (entry), 15);
	if (ipv4_config.address)
		gtk_entry_set_text(GTK_ENTRY(entry), ipv4_config.address);
	gtk_table_attach_defaults(GTK_TABLE(table), entry, 2, 4, 4, 5);
	data->ipv4.entry[0] = entry;


	label = gtk_label_new(_("Netmask:"));
	gtk_table_attach_defaults(GTK_TABLE(table), label, 1, 2, 5, 6);
	data->ipv4.label[1] = label;

	entry = gtk_entry_new();
	gtk_entry_set_max_length (GTK_ENTRY (entry), 15);
	if (ipv4_config.netmask)
		gtk_entry_set_text(GTK_ENTRY(entry), ipv4_config.netmask);
	gtk_table_attach_defaults(GTK_TABLE(table), entry, 2, 4, 5, 6);
	data->ipv4.entry[1] = entry;

	label = gtk_label_new(_("Gateway:"));
	gtk_table_attach_defaults(GTK_TABLE(table), label, 1, 2, 6, 7);
	data->ipv4.label[2] = label;

	entry = gtk_entry_new();
	gtk_entry_set_max_length (GTK_ENTRY (entry), 15);
	if (ipv4_config.gateway)
		gtk_entry_set_text(GTK_ENTRY(entry), ipv4_config.gateway);
	gtk_table_attach_defaults(GTK_TABLE(table), entry, 2, 4, 6, 7);
	data->ipv4.entry[2] = entry;

	data->ipv4_config = ipv4_config;

	if (g_str_equal(ipv4_config.method, "dhcp") == TRUE)
		update_wifi_ipv4(data, CONNMAN_POLICY_DHCP);
	else
		update_wifi_ipv4(data, CONNMAN_POLICY_MANUAL);

	g_signal_connect(G_OBJECT(combo), "changed",
			G_CALLBACK(changed_callback), data);
}

static void toggled_callback(GtkWidget *button, gpointer user_data)
{
	GtkWidget *entry = user_data;
	gboolean mode;

	mode = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));

	gtk_entry_set_visibility(GTK_ENTRY(entry), mode);
}

void add_wifi_service(GtkWidget *mainbox, GtkTreeIter *iter, struct config_data *data)
{
	GtkWidget *vbox;
	GtkWidget *table;
	GtkWidget *label;
	GtkWidget *entry;
	GtkWidget *button;

	const char *name, *security, *icon, *state;
	guint strength;

	gtk_tree_model_get(data->model, iter,
			CONNMAN_COLUMN_NAME, &name,
			CONNMAN_COLUMN_SECURITY, &security,
			CONNMAN_COLUMN_ICON, &icon,
			CONNMAN_COLUMN_STATE, &state,
			CONNMAN_COLUMN_STRENGTH, &strength,
			-1);

	if (g_str_equal(state, "failure") == TRUE)
		connman_client_remove(data->client, data->device);

	vbox = gtk_vbox_new(TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 24);
	gtk_box_pack_start(GTK_BOX(mainbox), vbox, FALSE, FALSE, 0);

	table = gtk_table_new(10, 5, TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);
	data->table = table;

	label = gtk_label_new(_("Access Point:"));
	gtk_table_attach_defaults(GTK_TABLE(table), label, 1, 2, 0, 1);

	label = gtk_label_new(_(name));
	gtk_table_attach_defaults(GTK_TABLE(table), label, 2, 4, 0, 1);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	data->wifi.name = label;

	label = gtk_label_new(_("Security:"));
	gtk_table_attach_defaults(GTK_TABLE(table), label, 1, 2, 1, 2);

	label = gtk_label_new(_(security));
	gtk_table_attach_defaults(GTK_TABLE(table), label, 2, 4, 1, 2);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	data->wifi.security = label;

	label = gtk_label_new(_("Passphrase:"));
	gtk_table_attach_defaults(GTK_TABLE(table), label, 1, 2, 2, 3);

	if (g_str_equal(security, "none") != TRUE &&
			g_str_equal(security, "unknown") != TRUE) {
		entry = gtk_entry_new();
		gtk_entry_set_max_length (GTK_ENTRY (entry), 64);
		gtk_table_attach_defaults(GTK_TABLE(table), entry, 2, 4, 2, 3);
		gtk_entry_set_visibility(GTK_ENTRY(entry), 0);
		data->wifi.passphrase = entry;

		button = gtk_check_button_new_with_label(_("Show input"));
		gtk_table_attach_defaults(GTK_TABLE(table), button, 4, 5, 2, 3);

		g_signal_connect(G_OBJECT(button), "toggled",
				G_CALLBACK(toggled_callback), entry);


	} else {
		label = gtk_label_new(_("none"));
		gtk_table_attach_defaults(GTK_TABLE(table), label, 2, 4, 2, 3);
		gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
		gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
		data->wifi.passphrase = NULL;
	}

	label = gtk_label_new(_(""));
	gtk_table_attach_defaults(GTK_TABLE(table), label, 2, 3, 8, 9);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
	gtk_widget_hide(label);
	data->wifi.connect_info = label;

	button = gtk_button_new_with_label(_("Connect"));
	gtk_table_attach_defaults(GTK_TABLE(table), button, 3, 4, 8, 9);
	g_signal_connect(G_OBJECT(button), "clicked",
			G_CALLBACK(connect_callback), data);
	gtk_widget_set_no_show_all(button, TRUE);
	data->wifi.connect = button;

	button = gtk_button_new_with_label(_("Disconnect"));
	gtk_table_attach_defaults(GTK_TABLE(table), button, 3, 4, 8, 9);
	g_signal_connect(G_OBJECT(button), "clicked",
			G_CALLBACK(disconnect_callback), data);
	data->wifi.disconnect = button;
	gtk_widget_set_no_show_all(button, TRUE);

	if (g_str_equal(state, "failure") == TRUE
			|| g_str_equal(state, "idle") == TRUE) {
		gtk_widget_show(data->wifi.connect);
		gtk_widget_hide(data->wifi.disconnect);
	} else {
		gtk_widget_hide(data->wifi.connect);
		gtk_widget_show(data->wifi.disconnect);
	}

	wifi_ipconfig(table, data, iter);
}

void update_wifi_ipv4(struct config_data *data, guint policy)
{
	GtkWidget *combo = data->policy.config;
	GtkWidget *entry[3];
	int i;

	for (i = 0; i < 3; i++)
		entry[i] = data->ipv4.entry[i];


	g_signal_handlers_block_by_func(G_OBJECT(combo),
			G_CALLBACK(changed_callback), data);

	switch (policy) {
	case CONNMAN_POLICY_DHCP:
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
		for (i = 0; i < 3; i++) {
			gtk_entry_set_editable(GTK_ENTRY(entry[i]), 0);
			gtk_widget_set_sensitive(entry[i], 0);
		}
		break;
	case CONNMAN_POLICY_MANUAL:
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 1);
		for (i = 0; i < 3; i++) {
			gtk_entry_set_editable(GTK_ENTRY(entry[i]), 1);
			gtk_widget_set_sensitive(entry[i], 1);
		}
		break;
	default:
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo), -1);
		break;
	}

	g_signal_handlers_unblock_by_func(G_OBJECT(combo),
					G_CALLBACK(changed_callback), data);
}
