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

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "connman-client.h"

#include "advanced.h"

static void changed_callback(GtkWidget *editable, gpointer user_data)
{
	struct config_data *data = user_data;
	gint active;

	active = gtk_combo_box_get_active(GTK_COMBO_BOX(data->policy.config));

	switch (active) {
	case 0:
		connman_client_set_policy(data->client, data->device, "auto");
		update_wifi_policy(data, CONNMAN_POLICY_AUTO);
		break;
	case 1:
		connman_client_set_policy(data->client, data->device, "manual");
		update_wifi_policy(data, CONNMAN_POLICY_MANUAL);
		break;
	case 3:
		connman_client_set_policy(data->client, data->device, "off");
		update_wifi_policy(data, CONNMAN_POLICY_OFF);
		break;
	}
}

void add_wifi_policy(GtkWidget *mainbox, struct config_data *data)
{
	GtkWidget *vbox;
	GtkWidget *table;
	GtkWidget *label;
	GtkWidget *combo;

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 24);
	gtk_box_pack_start(GTK_BOX(mainbox), vbox, FALSE, FALSE, 0);

	table = gtk_table_new(2, 4, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 8);
	gtk_table_set_col_spacings(GTK_TABLE(table), 8);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);

#if 0
	label = gtk_label_new(_("Network Name:"));
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
	gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);

	combo = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "Guest");
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "");
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "Off");
	gtk_combo_box_set_row_separator_func(GTK_COMBO_BOX(combo),
					separator_function, NULL, NULL);
	gtk_table_attach_defaults(GTK_TABLE(table), combo, 1, 4, 0, 1);
	data->policy.config = combo;
#endif

	label = gtk_label_new(_("Configuration:"));
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
	gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);

	combo = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "Automatically");
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "Manually");
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "");
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "Off");
	gtk_combo_box_set_row_separator_func(GTK_COMBO_BOX(combo),
					separator_function, NULL, NULL);
	gtk_table_attach_defaults(GTK_TABLE(table), combo, 1, 4, 0, 1);
	data->policy.config = combo;

	label = gtk_label_new(NULL);
	gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 1, 4, 1, 2);
	gtk_widget_set_size_request(label, 180, -1);
	data->policy.label = label;

	g_signal_connect(G_OBJECT(combo), "changed",
				G_CALLBACK(changed_callback), data);
}

void update_wifi_policy(struct config_data *data, guint policy)
{
	GtkWidget *combo = data->policy.config;
	gchar *info = NULL;

	g_signal_handlers_block_by_func(G_OBJECT(combo),
					G_CALLBACK(changed_callback), data);

	switch (policy) {
	case CONNMAN_POLICY_OFF:
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 3);
		break;
	case CONNMAN_POLICY_MANUAL:
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 1);
		break;
	case CONNMAN_POLICY_AUTO:
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
		break;
	default:
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo), -1);
		break;
	}

	g_signal_handlers_unblock_by_func(G_OBJECT(combo),
					G_CALLBACK(changed_callback), data);

	gtk_label_set_markup(GTK_LABEL(data->policy.label), info);

	g_free(info);
}
