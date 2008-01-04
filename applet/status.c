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

#include <gtk/gtk.h>

#include "status.h"

static GtkStatusIcon *statusicon = NULL;

static GdkPixbuf *pixbuf_load(GtkIconTheme *icontheme, const gchar *name)
{
	GdkPixbuf *pixbuf;
	GError *error = NULL;

	pixbuf = gtk_icon_theme_load_icon(icontheme, name, 22, 0, &error);

	if (pixbuf == NULL) {
		g_warning("Missing icon %s: %s", name, error->message);
		g_error_free(error);
	}

	return pixbuf;
}

typedef struct {
	guint id;
	guint count;
	guint frame;
	guint start;
	guint end;
	GdkPixbuf **pixbuf;
} IconAnimation;

static IconAnimation *icon_animation_load(GtkIconTheme *icontheme,
					const gchar *pattern, guint count)
{
	IconAnimation *animation;
	int i;

	animation = g_new0(IconAnimation, 1);

	animation->frame = 0;
	animation->count = count;

	animation->pixbuf = (void *) g_new(gpointer, count);
	if (animation->pixbuf == NULL) {
		g_free(animation);
		return NULL;
	}

	for (i = 0; i < count; i++) {
		gchar *name = g_strdup_printf("%s-%02d", pattern, i + 1);

		animation->pixbuf[i] = pixbuf_load(icontheme, name);

		g_free(name);
	}

	return animation;
}

static gboolean icon_animation_timeout(gpointer data)
{
	IconAnimation *animation = data;

	gtk_status_icon_set_from_pixbuf(statusicon,
				animation->pixbuf[animation->frame]);

	animation->frame++;
	if (animation->frame > animation->end)
		animation->frame = animation->start;

	return TRUE;
}

static void icon_animation_start(IconAnimation *animation,
						guint start, guint end)
{
	animation->start = start;
	animation->end = (end == 0) ? animation->count - 1 : end;

	if (animation->id > 0)
		return;

	animation->id = g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE, 100,
				icon_animation_timeout, animation, NULL);
}

static void icon_animation_stop(IconAnimation *animation)
{
	if (animation->id > 0)
		g_source_remove(animation->id);

	animation->id = 0;

	animation->frame = 0;
}

static void icon_animation_free(IconAnimation *animation)
{
	int i;

	gtk_status_icon_set_visible(statusicon, FALSE);

	icon_animation_stop(animation);

	gtk_status_icon_set_from_pixbuf(statusicon, NULL);

	for (i = 0; i < animation->count; i++)
		if (animation->pixbuf[i])
			g_object_unref(animation->pixbuf[i]);

	g_free(animation->pixbuf);

	g_free(animation);
}

static void activate_callback(GObject *object, gpointer user_data)
{
	GtkWidget *menu = user_data;
	guint32 activate_time = gtk_get_current_event_time();

	if (menu == NULL)
		return;

	gtk_menu_popup(GTK_MENU(menu), NULL, NULL,
			gtk_status_icon_position_menu,
			GTK_STATUS_ICON(object), 1, activate_time);
}

static void popup_callback(GObject *object, guint button,
				guint activate_time, gpointer user_data)
{
	GtkWidget *menu = user_data;

	if (menu == NULL)
		return;

	gtk_menu_popup(GTK_MENU(menu), NULL, NULL,
			gtk_status_icon_position_menu,
			GTK_STATUS_ICON(object), button, activate_time);
}

static GtkIconTheme *icontheme;
static IconAnimation *animation;
static GdkPixbuf *pixbuf_none;
static GdkPixbuf *pixbuf_wired;

int status_init(GtkWidget *activate, GtkWidget *popup)
{
	GdkScreen *screen;

	statusicon = gtk_status_icon_new();

	gtk_status_icon_set_visible(statusicon, FALSE);

	screen = gtk_status_icon_get_screen(statusicon);

	icontheme = gtk_icon_theme_get_for_screen(screen);

	gtk_icon_theme_append_search_path(icontheme, ICONDIR);

	animation = icon_animation_load(icontheme, "connman-connecting", 33);

	pixbuf_none = pixbuf_load(icontheme, "connman-type-none");
	pixbuf_wired = pixbuf_load(icontheme, "connman-type-wired");

	g_signal_connect(statusicon, "activate",
				G_CALLBACK(activate_callback), activate);

	g_signal_connect(statusicon, "popup-menu",
				G_CALLBACK(popup_callback), popup);

	return 0;
}

void status_cleanup(void)
{
	icon_animation_free(animation);

	g_object_unref(pixbuf_none);
	g_object_unref(pixbuf_wired);

	g_object_unref(icontheme);

	g_object_unref(statusicon);
}

void status_hide(void)
{
	gtk_status_icon_set_visible(statusicon, FALSE);

	icon_animation_stop(animation);

	gtk_status_icon_set_from_pixbuf(statusicon, NULL);
}

void status_offline(void)
{
	icon_animation_stop(animation);

	gtk_status_icon_set_from_pixbuf(statusicon, pixbuf_none);

	gtk_status_icon_set_visible(statusicon, TRUE);
}

void status_prepare(void)
{
	gtk_status_icon_set_visible(statusicon, TRUE);

	icon_animation_start(animation, 0, 10);
}

void status_config(void)
{
	gtk_status_icon_set_visible(statusicon, TRUE);

	icon_animation_start(animation, 11, 21);
}

static gboolean ready_timeout(gpointer data)
{
	IconAnimation *animation = data;

	icon_animation_stop(animation);

	gtk_status_icon_set_from_pixbuf(statusicon, pixbuf_wired);

	return FALSE;
}

void status_ready(void)
{
	if (animation->id > 0) {
		gtk_status_icon_set_visible(statusicon, TRUE);

		icon_animation_start(animation, 22, 32);

		g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE, 1500,
					ready_timeout, animation, NULL);
	} else {
		gtk_status_icon_set_from_pixbuf(statusicon, pixbuf_wired);

		gtk_status_icon_set_visible(statusicon, TRUE);
	}
}
