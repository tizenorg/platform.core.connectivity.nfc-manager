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


#include <vconf.h>
#include <glib.h>
#include <string.h>
#include "net_nfc_typedef_private.h"
#include "net_nfc_debug_private.h"
#include "net_nfc_controller_private.h"
#include "net_nfc_util_private.h"
#include "net_nfc_server_dispatcher_private.h"
#include "net_nfc_app_util_private.h"
#include "aul.h"



/////////////////////////////////////////////////////////////////////////

static void net_nfc_service_airplane_mode_cb(keynode_t* key, void* data)
{
	int flight_mode = 0;
	int nfc_state = 0;
	static gboolean powered_off_by_flightmode = FALSE;

	vconf_get_bool(VCONFKEY_SETAPPL_FLIGHT_MODE_BOOL, &flight_mode);
	vconf_get_bool(VCONFKEY_NFC_STATE, &nfc_state);

	DEBUG_SERVER_MSG("flight mode %s \n", flight_mode > 0 ? "ON" : "OFF");
	DEBUG_SERVER_MSG("NFC state %d, NFC was off by flight mode %s \n",
			nfc_state, powered_off_by_flightmode == TRUE ? "Yes" : "No");

	if (flight_mode > 0)
	{
		/* flight mode enabled */
		if (nfc_state == VCONFKEY_NFC_STATE_OFF)
			return;

		DEBUG_SERVER_MSG("Turning NFC off \n");

		/* nfc off */
		net_nfc_request_msg_t *req_msg = NULL;

		_net_nfc_util_alloc_mem(req_msg, sizeof(net_nfc_request_msg_t));
		if (req_msg == NULL)
			return false;

		req_msg->length = sizeof(net_nfc_request_msg_t);
		req_msg->request_type = NET_NFC_MESSAGE_SERVICE_DEINIT;

		net_nfc_dispatcher_queue_push(req_msg);

		/* set internal flag */
		powered_off_by_flightmode = TRUE;
	}
	else if (flight_mode == 0)
	{
		/* flight mode disabled */
		if (nfc_state > VCONFKEY_NFC_STATE_OFF)
			return;

		if (powered_off_by_flightmode != TRUE)
			return;

		DEBUG_SERVER_MSG("Turning NFC on \n");

		/* nfc on */
		net_nfc_request_msg_t *req_msg = NULL;

		_net_nfc_util_alloc_mem(req_msg, sizeof(net_nfc_request_msg_t));
		if (req_msg == NULL)
			return false;

		req_msg->length = sizeof(net_nfc_request_msg_t);
		req_msg->request_type = NET_NFC_MESSAGE_SERVICE_INIT;

		net_nfc_dispatcher_queue_push(req_msg);

		/* unset internal flag */
		powered_off_by_flightmode = FALSE;
	}
	else
	{
		DEBUG_SERVER_MSG("Invalid Vconf value \n");
	}
}



void net_nfc_service_vconf_register_notify_listener()
{
	vconf_notify_key_changed(VCONFKEY_SETAPPL_FLIGHT_MODE_BOOL, net_nfc_service_airplane_mode_cb, NULL);
}

void net_nfc_service_vconf_unregister_notify_listener()
{
	vconf_ignore_key_changed(VCONFKEY_SETAPPL_FLIGHT_MODE_BOOL, net_nfc_service_airplane_mode_cb);
}
