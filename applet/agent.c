/*
 *
 *  Connection Manager
 *
 *  Agent implementation based on code from bluez-gnome
 *
 *  Copyright (C) 2005-2008  Marcel Holtmann <marcel@holtmann.org>
 *  Copyright (C) 2006-2007  Bastien Nocera <hadess@hadess.net>
 *  Copyright (C) 2012  Intel Corporation
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
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

#include <stdlib.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include <dbus/dbus-glib.h>

#include <connman-agent.h>

#include "agent.h"

struct input_data {
	gboolean numeric;
	gpointer request_data;
	GtkWidget *dialog;
	GHashTable *entries;
};

static struct input_data *input_data_inst = NULL;

static void input_free(struct input_data *input)
{
	gtk_widget_destroy(input->dialog);

	g_hash_table_destroy(input->entries);

	if( input_data_inst == input )
		input_data_inst = NULL;

	g_free(input);
}

static void request_input_callback(GtkWidget *dialog,
				gint response, gpointer user_data)
{
	GHashTableIter iter;
	gpointer key, value;
	GValue *retvalue = NULL;
	const gchar *text;
	struct input_data *input = user_data;

	if (response == GTK_RESPONSE_OK) {
		GHashTable *reply = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
		g_hash_table_iter_init (&iter, input->entries);
		while (g_hash_table_iter_next (&iter, &key, &value)) {
			text = gtk_entry_get_text((GtkEntry *)value);
			if(strlen(text)) {
				retvalue = g_slice_new0(GValue);
				g_value_init(retvalue, G_TYPE_STRING);
				g_value_set_string(retvalue, text);
				g_hash_table_insert(reply, g_strdup(key), retvalue);
			}
		}

		connman_agent_request_input_set_reply(input->request_data, reply);
	} else {
		connman_agent_request_input_abort(input->request_data);
	}

	input_free(input);
}

static void show_dialog(gpointer data, gpointer user_data)
{
	struct input_data *input = data;

	gtk_widget_show_all(input->dialog);

	gtk_window_present(GTK_WINDOW(input->dialog));
}

static void request_input_dialog(GHashTable *request,
						gpointer request_data)
{
	GtkWidget *dialog;
	GtkWidget *label;
	GtkWidget *table;
	GtkWidget *entry;
	struct input_data *input;
	GHashTableIter iter;
	gpointer key, value;
	int elems, i;

	input = g_try_malloc0(sizeof(*input));
	if (!input)
		return;

	input->request_data = request_data;

	input->entries = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

	dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dialog), _("Connection Manager"));
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
	gtk_window_set_keep_above(GTK_WINDOW(dialog), TRUE);
	gtk_window_set_urgency_hint(GTK_WINDOW(dialog), TRUE);
	gtk_dialog_set_has_separator(GTK_DIALOG(dialog), FALSE);
	input->dialog = dialog;

	gtk_dialog_add_button(GTK_DIALOG(dialog),
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button(GTK_DIALOG(dialog),
					GTK_STOCK_OK, GTK_RESPONSE_OK);

	elems = g_hash_table_size(request);
	table = gtk_table_new(elems+1, 2, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 4);
	gtk_table_set_col_spacings(GTK_TABLE(table), 20);
	gtk_container_set_border_width(GTK_CONTAINER(table), 12);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), table);

	label = gtk_label_new(_("Please provide some network information:"));
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
	gtk_table_attach(GTK_TABLE(table), label, 0, 2, 0, 1,
				GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);

	g_hash_table_iter_init (&iter, request);
	i=1;
	while (g_hash_table_iter_next (&iter, &key, &value)) {
		label = gtk_label_new((const char *)key);
		gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
		gtk_table_attach(GTK_TABLE(table), label, 0, 1, i, i+1,
					GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);

		entry = gtk_entry_new();
		gtk_entry_set_max_length(GTK_ENTRY(entry), 64);
		gtk_entry_set_width_chars(GTK_ENTRY(entry), 16);
		gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);
		gtk_table_attach(GTK_TABLE(table), entry, 1, 2, i, i+1,
					GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
		g_hash_table_insert(input->entries, g_strdup(key), entry);

		i++;
	}

	input_data_inst = input;

	g_signal_connect(G_OBJECT(dialog), "response",
				G_CALLBACK(request_input_callback), input);

	show_dialog(input, NULL);
}

static void request_input(const char *service_id,
                              GHashTable *request, gpointer request_data, gpointer user_data)
{
	request_input_dialog(request, request_data);
}

static gboolean cancel_request(DBusGMethodInvocation *context,
							gpointer user_data)
{
	if( input_data_inst ) {
		connman_agent_request_input_abort(input_data_inst->request_data);

		input_free(input_data_inst);
	}

	return TRUE;
}

int setup_agents(void)
{
	ConnmanAgent *agent = connman_agent_new();
	connman_agent_setup(agent, "/org/gnome/connman/applet");

	connman_agent_set_request_input_func(agent, request_input, agent);
	connman_agent_set_cancel_func(agent, cancel_request, agent);

	connman_agent_register(agent);

	return 0;
}

void cleanup_agents(void)
{
}
