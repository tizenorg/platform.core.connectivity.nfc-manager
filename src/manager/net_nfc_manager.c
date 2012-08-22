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

#include <glib.h>
#include <pthread.h>
#include <dbus/dbus-glib.h>
//#include <dbus/dbus-glib-bindings.h>
#include <sys/utsname.h>


#include "heynoti.h"
#include "vconf.h"

#include "net_nfc_server_ipc_private.h"
#include "net_nfc_debug_private.h"
#include "net_nfc_service_private.h"
#include "net_nfc_typedef_private.h"
#include "net_nfc_service_vconf_private.h"
#include "net_nfc_app_util_private.h"
#include "net_nfc_controller_private.h"
#include "net_nfc_util_private.h"
#include "net_nfc_util_defines.h"
#include "net_nfc_server_dispatcher_private.h"

GMainLoop *loop = NULL;

void __net_nfc_discovery_polling_cb(int signo);
bool Check_Redwood();


int main()
{
	int result = 0;
	void *handle = NULL;
	/*+REDWOOD+*/
	bool ret = false;
	/*-REDWOOD-*/

	DEBUG_SERVER_MSG("start nfc manager");

	net_nfc_app_util_clean_storage(MESSAGE_STORAGE);

	if(!g_thread_supported())
	{
		g_thread_init(NULL);
	}

	g_type_init();

	handle = net_nfc_controller_onload();
	if(handle == NULL)
	{
		DEBUG_SERVER_MSG("load plugin library is failed");
		return 0;
	}

	if (net_nfc_controller_support_nfc(&result) == true)
	{
		DEBUG_SERVER_MSG("NFC Support");

		/*+REDWOOD+*/
		ret = Check_Redwood();
		if(ret == true)
		{
			DEBUG_ERR_MSG("NFC doesn't support");

			vconf_set_bool(VCONFKEY_NFC_FEATURE, VCONFKEY_NFC_FEATURE_OFF);
			vconf_set_bool(VCONFKEY_NFC_STATE, FALSE);

			net_nfc_controller_unload(handle);
		}
		else
		{
		vconf_set_bool(VCONFKEY_NFC_FEATURE, VCONFKEY_NFC_FEATURE_ON);
	}
		/*-REDWOOD-*/
	}
	else
	{
		DEBUG_ERR_MSG("NFC doesn't support");

		vconf_set_bool(VCONFKEY_NFC_FEATURE, VCONFKEY_NFC_FEATURE_OFF);
		vconf_set_bool(VCONFKEY_NFC_STATE, FALSE);

		net_nfc_controller_unload(handle);

		//return(0);
	}

	if(net_nfc_server_ipc_initialize() != true)
	{
		DEBUG_ERR_MSG("nfc server ipc initialization is failed \n");

		net_nfc_controller_unload(handle);

		return 0;
	}

	DEBUG_SERVER_MSG("nfc server ipc init is ok \n");

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

	net_nfc_service_vconf_register_notify_listener();

	loop = g_main_new(TRUE);
	g_main_loop_run(loop);

	net_nfc_service_vconf_unregister_notify_listener();
	net_nfc_server_ipc_finalize();
	net_nfc_controller_unload(handle);

	return 0;
}

void __net_nfc_discovery_polling_cb(int signo)
{
	int state;
	net_nfc_error_e result;

	result = vconf_get_bool(VCONFKEY_NFC_STATE, &state);
	if (result != 0)
	{
		DEBUG_MSG("VCONFKEY_NFC_STATE is not exist: %d ", result);
	}

	DEBUG_MSG("__net_nfc_discovery_polling_cb[Enter]");

	if(state == TRUE)
	{
		net_nfc_request_msg_t *req_msg = NULL;

		_net_nfc_util_alloc_mem(req_msg, sizeof(net_nfc_request_msg_t));

		if (req_msg == NULL)
		{
			DEBUG_MSG("_net_nfc_util_alloc_mem[NULL]");
			return false;
		}

		req_msg->length = sizeof(net_nfc_request_msg_t);
		req_msg->request_type = NET_NFC_MESSAGE_SERVICE_RESTART_POLLING_LOOP;

		net_nfc_dispatcher_queue_push(req_msg);
	}
 	else
	{
		DEBUG_SERVER_MSG("Don't need to wake up. NFC is OFF!!");
	}

	DEBUG_MSG("__net_nfc_discovery_polling_cb[Out]");
}

bool Check_Redwood()
{
	struct utsname hwinfo;

	int ret = uname(&hwinfo);
	DEBUG_MSG("uname returned %d",ret);
	DEBUG_MSG("sysname::%s",hwinfo.sysname);
	DEBUG_MSG("release::%s",hwinfo.release);
	DEBUG_MSG("version::%s",hwinfo.version);
	DEBUG_MSG("machine::%s",hwinfo.machine);
	DEBUG_MSG("nodename::%s",hwinfo.nodename);

	if(strstr(hwinfo.nodename,"REDWOOD"))
	{
		DEBUG_MSG("REDWOOD harware");
		return true;

	}
	else
	{
		DEBUG_MSG("except REDWOOD");
		return false;
	}

}


