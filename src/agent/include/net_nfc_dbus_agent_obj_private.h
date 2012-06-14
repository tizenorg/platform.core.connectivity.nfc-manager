/*
  * Copyright 2012  Samsung Electronics Co., Ltd
  *
  * Licensed under the Flora License, Version 1.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at

  *     http://www.tizenopensource.org/license
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  */

#include <glib-object.h>
#include <dbus/dbus-glib-bindings.h>


typedef struct _Dbus_Agent Dbus_Agent;
typedef struct _Dbus_AgentClass Dbus_AgentClass;

#define DBUS_AGENT_NAME "com.samsung.slp.nfc.agent"
#define DBUS_AGENT_PATH "/com/samsung/slp/nfc/agent"


GType dbus_agent_get_type (void);

struct _Dbus_Agent
{
  GObject parent;
  int status;
  DBusGConnection *connection;
};

struct _Dbus_AgentClass
{
  GObjectClass parent;
};

#define DBUS_AGENT_TYPE				(dbus_agent_get_type ())
#define DBUS_AGENT(object)			(G_TYPE_CHECK_INSTANCE_CAST ((object), DBUS_AGENT_TYPE, Dbus_Agent))
#define DBUS_AGENT_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), DBUS_AGENT_TYPE, Dbus_AgentClass))
#define IS_DBUS_AGENT(object)			(G_TYPE_CHECK_INSTANCE_TYPE ((object), DBUS_AGENT_TYPE))
#define IS_DBUS_AGENT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), DBUS_AGENT_TYPE))
#define DBUS_AGENT_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj), DBUS_AGENT_TYPE, Dbus_AgentClass))

typedef enum
{
  DBUS_AGENT_ERROR_INVALID_PRAM
} Dbus_Agent_Error;

GQuark dbus_agent_error_quark (void);
#define DBUS_AGENT_ERROR dbus_agent_error_quark ()

/**
 *     launch the nfc-manager
 */
gboolean dbus_agent_launch (Dbus_Agent *dbus_agent, guint *result_val, GError **error);

gboolean dbus_agent_terminate (Dbus_Agent *dbus_agent, guint *result_val, GError **error);


