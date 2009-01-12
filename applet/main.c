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

#include "connman-client.h"

#include "status.h"

static ConnmanClient *client;

static void open_uri(GtkWindow *parent, const char *uri)
{
	GtkWidget *dialog;
	GdkScreen *screen;
	GError *error = NULL;
	gchar *cmdline;

	screen = gtk_window_get_screen(parent);

	cmdline = g_strconcat("xdg-open ", uri, NULL);

	if (gdk_spawn_command_line_on_screen(screen,
						cmdline, &error) == FALSE) {
		dialog = gtk_message_dialog_new(parent,
			GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
				GTK_BUTTONS_CLOSE, "%s", error->message);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		g_error_free(error);
	}

	g_free(cmdline);
}

static void about_url_hook(GtkAboutDialog *dialog,
					const gchar *url, gpointer data)
{
	open_uri(GTK_WINDOW(dialog), url);
}

static void about_email_hook(GtkAboutDialog *dialog,
					const gchar *email, gpointer data)
{
	gchar *uri;

	uri = g_strconcat("mailto:", email, NULL);
	open_uri(GTK_WINDOW(dialog), uri);
	g_free(uri);
}

static void about_callback(GtkWidget *item, gpointer user_data)
{
	const gchar *authors[] = {
		"Marcel Holtmann <marcel@holtmann.org>",
		NULL
	};

	gtk_about_dialog_set_url_hook(about_url_hook, NULL, NULL);
	gtk_about_dialog_set_email_hook(about_email_hook, NULL, NULL);

	gtk_show_about_dialog(NULL, "version", VERSION,
		"copyright", "Copyright \xc2\xa9 2008 Intel Corporation",
		"comments", _("A connection manager for the GNOME desktop"),
		"authors", authors,
		"translator-credits", _("translator-credits"),
		"logo-icon-name", "network-wireless", NULL);
}

static void settings_callback(GtkWidget *item, gpointer user_data)
{
	const char *command = "connman-properties";

	if (g_spawn_command_line_async(command, NULL) == FALSE)
		g_printerr("Couldn't execute command: %s\n", command);
}

static void show_error_dialog(const gchar *message)
{
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new_with_markup(NULL, GTK_DIALOG_MODAL,
					GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
								"%s", message);

	gtk_dialog_run(GTK_DIALOG(dialog));

	gtk_widget_destroy(dialog);
}

static void activate_callback(GtkWidget *item, gpointer user_data)
{
	const gchar *path = user_data;
	guint security;
	gchar *passphrase;

	security = connman_client_get_security(client, path);
	if (security == CONNMAN_SECURITY_UNKNOWN)
		return;

	if (security == CONNMAN_SECURITY_NONE) {
		connman_client_connect(client, path);
		return;
	}

	passphrase = connman_client_get_passphrase(client, path);
	if (passphrase != NULL) {
		g_free(passphrase);
		connman_client_connect(client, path);
		return;
	}

	show_error_dialog("Security settings for network are missing");
}

static GtkWidget *create_popupmenu(void)
{
	GtkWidget *menu;
	GtkWidget *item;

	menu = gtk_menu_new();

	item = gtk_image_menu_item_new_from_stock(GTK_STOCK_PREFERENCES, NULL);
	g_signal_connect(item, "activate", G_CALLBACK(settings_callback), NULL);
	gtk_widget_show(item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ABOUT, NULL);
	g_signal_connect(item, "activate", G_CALLBACK(about_callback), NULL);
	gtk_widget_show(item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	return menu;
}

static GtkWidget *append_menuitem(GtkMenu *menu, const char *ssid,
					guint security, guint strength)
{
	GtkWidget *item;
	GtkWidget *hbox;
	GtkWidget *label;
	GtkWidget *image;
	GtkWidget *progress;

	item = gtk_check_menu_item_new();
	gtk_check_menu_item_set_draw_as_radio(GTK_CHECK_MENU_ITEM(item), TRUE);

	hbox = gtk_hbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(item), hbox);
	gtk_widget_show(hbox);

	label = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_label_set_text(GTK_LABEL(label), ssid);
	gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);
	gtk_widget_show(label);

	image = gtk_image_new_from_stock(GTK_STOCK_DIALOG_AUTHENTICATION,
							GTK_ICON_SIZE_MENU);
	gtk_misc_set_alignment(GTK_MISC(image), 1.0, 0.5);
	if (security != CONNMAN_SECURITY_NONE) {
		gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);
		gtk_widget_show(image);
	}

	progress = gtk_progress_bar_new();
	gtk_widget_set_size_request(progress, 100, -1);
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress),
						(double) strength / 100);
	gtk_box_pack_end(GTK_BOX(hbox), progress, FALSE, TRUE, 0);
	gtk_widget_show(progress);

	gtk_widget_show(item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	return item;
}

