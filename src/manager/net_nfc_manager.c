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

#include <glib.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>


#include "net_nfc_server_ipc_private.h"
#include "net_nfc_debug_private.h"
#include "net_nfc_service_private.h"
#include "net_nfc_typedef_private.h"
#include "net_nfc_service_vconf_private.h"
#include "net_nfc_app_util_private.h"
#include "net_nfc_dbus_service_obj_private.h"
#include "net_nfc_controller_private.h"
#include "net_nfc_util_private.h"

#include "net_nfc_util_defines.h"
#include "heynoti.h"
#include "vconf.h"

GMainLoop* loop = NULL;

bool __net_nfc_intialize_dbus_connection ();
void __net_nfc_discovery_polling_cb(int signo);

int main()
{

	DEBUG_SERVER_MSG("start nfc manager");

	net_nfc_app_util_clean_storage(MESSAGE_STORAGE);

	if(!g_thread_supported())
	{
		g_thread_init(NULL);
	}

	dbus_g_thread_init();

	g_type_init();

	void* handle = NULL;

	handle = net_nfc_controller_onload();

	if(handle == NULL)
	{
		DEBUG_SERVER_MSG("load plugin library is failed");
		return 0;
	}

	if(net_nfc_server_ipc_initialize() != true)
	{
		DEBUG_ERR_MSG("nfc server ipc initialization is failed \n");
		return 0;
	}


	DEBUG_SERVER_MSG("nfc server ipc init is ok \n");


	if (__net_nfc_intialize_dbus_connection ())
	{
		DEBUG_SERVER_MSG("Registering DBUS is OK! \n");
	}
	else
	{
		DEBUG_SERVER_MSG("Registering DBUS is FAILED !!!\n");
	}

	int result = 0;
	int fd = 0;

	fd = heynoti_init();
	DEBUG_MSG("Noti init: %d\n", fd);
	if (fd == -1)
		return 0;

	/*Power Manager send the system_wakeup noti to subscriber*/
	result = heynoti_subscribe(fd, "system_wakeup", __net_nfc_discovery_polling_cb, (void *)fd);
	DEBUG_MSG("noti add: %d\n", result);

	if (result == -1)
		return 0;

	result = heynoti_attach_handler(fd);
	DEBUG_MSG("attach handler : %d\n", result);

	if (result == -1)
		return 0;

	loop = g_main_new(TRUE);
	g_main_loop_run(loop);

	net_nfc_service_vconf_unregister_notify_listener();
	net_nfc_server_ipc_finalize();
	net_nfc_controller_unload(handle);

	return 0;
}

void __net_nfc_discovery_polling_cb(int signo)
{
	int status;
	net_nfc_error_e result;

	DEBUG_MSG("__net_nfc_discovery_polling_cb[Enter]");

	if(net_nfc_controller_confiure_discovery(NET_NFC_DISCOVERY_MODE_CONFIG, NET_NFC_ALL_ENABLE, &result) == true)
	{
		DEBUG_SERVER_MSG("someone wake-up the nfc-manager daemon. and it succeeds to restart the polling. ");
	}
	else
	{
		if (vconf_set_int(NET_NFC_VCONF_KEY_PROGRESS, result) != 0)
		{
			DEBUG_ERR_MSG("vconf_set_int(NET_NFC_VCONF_KEY_PROGRESS, ERROR) != 0");
		}
		exit(result);
	}

	DEBUG_MSG("__net_nfc_discovery_polling_cb[Out]");
}

bool __net_nfc_intialize_dbus_connection ()
{

	GError *error = NULL;
	DBusGProxy *proxy;
	GObject *object = NULL;
	guint32 name;
	DBusGConnection *connection;

	connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
	if (connection == NULL) {
		DEBUG_ERR_MSG ("DBUS: getting dbus connection is failed \n");
		DEBUG_SERVER_MSG("DBUS: %s", error->message);
		g_error_free (error);
		return false;
	}

	object = g_object_new (DBUS_SERVICE_TYPE, NULL);

	dbus_g_connection_register_g_object (connection, DBUS_SERVICE_PATH, object);

	proxy = dbus_g_proxy_new_for_name (connection, DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS);

	/* Make sure getting name is correct version or not */
	if (!org_freedesktop_DBus_request_name (proxy, DBUS_SERVICE_NAME, 0, &name, &error))
	{
		DEBUG_ERR_MSG ("DBUS: getting dbus proxy is failed \n");
		DEBUG_SERVER_MSG("DBUS: %s", error->message);
		g_error_free (error);
		return false;
	}

	if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != name)
	{
		DEBUG_SERVER_MSG ("Requested name is: %d", name);
	}
	return true;
}

