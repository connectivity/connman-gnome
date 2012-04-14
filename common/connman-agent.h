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

#ifndef   	CONNMAN_AGENT_H_
# define   	CONNMAN_AGENT_H_

#include <glib-object.h>
#include <dbus/dbus-glib.h>

G_BEGIN_DECLS

#define CONNMAN_TYPE_AGENT (connman_agent_get_type())
#define CONNMAN_AGENT(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), \
                                        CONNMAN_TYPE_AGENT, ConnmanAgent))
#define CONNMAN_AGENT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), \
                                        CONNMAN_TYPE_AGENT, ConnmanAgentClass))
#define CONNMAN_IS_AGENT(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
                                                        CONNMAN_TYPE_AGENT))
#define CONNMAN_IS_AGENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), \
                                                        CONNMAN_TYPE_AGENT))
#define CONNMAN_GET_AGENT_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), \
                                        CONNMAN_TYPE_AGENT, ConnmanAgentClass))

typedef struct _ConnmanAgent ConnmanAgent;
typedef struct _ConnmanAgentClass ConnmanAgentClass;

struct _ConnmanAgent {
	GObject parent;
};

struct _ConnmanAgentClass {
	GObjectClass parent_class;
};

GType connman_agent_get_type(void);

ConnmanAgent *connman_agent_new(void);

gboolean connman_agent_setup(ConnmanAgent *agent, const char *path);

gboolean connman_agent_register(ConnmanAgent *agent);
gboolean connman_agent_unregister(ConnmanAgent *agent);
gboolean connman_agent_request_input_set_reply(gpointer request_data, GHashTable *reply);
gboolean connman_agent_request_input_abort(gpointer request_data);

typedef void (*ConnmanAgentRequestInputFunc) (const char *service_id, GHashTable *request, gpointer request_data, gpointer user_data);
typedef gboolean (*ConnmanAgentCancelFunc) (DBusGMethodInvocation *context, gpointer data);
typedef gboolean (*ConnmanAgentReleaseFunc) (DBusGMethodInvocation *context, gpointer data);
typedef void (*ConnmanAgentDebugFunc) (const char *str, gpointer user_data);

void connman_agent_set_request_input_func(ConnmanAgent *agent, ConnmanAgentRequestInputFunc func, gpointer data);
void connman_agent_set_cancel_func(ConnmanAgent *agent, ConnmanAgentCancelFunc func, gpointer data);
void connman_agent_set_debug_func(ConnmanAgent *agent, ConnmanAgentDebugFunc func, gpointer data);

G_END_DECLS
#endif 	    /* !CONNMAN_AGENT_H_ */
