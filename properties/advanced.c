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

#include "client.h"
#include "advanced.h"

static void delete_callback(GtkWidget *window, GdkEvent *event,
							gpointer user_data)
{
	gtk_widget_hide(window);
}

static void close_callback(GtkWidget *button, gpointer user_data)
{
	GtkWidget *window = user_data;

	gtk_widget_hide(window);
}

void create_advanced_dialog(struct config_data *data, guint type)
{
	GtkWidget *dialog;
	GtkWidget *vbox;
	GtkWidget *notebook;
	GtkWidget *buttonbox;
	GtkWidget *button;
	GtkWidget *widget;

	dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
	gtk_window_set_transient_for(GTK_WINDOW(dialog),
						GTK_WINDOW(data->window));
	gtk_window_set_title(GTK_WINDOW(dialog), _("Advanced Settings"));
	gtk_window_set_position(GTK_WINDOW(dialog),
					GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_default_size(GTK_WINDOW(dialog), 460, 320);
	g_signal_connect(G_OBJECT(dialog), "delete-event",
					G_CALLBACK(delete_callback), NULL);

	vbox = gtk_vbox_new(FALSE, 12);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);
	gtk_container_add(GTK_CONTAINER(dialog), vbox);

	notebook = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

	buttonbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(buttonbox), GTK_BUTTONBOX_END);
	gtk_box_set_spacing(GTK_BOX(buttonbox), 6);
	gtk_box_pack_start(GTK_BOX(vbox), buttonbox, FALSE, FALSE, 0);

	button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	gtk_container_add(GTK_CONTAINER(buttonbox), button);
	g_signal_connect(G_OBJECT(button), "clicked",
					G_CALLBACK(close_callback), dialog);

	button = gtk_button_new_from_stock(GTK_STOCK_OK);
	gtk_container_add(GTK_CONTAINER(buttonbox), button);
	g_signal_connect(G_OBJECT(button), "clicked",
					G_CALLBACK(close_callback), dialog);

	if (type == CLIENT_TYPE_80211) {
		widget = gtk_label_new(NULL);
		gtk_notebook_append_page(GTK_NOTEBOOK(notebook), widget, NULL);
		gtk_notebook_set_tab_label_text(GTK_NOTEBOOK(notebook),
							widget, _("Wireless"));
	}

	widget = gtk_label_new(NULL);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), widget, NULL);
	gtk_notebook_set_tab_label_text(GTK_NOTEBOOK(notebook),
						widget, _("TCP/IP"));

	widget = gtk_label_new(NULL);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), widget, NULL);
	gtk_notebook_set_tab_label_text(GTK_NOTEBOOK(notebook),
						widget, _("DNS"));

	if (type == CLIENT_TYPE_80203) {
		widget = gtk_label_new(NULL);
		gtk_notebook_append_page(GTK_NOTEBOOK(notebook), widget, NULL);
		gtk_notebook_set_tab_label_text(GTK_NOTEBOOK(notebook),
							widget, _("Ethernet"));
	}

	data->dialog = dialog;
}
