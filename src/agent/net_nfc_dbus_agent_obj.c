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

#include <stdlib.h>
#include <glib-object.h>
#include <dbus/dbus-glib-bindings.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

#include "net_nfc_typedef_private.h"
#include "net_nfc_debug_private.h"
#include "net_nfc_agent_private.h"
#include "net_nfc_dbus_agent_obj_private.h"
#include "dbus_agent_glue_private.h"
#include "dbus_service_binding_private.h"

G_DEFINE_TYPE(Dbus_Agent, dbus_agent, G_TYPE_OBJECT)

// Just Check the assert  and set the error message
#define __G_ASSERT(test, return_val, error, domain, error_code)\
G_STMT_START\
{\
	if G_LIKELY (!(test)) { \
		g_set_error (error, domain, error_code, #test); \
		return (return_val); \
	}\
}\
G_STMT_END

GQuark dbus_agent_error_quark(void)
{
	return g_quark_from_static_string("dbus_agent_error");
}

static void dbus_agent_init(Dbus_Agent *dbus_agent)
{
	GError *error = NULL;

	dbus_agent->connection = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);
	if (dbus_agent->connection == NULL)
	{
		DEBUG_MSG("failed to get dbus connection");
	}
}

static void dbus_agent_class_init(Dbus_AgentClass *dbus_agent_class)
{
	dbus_g_object_type_install_info(DBUS_AGENT_TYPE, &dbus_glib_dbus_agent_object_info);
}

gboolean dbus_agent_launch(Dbus_Agent *dbus_agent, guint *result_val, GError **error)
{
	DBusGProxy *proxy;
	guint status;
	GError *inter_error = NULL;
	int count = 0;

	DEBUG_MSG("---- dbus_agent_launch is called -----");

	proxy = dbus_g_proxy_new_for_name_owner(dbus_agent->connection, "com.samsung.slp.nfc.manager",
		"/com/samsung/slp/nfc/manager", "com.samsung.slp.nfc.manager", &inter_error);
	if (proxy != NULL)
	{
		DEBUG_MSG("Posting terminate again");
		com_samsung_slp_nfc_manager_terminate(proxy, &status, &inter_error);
	}
	else
	{
		_net_nfc_terminate_net_nfc_manager();
	}

	DEBUG_MSG("Posting terminate success");

	count = 0;
	while (_net_nfc_is_terminated() == FALSE)
	{
		usleep(30000);
		if (++count % 30)
		{
			_net_nfc_terminate_net_nfc_manager();
		}
	}

	if (_net_nfc_launch_net_nfc_manager() == FALSE)
	{
		return FALSE;
	}

	DEBUG_MSG("---- dbus_agent_launch is completed -----");

	return TRUE;
}

gboolean dbus_agent_terminate(Dbus_Agent *dbus_agent, guint *result_val, GError **error)
{
	DBusGProxy *proxy;
	guint status;
	GError *inter_error = NULL;

	DEBUG_MSG("---- dbus_agent_terminate is called -----");

	proxy = dbus_g_proxy_new_for_name_owner(dbus_agent->connection, "com.samsung.slp.nfc.manager",
		"/com/samsung/slp/nfc/manager", "com.samsung.slp.nfc.manager", &inter_error);

	if (proxy == NULL)
	{
		_net_nfc_terminate_net_nfc_manager();
	}
	else
	{
		if (!com_samsung_slp_nfc_manager_se_only(proxy, &status, &inter_error))
		{
			DEBUG_MSG("Termination is failed: %d", status);

			return FALSE;
		}
	}

	DEBUG_MSG("---- dbus_agent_terminate is completed -----");

	return TRUE;
}