static void enumerate_networks(GtkMenu *menu,
				GtkTreeModel *model, GtkTreeIter *parent)
{
	GtkTreeIter iter;
	gboolean cont;

	cont = gtk_tree_model_iter_children(model, &iter, parent);

	while (cont == TRUE) {
		GtkWidget *item;
		DBusGProxy *proxy;
		guint strength, security;
		gchar *name, *path;
		gboolean connected;

		gtk_tree_model_get(model, &iter,
				CONNMAN_COLUMN_PROXY, &proxy,
				CONNMAN_COLUMN_NAME, &name,
				CONNMAN_COLUMN_ENABLED, &connected,
				CONNMAN_COLUMN_STRENGTH, &strength,
				CONNMAN_COLUMN_SECURITY, &security, -1);

		item = append_menuitem(menu, name, security, strength);
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item),
								connected);

		path = g_strdup(dbus_g_proxy_get_path(proxy));
		g_signal_connect(item, "activate",
					G_CALLBACK(activate_callback), path);

		g_free(name);

		cont = gtk_tree_model_iter_next(model, &iter);
	}
}

static gboolean menu_callback(GtkMenu *menu)
{
	GtkTreeModel *model;
	GtkTreeIter parent;
	GtkWidget *item;
	gboolean cont;

	connman_client_propose_scan(client, NULL);

	model = connman_client_get_device_network_model(client);

	cont = gtk_tree_model_get_iter_first(model, &parent);

	while (cont == TRUE) {
		guint type;
		gchar *name;

		gtk_tree_model_get(model, &parent,
					CONNMAN_COLUMN_TYPE, &type,
					CONNMAN_COLUMN_NAME, &name, -1);

		switch (type) {
		case CONNMAN_TYPE_WIFI:
			enumerate_networks(menu, model, &parent);
			break;
		default:
			break;
		}

		g_free(name);

		cont = gtk_tree_model_iter_next(model, &parent);
	}

	g_object_unref(model);

	item = gtk_separator_menu_item_new();
	gtk_widget_show(item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	item = gtk_menu_item_new_with_label(_("Join Other Network..."));
	gtk_widget_set_sensitive(item, FALSE);
	//g_signal_connect(item, "activate",
	//			G_CALLBACK(join_callback), NULL);
	gtk_widget_show(item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	return TRUE;
}

static void update_status(GtkTreeModel *model)
{
	GtkTreeIter iter;
	gboolean cont;
	gboolean online = FALSE;
	guint strength, type;

	cont = gtk_tree_model_get_iter_first(model, &iter);

	while (cont == TRUE) {
		gboolean enabled;

		gtk_tree_model_get(model, &iter,
					CONNMAN_COLUMN_TYPE, &type,
					CONNMAN_COLUMN_STRENGTH, &strength,
					CONNMAN_COLUMN_ENABLED, &enabled, -1);

		online = TRUE;

		if (enabled == TRUE)
			break;

		cont = gtk_tree_model_iter_next(model, &iter);
	}

	if (online == FALSE) {
		status_offline();
		return;
	}

	switch (type) {
	case CONNMAN_TYPE_WIFI:
	case CONNMAN_TYPE_WIMAX:
		status_ready(strength / 25);
		break;
	default:
		status_ready(-1);
		break;
	}
}

static void connection_added(GtkTreeModel *model, GtkTreePath *path,
					GtkTreeIter *iter, gpointer user_data)
{
	update_status(model);
}

static void connection_removed(GtkTreeModel *model, GtkTreePath *path,
							gpointer user_data)
{
	update_status(model);
}

int main(int argc, char *argv[])
{
	GtkTreeModel *model;

	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);

	gtk_init(&argc, &argv);
	gtk_window_set_default_icon_name("network-wireless");

	g_set_application_name(_("Connection Manager"));

	status_init(menu_callback, create_popupmenu());

	client = connman_client_new();
	model = connman_client_get_connection_model(client);

	g_signal_connect(G_OBJECT(model), "row-inserted",
					G_CALLBACK(connection_added), NULL);
	g_signal_connect(G_OBJECT(model), "row-changed",
					G_CALLBACK(connection_added), NULL);
	g_signal_connect(G_OBJECT(model), "row-deleted",
					G_CALLBACK(connection_removed), NULL);

	update_status(model);

	gtk_main();

	g_object_unref(model);
	g_object_unref(client);

	status_cleanup();

	return 0;
}
