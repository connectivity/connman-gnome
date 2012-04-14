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

struct config_data {
	ConnmanClient *client;

	GtkWidget *widget;
	GtkWidget *table;
	GtkWidget *title;
	GtkWidget *label;
	GtkWidget *button;

	GtkWidget *window;
	GtkTreeModel *model;
	gchar *index;
	gchar *device;

	GtkWidget *dialog;

	struct {
		GtkWidget *config;
		GtkWidget *label;
	} policy;

	struct {
		GtkWidget *config;
		GtkWidget *label[3];
		GtkWidget *value[3];
		GtkWidget *entry[3];
	} ipv4;

	struct ipv4_config ipv4_config;

	struct {
		GtkWidget *name;
		GtkWidget *security;
		GtkWidget *strength;
		GtkWidget *connect_info;
		GtkWidget *connect;
		GtkWidget *disconnect;
	} wifi;

	struct {
		GtkWidget *name;
		GtkWidget *strength;
		GtkWidget *connect_info;
		GtkWidget *connect;
		GtkWidget *disconnect;
	} cellular;

	GtkWidget *ethernet_button;
	GtkWidget *wifi_button;
	GtkWidget *scan_button;
	GtkWidget *cellular_button;
	GtkWidget *offline_button;
};

static inline gboolean separator_function(GtkTreeModel *model,
					GtkTreeIter *iter, gpointer user_data)
{
	gchar *text;
	gboolean result = FALSE;

	gtk_tree_model_get(model, iter, 0, &text, -1);

	if (text && *text == '\0')
		result = TRUE;

	g_free(text);

	return result;
}

void add_ethernet_service(GtkWidget *mainbox, GtkTreeIter *iter, struct config_data *data);
void update_ethernet_ipv4(struct config_data *data, guint policy);

void add_wifi_service(GtkWidget *mainbox, GtkTreeIter *iter, struct config_data *data);
void update_wifi_policy(struct config_data *data, guint policy);

void add_cellular_service(GtkWidget *mainbox, GtkTreeIter *iter, struct config_data *data);
void add_ethernet_switch_button(GtkWidget *mainbox, GtkTreeIter *iter,
				struct config_data *data);

void add_wifi_switch_button(GtkWidget *mainbox, GtkTreeIter *iter,
				struct config_data *data);
void add_cellular_switch_button(GtkWidget *mainbox, GtkTreeIter *iter,
				struct config_data *data);
