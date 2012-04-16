/*
 *  Connection Manager Agent implementation
 *
 *  Author(s):
 *  - Julien MASSOT <jmassot@aldebaran-robotics.com>
 *  - Paul Eggleton <paul.eggleton@linux.intel.com>
 *
 *  Copyright (C) 2012 Aldebaran Robotics
 *  Copyright (C) 2012 Intel Corporation
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License version 2.1 as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <stdio.h>

#include "connman-agent.h"
#include "connman-dbus.h"

#define CONNMAN_AGENT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), \
  CONNMAN_TYPE_AGENT, ConnmanAgentPrivate))

typedef enum {
  AGENT_ERROR_REJECT,
  AGENT_ERROR_RETRY
} AgentError;

#define AGENT_ERROR (agent_error_quark())

#define AGENT_ERROR_TYPE (agent_error_get_type())

static GQuark agent_error_quark(void)
{
	static GQuark quark = 0;
	if (!quark)
		quark = g_quark_from_static_string("Agent");

	return quark;
}

#define ENUM_ENTRY(NAME, DESC) { NAME, "" #NAME "", DESC }

typedef struct _ConnmanAgentPrivate ConnmanAgentPrivate;

typedef struct _PendingRequest PendingRequest;

struct _PendingRequest {
	DBusGMethodInvocation *context;
	ConnmanAgent *agent;
};

struct _ConnmanAgentPrivate {
	gchar *busname;
	gchar *path;
	DBusGConnection *connection;
	DBusGProxy *connman_proxy;

	ConnmanAgentRequestInputFunc input_func;
	gpointer input_data;

	ConnmanAgentCancelFunc cancel_func;
	gpointer cancel_data;

	ConnmanAgentReleaseFunc release_func;
	gpointer release_data;

	ConnmanAgentDebugFunc debug_func;
	gpointer debug_data;

};

G_DEFINE_TYPE(ConnmanAgent, connman_agent, G_TYPE_OBJECT)

static inline void debug(ConnmanAgent *agent, const char *format, ...)
{
	char str[256];
	va_list ap;
	ConnmanAgentPrivate *priv = CONNMAN_AGENT_GET_PRIVATE(agent);

	if (priv->debug_func == NULL)
		return;

	va_start(ap, format);

	if (vsnprintf(str, sizeof(str), format, ap) > 0)
		priv->debug_func(str, priv->debug_data);

	va_end(ap);
}

gboolean connman_agent_request_input_set_reply(gpointer request_data, GHashTable *reply)
{
	PendingRequest *pendingrequest = request_data;

	if (request_data == NULL)
		return FALSE;

	dbus_g_method_return(pendingrequest->context, reply);

	g_free(pendingrequest);

	return FALSE;
}

gboolean connman_agent_request_input_abort(gpointer request_data)
{
	PendingRequest *pendingrequest = request_data;
	GError *result;
	if (request_data == NULL)
		return FALSE;

	result = g_error_new(AGENT_ERROR, AGENT_ERROR_REJECT,
	                     "Input request rejected");
	dbus_g_method_return_error(pendingrequest->context, result);
	g_clear_error(&result);
	g_free(pendingrequest);

	return FALSE;
}

static gboolean connman_agent_report_error(ConnmanAgent *agent,
					   const char *path, const char *error,
					   DBusGMethodInvocation *context)
{
	GError *result;

	debug(agent, "connection %s, reports an error: %s", path, error);
	result = g_error_new(AGENT_ERROR, AGENT_ERROR_RETRY,
	                     "Retry");
	dbus_g_method_return_error(context, result);
	g_clear_error(&result);

	return FALSE;
}

static gboolean connman_agent_request_input(ConnmanAgent *agent,
					    const char *path, GHashTable *fields,
					    DBusGMethodInvocation *context)
{
	ConnmanAgentPrivate *priv = CONNMAN_AGENT_GET_PRIVATE(agent);
	const char *sender = dbus_g_method_get_sender(context);
	char **id = NULL;
	PendingRequest *pendingrequest = NULL;

	debug(agent, "request %s, sender %s", path, sender);

	if (fields == NULL)
		return FALSE;

	if (priv->input_func != NULL) {
		id = g_strsplit(path, "/net/connman/service/", 2);
		if (g_strv_length(id) == 2) {
			pendingrequest = g_try_new0(PendingRequest, 1);
			pendingrequest->context = context;
			pendingrequest->agent   = agent;
			priv->input_func(id[1], fields, pendingrequest, priv->input_data);
		}
		g_strfreev(id);
	}

	return FALSE;
}

static gboolean connman_agent_cancel(ConnmanAgent *agent,
				     DBusGMethodInvocation *context)
{
	ConnmanAgentPrivate *priv = CONNMAN_AGENT_GET_PRIVATE(agent);
	const char *sender = dbus_g_method_get_sender(context);
	gboolean result = FALSE;

	debug(agent, "Request Canceled %s", sender);

	if (g_str_equal(sender, priv->busname) == FALSE)
		return FALSE;

	if (priv->cancel_func)
		result = priv->cancel_func(context, priv->cancel_data);

	return result;
}

static gboolean connman_agent_release(ConnmanAgent *agent,
				      DBusGMethodInvocation *context)
{
	ConnmanAgentPrivate *priv = CONNMAN_AGENT_GET_PRIVATE(agent);
	const char *sender = dbus_g_method_get_sender(context);

	debug(agent, "agent %p sender %s", agent, sender);

	if (g_str_equal(sender, priv->busname) == FALSE)
		return FALSE;

	dbus_g_method_return(context);

	return TRUE;
}

#include "connman-agent-glue.h"

static void connman_agent_init(ConnmanAgent *agent)
{
  debug(agent, "agent %p", agent);
}

static void connman_agent_finalize(GObject *agent)
{
	ConnmanAgentPrivate *priv = CONNMAN_AGENT_GET_PRIVATE(agent);

	if (priv->connman_proxy != NULL) {
		g_object_unref(priv->connman_proxy);
	}

	g_free(priv->path);
	g_free(priv->busname);
	dbus_g_connection_unref(priv->connection);

	G_OBJECT_CLASS(connman_agent_parent_class)->finalize(agent);
}

static void connman_agent_class_init(ConnmanAgentClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;

	g_type_class_add_private(klass, sizeof(ConnmanAgentPrivate));

	object_class->finalize = connman_agent_finalize;

	dbus_g_object_type_install_info(CONNMAN_TYPE_AGENT,
	                                &dbus_glib_connman_agent_object_info);
}

ConnmanAgent *connman_agent_new(void)
{
	ConnmanAgent *agent;
	g_type_init();

	agent = CONNMAN_AGENT(g_object_new(CONNMAN_TYPE_AGENT, NULL));

	return agent;
}

gboolean connman_agent_setup(ConnmanAgent *agent, const char *path)
{
	ConnmanAgentPrivate *priv = CONNMAN_AGENT_GET_PRIVATE(agent);
	DBusGProxy *proxy;
	GObject *object;
	GError *error = NULL;

	debug(agent, "agent_setup %p", agent);

	if (priv->path != NULL)
		return FALSE;

	priv->path = g_strdup(path);
	priv->connection = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);
	if (error != NULL) {
		g_printerr("Connecting to system bus failed: %s\n",
		           error->message);
		g_error_free(error);
		return FALSE;
	}

	proxy = dbus_g_proxy_new_for_name_owner(priv->connection, CONNMAN_SERVICE,
	                                        CONNMAN_MANAGER_PATH, CONNMAN_MANAGER_INTERFACE, NULL);

	g_free(priv->busname);

	if (proxy != NULL) {
		priv->busname = g_strdup(dbus_g_proxy_get_bus_name(proxy));
		g_object_unref(proxy);
	} else
		priv->busname = NULL;

	object = dbus_g_connection_lookup_g_object(priv->connection, priv->path);
	if (object != NULL)
		g_object_unref(object);

	return TRUE;
}


gboolean connman_agent_register(ConnmanAgent *agent)
{
	ConnmanAgentPrivate *priv = CONNMAN_AGENT_GET_PRIVATE(agent);
	GObject *object;
	GError *error = NULL;

	debug(agent, "register agent %p", agent);

	if (priv->connman_proxy != NULL)
		return FALSE;

	priv->connman_proxy = dbus_g_proxy_new_for_name_owner(priv->connection, CONNMAN_SERVICE,
	                                                      CONNMAN_MANAGER_PATH, CONNMAN_MANAGER_INTERFACE, NULL);

	g_free(priv->busname);

	priv->busname = g_strdup(dbus_g_proxy_get_bus_name(priv->connman_proxy));

	object = dbus_g_connection_lookup_g_object(priv->connection, priv->path);
	if (object != NULL)
		g_object_unref(object);

	dbus_g_connection_register_g_object(priv->connection,
	                                    priv->path, G_OBJECT(agent));

	dbus_g_proxy_call(priv->connman_proxy, "RegisterAgent", &error,
	                  DBUS_TYPE_G_OBJECT_PATH, priv->path,
	                  G_TYPE_INVALID, G_TYPE_INVALID);

	if (error != NULL) {
		g_printerr("Agent registration failed: %s\n",
		           error->message);
		g_error_free(error);
		return FALSE;
	}

	return TRUE;
}

gboolean connman_agent_unregister(ConnmanAgent *agent)
{
	ConnmanAgentPrivate *priv = CONNMAN_AGENT_GET_PRIVATE(agent);
	GError *error = NULL;

	debug(agent, "unregister agent %p", agent);

	if (priv->connman_proxy == NULL)
		return FALSE;

	dbus_g_proxy_call(priv->connman_proxy, "UnregisterAgent", &error,
	                  DBUS_TYPE_G_OBJECT_PATH, priv->path,
	                  G_TYPE_INVALID, G_TYPE_INVALID);

	if (error != NULL) {
		g_printerr("Agent unregistration failed: %s\n",
		           error->message);
		g_error_free(error);
	}

	g_object_unref(priv->connman_proxy);
	priv->connman_proxy = NULL;

	g_free(priv->path);
	priv->path = NULL;

	return TRUE;
}

void connman_agent_set_request_input_func(ConnmanAgent *agent,
                                          ConnmanAgentRequestInputFunc func, gpointer data)
{
	ConnmanAgentPrivate *priv = CONNMAN_AGENT_GET_PRIVATE(agent);

	priv->input_func = func;
	priv->input_data = data;
}

void connman_agent_set_cancel_func(ConnmanAgent *agent,
                                   ConnmanAgentCancelFunc func, gpointer data)
{
	ConnmanAgentPrivate *priv = CONNMAN_AGENT_GET_PRIVATE(agent);

	priv->cancel_func = func;
	priv->cancel_data = data;
}
