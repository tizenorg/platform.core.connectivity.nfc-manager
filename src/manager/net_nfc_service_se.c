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
#include "net_nfc_server_ipc_private.h"
#include "net_nfc_server_dispatcher_private.h"
#include "net_nfc_manager_util_private.h"
#include "net_nfc_service_se_private.h"

#include <pthread.h>
#include <glib.h>
#include <malloc.h>

#include <tapi_common.h>
#include <ITapiSim.h>

/* define */
/* For ESE*/
se_setting_t g_se_setting;

/* For UICC */
struct tapi_handle *uicc_handle = NULL;

static bool net_nfc_service_check_sim_state(void);

static void _uicc_transmit_apdu_cb(TapiHandle *handle, int result, void *data, void *user_data);
static void _uicc_get_atr_cb(TapiHandle *handle, int result, void *data, void *user_data);
static void _uicc_status_noti_cb(TapiHandle *handle, const char *noti_id, void *data, void *user_data);

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
	char **cpList = NULL;

	DEBUG_SERVER_MSG("tapi init");

	cpList = tel_get_cp_name_list();

	uicc_handle = tel_init(cpList[0]);
	if (uicc_handle == NULL)
	{
		int error;

		error = tel_register_noti_event(uicc_handle, TAPI_NOTI_SIM_STATUS, _uicc_status_noti_cb, NULL);
	}
	else
	{
		DEBUG_SERVER_MSG("tel_init() failed");
		return false;
	}

	DEBUG_SERVER_MSG("tel_init() is success");

	return net_nfc_service_check_sim_state();
}

void net_nfc_service_tapi_deinit(void)
{
	DEBUG_SERVER_MSG("deinit tapi");

	tel_deregister_noti_event(uicc_handle, TAPI_NOTI_SIM_STATUS);

	tel_deinit(uicc_handle);
}

bool net_nfc_service_transfer_apdu(data_s *apdu, void *trans_param)
{
	TelSimApdu_t apdu_data = { 0 };
	int result;

	DEBUG_SERVER_MSG("tranfer apdu");

	if (apdu == NULL)
		return false;

	apdu_data.apdu = apdu->buffer;
	apdu_data.apdu_len = apdu->length;

	result = tel_req_sim_apdu(uicc_handle, &apdu_data, _uicc_transmit_apdu_cb, trans_param);
	if (result == 0)
	{
		DEBUG_SERVER_MSG("sim apdu request is success");
	}
	else
	{
		DEBUG_SERVER_MSG("request sim apdu is failed with error = [%d]", result);
		return false;
	}

	return true;
}

bool net_nfc_service_request_atr(void *trans_param)
{
	int result;

	result = tel_req_sim_atr(uicc_handle, _uicc_get_atr_cb, trans_param);
	if(result == 0)
	{
		DEBUG_SERVER_MSG("request is success");
	}
	else
	{
		DEBUG_SERVER_MSG("failed to request ATR  = [%d]", result);
		return false;
	}

	return true;
}

static bool net_nfc_service_check_sim_state(void)
{
	TelSimCardStatus_t state = (TelSimCardStatus_t)0;
	int b_card_changed = 0;
	int error;

	DEBUG_SERVER_MSG("check sim state");

	error = tel_get_sim_init_info(uicc_handle, &state, &b_card_changed);

	DEBUG_SERVER_MSG("current sim init state = [%d]", state);

	if (error != 0)
	{
		DEBUG_SERVER_MSG("error = [%d]", error);
		return false;
	}
	else if (state == TAPI_SIM_STATUS_SIM_INIT_COMPLETED || state == TAPI_SIM_STATUS_SIM_INITIALIZING)
	{
		DEBUG_SERVER_MSG("sim is initialized");
	}
	else
	{
		DEBUG_SERVER_MSG("sim is not initialized");
		return false;
	}

	return true;
}

void _uicc_transmit_apdu_cb(TapiHandle *handle, int result, void *data, void *user_data)
{
	TelSimApduResp_t *apdu = (TelSimApduResp_t *)data;
	net_nfc_response_send_apdu_t resp = { 0 };

	DEBUG_SERVER_MSG("_uicc_transmit_apdu_cb");

	if (result == 0)
	{
		resp.result = NET_NFC_OK;
	}
	else
	{
		resp.result = NET_NFC_OPERATION_FAIL;
	}

	if (apdu != NULL && apdu->apdu_resp_len > 0)
	{
		resp.data.length = apdu->apdu_resp_len;

		DEBUG_MSG("send response send apdu msg");
		_net_nfc_send_response_msg(NET_NFC_MESSAGE_SEND_APDU_SE, (void *)&resp, sizeof(net_nfc_response_send_apdu_t), apdu->apdu_resp, apdu->apdu_resp_len, NULL);
	}
	else
	{
		DEBUG_MSG("send response send apdu msg");
		_net_nfc_send_response_msg(NET_NFC_MESSAGE_SEND_APDU_SE, (void *)&resp, sizeof(net_nfc_response_send_apdu_t), NULL);
	}
}

void _uicc_get_atr_cb(TapiHandle *handle, int result, void *data, void *user_data)
{
	/* TODO : response message */

	DEBUG_SERVER_MSG("_uicc_get_atr_cb");
}

void _uicc_status_noti_cb(TapiHandle *handle, const char *noti_id, void *data, void *user_data)
{
	TelSimCardStatus_t *status = (TelSimCardStatus_t *)data;

	/* TODO :  */
	DEBUG_SERVER_MSG("_uicc_status_noti_cb");

	switch (*status)
	{
	case TAPI_SIM_STATUS_SIM_INIT_COMPLETED :
		DEBUG_SERVER_MSG("TAPI_SIM_STATUS_SIM_INIT_COMPLETED");
		break;

	case TAPI_SIM_STATUS_CARD_REMOVED :
		DEBUG_SERVER_MSG("TAPI_SIM_STATUS_CARD_REMOVED");
		break;

	default :
		DEBUG_SERVER_MSG("unknown status [%d]", *status);
		break;
	}
}
