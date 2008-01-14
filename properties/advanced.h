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
	GtkWidget *widget;
	GtkWidget *title;
	GtkWidget *label;
	GtkWidget *button;

	GtkWidget *window;
	GtkTreeModel *model;
	gchar *index;

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
};

void create_advanced_dialog(struct config_data *data, guint type);

void add_80203_policy(GtkWidget *mainbox, struct config_data *data);
void update_80203_policy(struct config_data *data, guint policy);

gboolean separator_function(GtkTreeModel *model,
					GtkTreeIter *iter, gpointer user_data);
