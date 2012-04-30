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

#include "net_nfc_controller_private.h"
#include "net_nfc_util_private.h"
#include "net_nfc_typedef.h"
#include "net_nfc_debug_private.h"
#include "net_nfc_service_private.h"
#include "net_nfc_service_se_private.h"
#include "net_nfc_app_util_private.h"
#include "net_nfc_dbus_service_obj_private.h"
#include "net_nfc_server_ipc_private.h"
#include "net_nfc_server_dispatcher_private.h"
#include "net_nfc_manager_util_private.h"

#include "net_nfc_service_tag_private.h"
#include "net_nfc_service_llcp_private.h"

#include "net_nfc_util_ndef_message.h"
#include "net_nfc_util_ndef_record.h"

#include <pthread.h>
#include <malloc.h>


/* static variable */

/* static callback function */

/* static function */

extern uint8_t g_se_cur_type;
extern uint8_t g_se_cur_mode;

static void _net_nfc_service_show_exception_msg(char* msg);

static bool _net_nfc_service_check_internal_ese_detected()
{

	if(g_se_cur_type == SECURE_ELEMENT_TYPE_ESE && g_se_cur_mode == SECURE_ELEMENT_WIRED_MODE)
	{
		return true;
	}

	return false;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void net_nfc_service_target_detected_cb(void *info, void *user_context)
{
	int client_context = 0;
	net_nfc_request_msg_t *req_msg = (net_nfc_request_msg_t *)info;

	if (info == NULL)
		return;

	net_nfc_server_set_tag_info(info);

	if (req_msg->request_type == NET_NFC_MESSAGE_SERVICE_RESTART_POLLING_LOOP)
	{
		net_nfc_dispatcher_queue_push(req_msg);
	}
#ifdef BROADCAST_MESSAGE
	net_nfc_request_target_detected_t *detail = (net_nfc_request_target_detected_t *)req_msg;
	req_msg->request_type = NET_NFC_MESSAGE_SERVICE_SLAVE_TARGET_DETECTED;
	net_nfc_dispatcher_queue_push(req_msg);
#else
	else if (net_nfc_server_get_current_client_context(&client_context) == true &&
		net_nfc_server_check_client_is_running(&client_context) == true)
	{
		net_nfc_request_target_detected_t *detail = (net_nfc_request_target_detected_t *)req_msg;

		if (!_net_nfc_service_check_internal_ese_detected())
		{
			req_msg->request_type = NET_NFC_MESSAGE_SERVICE_SLAVE_TARGET_DETECTED;
		}
		else
		{
			req_msg->request_type = NET_NFC_MESSAGE_SERVICE_SLAVE_ESE_DETECTED;

			(detail->handle)->connection_type = NET_NFC_SE_CONNECTION;
		}

		net_nfc_server_set_current_client_target_handle(client_context, detail->handle);

		net_nfc_dispatcher_queue_push(req_msg);

		DEBUG_SERVER_MSG("current client is listener. stand alone mode will be activated");
	}
	else
	{
		req_msg->request_type = NET_NFC_MESSAGE_SERVICE_STANDALONE_TARGET_DETECTED;
		net_nfc_dispatcher_queue_push(req_msg);
	}
#endif
}

void net_nfc_service_se_transaction_cb(void* info, void* user_context)
{
	int client_context = 0;
	bool success;

	net_nfc_request_se_event_t* se_event = (net_nfc_request_se_event_t *)info;

	DEBUG_SERVER_MSG("se event [%d]", se_event->request_type);

	if(net_nfc_server_get_current_client_context(&client_context) == true)
	{
		if(net_nfc_server_check_client_is_running(&client_context) == true)
		{
			net_nfc_response_se_event_t resp_msg  = {0,};

			resp_msg.aid.length = se_event->aid.length;
			resp_msg.param.length = se_event->param.length;

			if(resp_msg.aid.length > 0)
			{
				
				if(resp_msg.param.length > 0)
				{
					success = _net_nfc_send_response_msg (se_event->request_type, (void *)&resp_msg,  sizeof (net_nfc_response_se_event_t),
							(void *)(se_event->aid.buffer), resp_msg.aid.length,
							(void *)(se_event->param.buffer), resp_msg.param.length, NULL);
				}
				else
				{
					success = _net_nfc_send_response_msg (se_event->request_type, (void *)&resp_msg,  sizeof (net_nfc_response_se_event_t),
								(void *)(se_event->aid.buffer), resp_msg.aid.length, NULL);
				}
			}
			else
			{
				success = _net_nfc_send_response_msg (se_event->request_type, (void *)&resp_msg,  sizeof (net_nfc_response_se_event_t), NULL);
			}

			if(success == true)
			{
				DEBUG_SERVER_MSG("sending response is ok");
			}
		}
	}
}

void net_nfc_service_llcp_event_cb(void* info, void* user_context)
{
	net_nfc_request_llcp_msg_t *req_msg = (net_nfc_request_llcp_msg_t *)info;

	if(req_msg == NULL)
	{
		DEBUG_SERVER_MSG("req msg is null");
		return;
	}

	if (req_msg->request_type == NET_NFC_MESSAGE_SERVICE_LLCP_ACCEPT)
	{
		net_nfc_request_accept_socket_t *detail = (net_nfc_request_accept_socket_t *)req_msg;
		detail->trans_param = user_context;
	}

	req_msg->user_param = (uint32_t)user_context;

	net_nfc_dispatcher_queue_push((net_nfc_request_msg_t *)req_msg);
}

bool net_nfc_service_standalone_mode_target_detected(net_nfc_request_msg_t* msg)
{
	net_nfc_request_target_detected_t* stand_alone = (net_nfc_request_target_detected_t *)msg;
	net_nfc_error_e error_status = NET_NFC_OK;

	data_s *recv_data = NULL;

	DEBUG_SERVER_MSG("*** Detected! type [0x%X(%d)] ***", stand_alone->devType);

	switch(stand_alone->devType)
	{
		case NET_NFC_NFCIP1_TARGET :
		case NET_NFC_NFCIP1_INITIATOR :
		{
			DEBUG_SERVER_MSG(" LLCP is detected");

			net_nfc_service_llcp_process(stand_alone->handle, stand_alone->devType, &error_status);

			malloc_trim(0);
			return true;
		}
		break;

		default:
		{
			DEBUG_SERVER_MSG(" PICC or PCD is detectd.");
			recv_data = net_nfc_service_tag_process(stand_alone->handle, stand_alone->devType, &error_status);
		}
		break;
	}

	if(recv_data != NULL)
	{
		net_nfc_util_play_target_detect_sound();
		net_nfc_service_msg_processing(recv_data);
	}
	else
	{
		if(((stand_alone->devType == NET_NFC_NFCIP1_INITIATOR) || (stand_alone->devType == NET_NFC_NFCIP1_TARGET)))
		{
			DEBUG_SERVER_MSG("p2p operation. recv data is NULL");
		}
		else
		{
			net_nfc_util_play_target_detect_sound();

			if(error_status == NET_NFC_NO_NDEF_SUPPORT)
			{
				DEBUG_SERVER_MSG("device type = [%d], it has null data", stand_alone->devType);

				/* 1. make empty type record */
				ndef_message_s *msg = NULL;
				ndef_record_s *record = NULL;
				data_s typeName;
				data_s id;
				data_s payload;
				data_s rawdata;
				int ndef_length = 0;

				if (net_nfc_util_create_ndef_message(&msg) != NET_NFC_OK)
					return true;

				DEBUG_SERVER_MSG("NET_NFC_NO_NDEF_SUPPORT #1");

				memset(&typeName, 0x00, sizeof(data_s));
				memset(&id, 0x00, sizeof(data_s));
				memset(&payload, 0x00, sizeof(data_s));

				if (net_nfc_util_create_record(NET_NFC_RECORD_EMPTY, &typeName, &id, &payload, &record) != NET_NFC_OK)
					return true;

				DEBUG_SERVER_MSG("NET_NFC_NO_NDEF_SUPPORT #2");

				if (net_nfc_util_append_record(msg, record) != NET_NFC_OK)
					return true;

				DEBUG_SERVER_MSG("NET_NFC_NO_NDEF_SUPPORT #3");

				memset(&rawdata, 0x00, sizeof(data_s));
				ndef_length = net_nfc_util_get_ndef_message_length(msg);
				_net_nfc_util_alloc_mem(rawdata.buffer, ndef_length);
				rawdata.length = ndef_length;

				if (net_nfc_util_convert_ndef_message_to_rawdata(msg, &rawdata) != NET_NFC_OK)
					return true;

				DEBUG_SERVER_MSG("NET_NFC_NO_NDEF_SUPPORT #4");

				/* 2. With this, process service routine */
				net_nfc_service_msg_processing(&rawdata);
			}
			else
			{
				_net_nfc_service_show_exception_msg("Try again");
			}
		}
	}

	if(stand_alone->devType != NET_NFC_NFCIP1_INITIATOR && stand_alone->devType != NET_NFC_NFCIP1_TARGET)
	{
		net_nfc_request_watch_dog_t* watch_dog_msg = NULL;

		_net_nfc_util_alloc_mem(watch_dog_msg, sizeof(net_nfc_request_watch_dog_t));

		if(watch_dog_msg != NULL)
		{
			watch_dog_msg->length = sizeof(net_nfc_request_watch_dog_t);
			watch_dog_msg->request_type = NET_NFC_MESSAGE_SERVICE_WATCH_DOG;
			watch_dog_msg->devType = stand_alone->devType;
			watch_dog_msg->handle = stand_alone->handle;

			net_nfc_dispatcher_queue_push((net_nfc_request_msg_t *)watch_dog_msg);
		}

	}

	DEBUG_SERVER_MSG("stand alone mode is end");
	malloc_trim(0);

	return true;
}

bool net_nfc_service_slave_mode_target_detected(net_nfc_request_msg_t* msg)
{
	DEBUG_SERVER_MSG("target detected callback for client");

	net_nfc_error_e result = NET_NFC_OK;
	int request_type = NET_NFC_MESSAGE_TAG_DISCOVERED;
	net_nfc_response_tag_discovered_t resp_msg  = {0,};
	bool success = true;
	data_s* recv_data = NULL;

	memset(&resp_msg, 0x00, sizeof(net_nfc_response_tag_discovered_t));

	net_nfc_request_target_detected_t *detail_msg = (net_nfc_request_target_detected_t *)msg;

	if(detail_msg == NULL)
	{
		return false;
	}

	if(detail_msg->devType != NET_NFC_NFCIP1_TARGET  && detail_msg->devType != NET_NFC_NFCIP1_INITIATOR)
	{
		if (!net_nfc_controller_connect (detail_msg->handle, &result))
		{
			DEBUG_SERVER_MSG("connect failed");
			return false;
		}

#ifdef BROADCAST_MESSAGE
		net_nfc_server_set_server_state(	NET_NFC_TAG_CONNECTED);
#endif
		DEBUG_SERVER_MSG("tag is connected");

		uint8_t ndef_card_state = 0;
		int max_data_size = 0;
		int real_data_size = 0;

		if(net_nfc_controller_check_ndef(detail_msg->handle, &ndef_card_state, &max_data_size, &real_data_size, &result) == true)
		{
			resp_msg.ndefCardState = ndef_card_state;
			resp_msg.maxDataSize = max_data_size;
			resp_msg.actualDataSize = real_data_size;
			resp_msg.is_ndef_supported = 1;
		}
		else
		{
			resp_msg.ndefCardState = 0;
			resp_msg.is_ndef_supported = 0;
			resp_msg.maxDataSize = 0;
			resp_msg.actualDataSize = 0;
		}

		resp_msg.devType = detail_msg->devType;
		resp_msg.handle = detail_msg->handle;
		resp_msg.number_of_keys = detail_msg->number_of_keys;

		net_nfc_util_duplicate_data(&resp_msg.target_info_values, &detail_msg->target_info_values);

		if(resp_msg.is_ndef_supported)
		{
			if(net_nfc_controller_read_ndef (detail_msg->handle, &recv_data, &(resp_msg.result)) == true) {
				DEBUG_SERVER_MSG("net_nfc_controller_read_ndef is success");

				resp_msg.raw_data.length = recv_data->length;

				success = _net_nfc_send_response_msg (request_type, (void *)&resp_msg,  sizeof (net_nfc_response_tag_discovered_t),
					(void *)(resp_msg.target_info_values.buffer), resp_msg.target_info_values.length,
					(void *)(recv_data->buffer), recv_data->length, NULL);

#ifdef BROADCAST_MESSAGE
				net_nfc_util_play_target_detect_sound();
				net_nfc_service_msg_processing(recv_data);
#endif
			}
			else {
				DEBUG_SERVER_MSG("net_nfc_controller_read_ndef is fail");

				resp_msg.raw_data.length = 0;

				success = _net_nfc_send_response_msg (request_type, (void *)&resp_msg,  sizeof (net_nfc_response_tag_discovered_t),
					(void *)(resp_msg.target_info_values.buffer), resp_msg.target_info_values.length, NULL);
			}
		}
		else
		{
#ifdef BROADCAST_MESSAGE
			DEBUG_SERVER_MSG("device type = [%d], it has null data", detail_msg->devType);

			/* 1. make empty type record */
			ndef_message_s *msg = NULL;
			ndef_record_s *record = NULL;
			data_s typeName;
			data_s id;
			data_s payload;
			data_s rawdata;
			int ndef_length = 0;

			if (net_nfc_util_create_ndef_message(&msg) != NET_NFC_OK)
				return true;

			DEBUG_SERVER_MSG("NET_NFC_NO_NDEF_SUPPORT #1");

			memset(&typeName, 0x00, sizeof(data_s));
			memset(&id, 0x00, sizeof(data_s));
			memset(&payload, 0x00, sizeof(data_s));

			if (net_nfc_util_create_record(NET_NFC_RECORD_EMPTY, &typeName, &id, &payload, &record) != NET_NFC_OK)
				return true;

			DEBUG_SERVER_MSG("NET_NFC_NO_NDEF_SUPPORT #2");

			if (net_nfc_util_append_record(msg, record) != NET_NFC_OK)
				return true;

			DEBUG_SERVER_MSG("NET_NFC_NO_NDEF_SUPPORT #3");

			memset(&rawdata, 0x00, sizeof(data_s));
			ndef_length = net_nfc_util_get_ndef_message_length(msg);
			_net_nfc_util_alloc_mem(rawdata.buffer, ndef_length);
			rawdata.length = ndef_length;

			if (net_nfc_util_convert_ndef_message_to_rawdata(msg, &rawdata) != NET_NFC_OK)
				return true;

			DEBUG_SERVER_MSG("NET_NFC_NO_NDEF_SUPPORT #4");

			/* 2. With this, process service routine */
			net_nfc_service_msg_processing(&rawdata);
#endif
			resp_msg.raw_data.length = 0;
			success = _net_nfc_send_response_msg (request_type, (void *)&resp_msg,  sizeof (net_nfc_response_tag_discovered_t),
				(void *)(resp_msg.target_info_values.buffer), resp_msg.target_info_values.length, NULL);
		}

		net_nfc_util_free_data(&resp_msg.target_info_values);

		DEBUG_SERVER_MSG("turn on watch dog");

		net_nfc_request_watch_dog_t* watch_dog_msg = NULL;

		_net_nfc_util_alloc_mem(watch_dog_msg, sizeof(net_nfc_request_watch_dog_t));
		if(watch_dog_msg != NULL)
		{
			watch_dog_msg->length = sizeof(net_nfc_request_watch_dog_t);
			watch_dog_msg->request_type = NET_NFC_MESSAGE_SERVICE_WATCH_DOG;
			watch_dog_msg->devType = detail_msg->devType;
			watch_dog_msg->handle = detail_msg->handle;

			net_nfc_dispatcher_queue_push((net_nfc_request_msg_t *)watch_dog_msg);
		}
	}
	else /* LLCP */
	{
		net_nfc_error_e error_status = NET_NFC_OK;

		net_nfc_service_llcp_process(detail_msg->handle, detail_msg->devType, &error_status);
	}

	return success;
}

bool net_nfc_service_termination (net_nfc_request_msg_t* msg)
{
	net_nfc_error_e result;

	if (net_nfc_controller_is_ready (&result) == true)
	{
	}
	else
	{
		DEBUG_SERVER_MSG("Not initialized");
		net_nfc_controller_init (&result);
	}


	if (net_nfc_controller_confiure_discovery(NET_NFC_DISCOVERY_MODE_CONFIG, NET_NFC_ALL_DISABLE, &result) != true)
	{
		DEBUG_SERVER_MSG("failed to discover off %d", result);
	}

	if (net_nfc_controller_set_secure_element_mode (NET_NFC_SE_CMD_UICC_ON, SECURE_ELEMENT_VIRTUAL_MODE, &result) != true)
	{
		DEBUG_SERVER_MSG("failed to set se mode to default mode: %d", result);
	}

	return true;
}

void net_nfc_service_msg_processing(data_s* data)
{
	if(data != NULL)
	{
		net_nfc_app_util_process_ndef(data);
	}
	else
	{
		_net_nfc_service_show_exception_msg("unknown type tag");
	}
}


static void _net_nfc_service_show_exception_msg(char* msg)
{
	bundle* kb = NULL;
	kb = bundle_create();
	bundle_add(kb, "type", "default");
	bundle_add(kb, "text", msg);

	net_nfc_app_util_aul_launch_app("com.samsung.nfc-app", kb); /* empty_tag */

	bundle_free(kb);
}
