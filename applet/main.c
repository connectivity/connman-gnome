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

#if 0
static void toggled_callback(GtkWidget *button, gpointer user_data)
{
	GtkWidget *entry = user_data;
	gboolean mode;

	mode = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));

	gtk_entry_set_visibility(GTK_ENTRY(entry), mode);
}

static void passphrase_dialog(const char *path, const char *name)
{
	GtkWidget *dialog;
	GtkWidget *button;
	GtkWidget *image;
	GtkWidget *label;
	GtkWidget *entry;
	GtkWidget *table;
	GtkWidget *vbox;

	dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dialog), _("Enter passphrase"));
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
	gtk_window_set_keep_above(GTK_WINDOW(dialog), TRUE);
	gtk_window_set_urgency_hint(GTK_WINDOW(dialog), TRUE);
	gtk_dialog_set_has_separator(GTK_DIALOG(dialog), FALSE);

	button = gtk_dialog_add_button(GTK_DIALOG(dialog),
				GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT);
	button = gtk_dialog_add_button(GTK_DIALOG(dialog),
					GTK_STOCK_OK, GTK_RESPONSE_ACCEPT);
	gtk_widget_grab_default(button);

	table = gtk_table_new(5, 2, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 4);
	gtk_table_set_col_spacings(GTK_TABLE(table), 20);
	gtk_container_set_border_width(GTK_CONTAINER(table), 12);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), table);
	image = gtk_image_new_from_icon_name(GTK_STOCK_DIALOG_AUTHENTICATION,
							GTK_ICON_SIZE_DIALOG);
	gtk_misc_set_alignment(GTK_MISC(image), 0.0, 0.0);
	gtk_table_attach(GTK_TABLE(table), image, 0, 1, 0, 5,
						GTK_SHRINK, GTK_FILL, 0, 0);
	vbox = gtk_vbox_new(FALSE, 6);

	label = gtk_label_new(_("Network requires input of a passphrase:"));
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
	gtk_container_add(GTK_CONTAINER(vbox), label);
	gtk_table_attach(GTK_TABLE(table), vbox, 1, 2, 0, 1,
				GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);

	entry = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(entry), 120);
	gtk_entry_set_width_chars(GTK_ENTRY(entry), 20);
	gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);
	gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
	gtk_container_add(GTK_CONTAINER(vbox), entry);

	button = gtk_check_button_new_with_label(_("Show input"));
	gtk_container_add(GTK_CONTAINER(vbox), button);

	g_signal_connect(G_OBJECT(button), "toggled",
				G_CALLBACK(toggled_callback), entry);

	button = gtk_check_button_new_with_label(_("Remember network"));
	gtk_container_add(GTK_CONTAINER(vbox), button);

	gtk_widget_show_all(dialog);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		const gchar *passphrase;
		gboolean remember;

		passphrase = gtk_entry_get_text(GTK_ENTRY(entry));
		remember = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));

		connman_client_set_passphrase(client, path, passphrase);
		connman_client_set_remember(client, path, remember);

		status_prepare();
		connman_client_connect(client, path);
	}

	gtk_widget_destroy(dialog);
}
#endif

static GtkWidget *create_popupmenu(void)
{
	GtkWidget *menu;
	GtkWidget *item;

	menu = gtk_menu_new();

	item = gtk_image_menu_item_new_from_stock(GTK_STOCK_PREFERENCES, NULL);
	g_signal_connect(item, "activate", G_CALLBACK(settings_callback), NULL);
	//gtk_widget_show(item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ABOUT, NULL);
	g_signal_connect(item, "activate", G_CALLBACK(about_callback), NULL);
	gtk_widget_show(item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	return menu;
}

static void update_status(GtkTreeModel *model)
{
	GtkTreeIter iter;
	gboolean cont;
	gboolean prepare = FALSE, config = FALSE, online = FALSE;
	guint type, strength;

	cont = gtk_tree_model_get_iter_first(model, &iter);

	while (cont == TRUE) {
		guint state;

		gtk_tree_model_get(model, &iter,
				CONNMAN_COLUMN_TYPE, &type,
				CONNMAN_COLUMN_STATE, &state,
				CONNMAN_COLUMN_STRENGTH, &strength, -1);

		if (state == CONNMAN_STATE_READY) {
			online = TRUE;
			break;
		}

		if (state == CONNMAN_STATE_ASSOCIATION)
			prepare = TRUE;

		if (state == CONNMAN_STATE_CONFIGURATION)
			config = TRUE;

		cont = gtk_tree_model_iter_next(model, &iter);
	}

	if (config == TRUE) {
		status_config();
		return;
	}

	if (prepare == TRUE) {
		status_prepare();
		return;
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

static void status_callback(const char *status, void *user_data)
{
	if (g_str_equal(status, "unavailable") == TRUE)
		status_unavailable();
	else if (g_str_equal(status, "offline") == TRUE)
		status_offline();
	else if (g_str_equal(status, "connecting") == TRUE)
		status_config();
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

	status_init(NULL, create_popupmenu());

	client = connman_client_new();
	model = connman_client_get_connection_model(client);

	g_signal_connect(G_OBJECT(model), "row-inserted",
					G_CALLBACK(connection_added), NULL);
	g_signal_connect(G_OBJECT(model), "row-changed",
					G_CALLBACK(connection_added), NULL);
	g_signal_connect(G_OBJECT(model), "row-deleted",
					G_CALLBACK(connection_removed), NULL);

	update_status(model);

	connman_client_set_callback(client, status_callback, NULL);

	gtk_main();

	g_object_unref(model);
	g_object_unref(client);

	status_cleanup();

	return 0;
}
