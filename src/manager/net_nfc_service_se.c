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

#include "net_nfc_controller_private.h"
#include "net_nfc_util_private.h"
#include "net_nfc_typedef.h"
#include "net_nfc_debug_private.h"
#include "net_nfc_service_private.h"
#include "net_nfc_app_util_private.h"
#include "net_nfc_dbus_service_obj_private.h"
#include "net_nfc_server_ipc_private.h"
#include "net_nfc_server_dispatcher_private.h"
#include "net_nfc_manager_util_private.h"
#include "net_nfc_service_se_private.h"

#include <pthread.h>
#include <glib.h>
#include <malloc.h>

#include <TapiCommon.h>
#include <TelSim.h>
#include <ITapiSim.h>

/* define */
/* For ESE*/
se_setting_t g_se_setting;


/* For UICC */

static unsigned int tapi_event_list[] =
{
        TAPI_EVENT_SIM_APDU_CNF,
        TAPI_EVENT_SIM_ATR_CNF
};

typedef struct _uicc_context_t{
	int req_id;
	void* trans_param;
}uicc_context_t;

static unsigned int *p_event_subscription_ids = NULL;

static bool net_nfc_service_check_sim_state(void);
static int  sim_callback  (TelTapiEvent_t *sim_event);

#define TAPI_EVENT_COUNT (sizeof(tapi_event_list) / sizeof(unsigned int))

static GList * list = NULL;


static int _compare_func (gconstpointer key1, gconstpointer key2)
{
	uicc_context_t * arg1 = (uicc_context_t*) key1;
	uicc_context_t * arg2 = (uicc_context_t *) key2;

	if (arg1->req_id < arg2->req_id){
		return -1;
	}
	else if (arg1->req_id > arg2->req_id){
		return 1;
	}
	else {
		return 0;
	}
}

static void _append_data(uicc_context_t* data)
{
	if (data != NULL)
	{
		list = g_list_append (list, data);
	}
}

static uicc_context_t* _get_data(int req_id)
{
	if(list != NULL)
	{

		GList* found = NULL;

		uicc_context_t value = {0, NULL};
		value.req_id = req_id;

		found = g_list_find_custom (list, &value, _compare_func );

		if(found != NULL){
			return (uicc_context_t*)found->data;
		}
	}

	return NULL;
}

static void _remove_data(uicc_context_t* data)
{
	if(list != NULL)
	{

		list = g_list_remove (list, data);
	}
}

void net_nfc_service_se_detected(net_nfc_request_msg_t *msg)
{
	net_nfc_request_target_detected_t *detail_msg = NULL;
	net_nfc_target_handle_s *handle = detail_msg->handle;
	net_nfc_error_e state = NET_NFC_OK;
	net_nfc_response_open_internal_se_t resp = { 0 };

	if (msg == NULL)
	{
		return;
	}

	detail_msg = (net_nfc_request_target_detected_t *)(msg);
	g_se_setting.current_ese_handle = handle;

	DEBUG_SERVER_MSG("trying to connect to ESE = [0x%x]", handle);

	if (!net_nfc_controller_connect(handle, &state))
	{
		DEBUG_SERVER_MSG("connect failed = [%d]", state);
		resp.result = state;
	}

#ifdef BROADCAST_MESSAGE
	net_nfc_server_set_server_state( NET_NFC_SE_CONNECTED);
#endif

	resp.handle = handle;
	resp.trans_param = g_se_setting.open_request_trans_param;
	resp.se_type = SECURE_ELEMENT_TYPE_ESE;

	DEBUG_SERVER_MSG("trans param = [%d]", resp.trans_param);

	_net_nfc_send_response_msg(NET_NFC_MESSAGE_OPEN_INTERNAL_SE, &resp, sizeof(net_nfc_response_open_internal_se_t), NULL);

	g_se_setting.open_request_trans_param = NULL;
}


bool net_nfc_service_tapi_init(void)
{
	DEBUG_SERVER_MSG("tapi init");

	unsigned int event_counts = TAPI_EVENT_COUNT;

	if ( tel_init() == TAPI_API_SUCCESS)
	{
		if((p_event_subscription_ids = (unsigned int *)calloc(1, sizeof(tapi_event_list))) == NULL)
		{
			return false;
		}


		int i = 0;

		for(; i < event_counts; i++)
		{
			tel_register_event(tapi_event_list[i], &(p_event_subscription_ids[i]), (TelAppCallback)sim_callback, NULL);
		}

	}
	else
	{
		DEBUG_SERVER_MSG(" tel_init() failed");
		return false;
	}

	tel_register_app_name("com.samsung.nfc");

	DEBUG_SERVER_MSG(" tel_init() is success");

	return net_nfc_service_check_sim_state();

}

void net_nfc_service_tapi_deinit(void)
{
	DEBUG_SERVER_MSG("deinit tapi");

	unsigned int event_counts = TAPI_EVENT_COUNT;
	int i = 0;

	if(p_event_subscription_ids != NULL)
	{
		for(; i < event_counts; i++){
			tel_deregister_event(p_event_subscription_ids[i]);
		}
	}

	tel_deinit();

	if(p_event_subscription_ids != NULL)
	{
		free(p_event_subscription_ids);
		p_event_subscription_ids = NULL;
	}
}

