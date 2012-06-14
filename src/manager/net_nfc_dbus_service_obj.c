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
#include "net_nfc_controller_private.h"
#include "net_nfc_server_dispatcher_private.h"
#include "net_nfc_manager_util_private.h"
#include "net_nfc_dbus_service_obj_private.h"
#include "dbus_service_glue_private.h"

G_DEFINE_TYPE(Dbus_Service, dbus_service, G_TYPE_OBJECT)

/* Just Check the assert  and set the error message */
#define __G_ASSERT(test, return_val, error, domain, error_code)\
G_STMT_START\
{\
	if G_LIKELY (!(test)) { \
		g_set_error (error, domain, error_code, #test); \
		return (return_val); \
	}\
}\
G_STMT_END

GQuark dbus_service_error_quark(void)
{
	return g_quark_from_static_string("dbus_service_error");
}

static void dbus_service_init(Dbus_Service *dbus_service)
{
}

static void dbus_service_class_init(Dbus_ServiceClass *dbus_service_class)
{
	dbus_g_object_type_install_info(DBUS_SERVICE_TYPE, &dbus_glib_dbus_service_object_info);
}

gboolean dbus_service_launch(Dbus_Service *dbus_service, guint *result_val, GError **error)
{
	return TRUE;
}

gboolean dbus_service_se_only(Dbus_Service *dbus_service, guint *result_val, GError **error)
{
	net_nfc_request_terminate_t *term = NULL;

	DEBUG_SERVER_MSG("dbus_service_se_only is called ..\n");

	*result_val = NET_NFC_OK;

	_net_nfc_manager_util_alloc_mem(term, sizeof(net_nfc_request_terminate_t));
	if (term != NULL)
	{
		term->length = sizeof(net_nfc_request_terminate_t);
		term->request_type = NET_NFC_MESSAGE_SERVICE_TERMINATION;
		term->handle = NULL;
		term->object = dbus_service;

		net_nfc_dispatcher_queue_push((net_nfc_request_msg_t *)term);
	}

	return TRUE;
}

gboolean dbus_service_terminate(Dbus_Service *dbus_service, guint *result_val, GError **error)
{
	net_nfc_controller_deinit();

	DEBUG_SERVER_MSG("service terminated");

	exit(0);

	return TRUE;
}
