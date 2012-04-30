/*
 * Copyright (c) 2000-2012 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * This file is part of nfc-manager
 *
 * PROPRIETARY/CONFIDENTIAL
 *
 * This software is the confidential and proprietary information of
 * SAMSUNG ELECTRONICS ("Confidential Information"). You shall not
 * disclose such Confidential Information and shall use it only in
 * accordance with the terms of the license agreement you entered
 * into with SAMSUNG ELECTRONICS.
 *
 * SAMSUNG make no representations or warranties about the suitability
 * of the software, either express or implied, including but not limited
 * to the implied warranties of merchantability, fitness for a particular
 * purpose, or non-infringement. SAMSUNG shall not be liable for any
 * damages suffered by licensee as a result of using, modifying or
 * distributing this software or its derivatives.
 *
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