bool net_nfc_service_transfer_apdu(data_s* apdu, void* trans_param)
{
	if(apdu == NULL)
		return false;

	TelSimApdu_t  apdu_data = {0};

	apdu_data.apdu = apdu->buffer;
	apdu_data.apdu_len = apdu->length;

	DEBUG_SERVER_MSG("tranfer apdu \n");

	int req_id = 0;
	TapiResult_t error = tel_req_sim_apdu(&apdu_data, (int *)&req_id);

	if(error != TAPI_API_SUCCESS)
	{
		DEBUG_SERVER_MSG("request sim apdu is failed with error = [%d] \n", error);
		return false;
	}
	else
	{
		uicc_context_t* context = calloc(1, sizeof(uicc_context_t));

		if(context != NULL)
		{
			context->req_id = req_id;
			_append_data(context);
		}

		DEBUG_SERVER_MSG("sim apdu request is success \n");
	}

	return true;
}

bool net_nfc_service_request_atr(void* trans_param)
{
	int req_id = 0;
	TapiResult_t error = tel_req_sim_atr((int *)&req_id);

	if(error != TAPI_API_SUCCESS)
	{
		DEBUG_SERVER_MSG("failed to request ATR  = [%d]\n", error);
		return false;
	}
	else
	{
		uicc_context_t* context = calloc(1, sizeof(uicc_context_t));

		if(context != NULL)
		{
			context->req_id = req_id;
			_append_data(context);
		}

		DEBUG_SERVER_MSG("request is success \n");
	}

	return true;
}

static bool net_nfc_service_check_sim_state(void)
{
	DEBUG_SERVER_MSG("check sim state");

	TelSimCardStatus_t state = TAPI_API_SUCCESS;
	int b_card_changed = 0;

	TapiResult_t error  = tel_get_sim_init_info(&state, &b_card_changed);

	DEBUG_SERVER_MSG("current sim init state = [%d] \n", state);

	if(error != TAPI_API_SUCCESS)
	{
		DEBUG_SERVER_MSG("error = [%d] \n", error);
		return false;
	}
	else if(state ==TAPI_SIM_STATUS_SIM_INIT_COMPLETED || state == TAPI_SIM_STATUS_SIM_INITIALIZING)
	{
		DEBUG_SERVER_MSG("sim is initialized \n");
	}
	else
	{
		DEBUG_SERVER_MSG("sim is not initialized \n");
		return false;
	}

	return true;
}

static int  sim_callback  (TelTapiEvent_t *sim_event)
{
	DEBUG_SERVER_MSG("[SIM]Reques Id[%d]", sim_event->RequestId);
	DEBUG_SERVER_MSG("[SIM]event state [%d]", sim_event->Status);

	switch(sim_event->EventType)
	{
		case TAPI_EVENT_SIM_APDU_CNF:
		{
			DEBUG_SERVER_MSG("TAPI_EVENT_SIM_APDU_CNF");

			net_nfc_response_send_apdu_t resp = {0};

			uicc_context_t* temp = _get_data(sim_event->RequestId);

			if(temp != NULL)
			{
				resp.trans_param = temp->trans_param;
				_remove_data(temp);
				free(temp);
			}
			else
			{
				resp.trans_param = NULL;
			}

			if(sim_event->Status == TAPI_API_SUCCESS)
			{
				resp.result = NET_NFC_OK;
			}
			else
			{
				resp.result = NET_NFC_OPERATION_FAIL;
			}

			if(sim_event->pData != NULL)
			{

				TelSimApduResp_t* apdu = (TelSimApduResp_t*)sim_event->pData;

				if(apdu->apdu_resp_len > 0)
				{
					resp.data.length = apdu->apdu_resp_len;
					DEBUG_MSG("send response send apdu msg");
					_net_nfc_send_response_msg (NET_NFC_MESSAGE_SEND_APDU_SE, (void*)&resp, sizeof(net_nfc_response_send_apdu_t), apdu->apdu_resp, apdu->apdu_resp_len, NULL);
				}
				else
				{
					DEBUG_MSG("send response send apdu msg");
					_net_nfc_send_response_msg (NET_NFC_MESSAGE_SEND_APDU_SE, (void*)&resp, sizeof(net_nfc_response_send_apdu_t), NULL);
				}
			}
			else
			{
				DEBUG_MSG("send response send apdu msg");
				_net_nfc_send_response_msg (NET_NFC_MESSAGE_SEND_APDU_SE, (void*)&resp, sizeof(net_nfc_response_send_apdu_t), NULL);
			}
		}
		break;

		case TAPI_EVENT_SIM_ATR_CNF:
		{
			DEBUG_SERVER_MSG("TAPI_EVENT_SIM_ATR_CNF");
		}
		break;

		default:
		{
			DEBUG_SERVER_MSG("[SIM]Undhandled event type [%d]", sim_event->EventType);
			DEBUG_SERVER_MSG("[SIM]Undhandled event state [%d]", sim_event->Status);
		}
		break;
	}

	return 0;
}

