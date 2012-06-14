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
#include <string.h>
#include <pthread.h>
#include <stdarg.h>
#include <glib.h>
#include "vconf.h"

#include "net_nfc_server_dispatcher_private.h"
#include "net_nfc_typedef_private.h"
#include "net_nfc_controller_private.h"
#include "net_nfc_server_ipc_private.h"
#include "net_nfc_debug_private.h"
#include "net_nfc_util_private.h"
#include "net_nfc_util_defines.h"
#include "net_nfc_controller_private.h"
#include "net_nfc_service_private.h"
#include "net_nfc_service_llcp_private.h"
#include "net_nfc_service_llcp_handover_private.h"
#include "net_nfc_service_tag_private.h"
#include "net_nfc_dbus_service_obj_private.h"
#include "net_nfc_manager_util_private.h"
#include "net_nfc_service_se_private.h"

static GQueue *g_dispatcher_queue;
static pthread_cond_t g_dispatcher_queue_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t g_dispatcher_queue_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_t g_dispatcher_thread;

extern se_setting_t g_se_setting;

uint8_t g_se_cur_type = SECURE_ELEMENT_TYPE_INVALD;
uint8_t g_se_cur_mode = SECURE_ELEMENT_OFF_MODE;
static uint8_t g_se_prev_type = SECURE_ELEMENT_TYPE_INVALD;
static uint8_t g_se_prev_mode = SECURE_ELEMENT_OFF_MODE;

static void *_net_nfc_dispatcher_thread_func(void *data);
static net_nfc_request_msg_t *_net_nfc_dispatcher_queue_pop();


static net_nfc_request_msg_t *_net_nfc_dispatcher_queue_pop()
{
	net_nfc_request_msg_t *msg = NULL;
	msg = g_queue_pop_head (g_dispatcher_queue);
	return msg;
}

void net_nfc_dispatcher_queue_push(net_nfc_request_msg_t *req_msg)
{
	pthread_mutex_lock(&g_dispatcher_queue_lock);
	 g_queue_push_tail (g_dispatcher_queue, req_msg);
	pthread_cond_signal(&g_dispatcher_queue_cond);
	pthread_mutex_unlock(&g_dispatcher_queue_lock);
}

bool net_nfc_dispatcher_start_thread()
{
	net_nfc_request_msg_t *req_msg = NULL;
	pthread_attr_t attr;

	DEBUG_SERVER_MSG("init queue");

	g_dispatcher_queue = g_queue_new();

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	_net_nfc_util_alloc_mem(req_msg, sizeof(net_nfc_request_msg_t));
	if (req_msg == NULL)
		return false;

	req_msg->length = sizeof(net_nfc_request_msg_t);
	req_msg->request_type = NET_NFC_MESSAGE_SERVICE_INIT;

	net_nfc_dispatcher_queue_push(req_msg);

	if(pthread_create(&g_dispatcher_thread, &attr, _net_nfc_dispatcher_thread_func, NULL) != 0)
	{
		return false;
	}
	else
	{
		DEBUG_SERVER_MSG("put controller init request");
		return true;
	}
}


static void *_net_nfc_dispatcher_thread_func(void *data)
{
	net_nfc_request_msg_t *req_msg = NULL;

	DEBUG_SERVER_MSG("net_nfc_controller_thread is created ");

	while(1)
	{
		pthread_mutex_lock(&g_dispatcher_queue_lock);
		if((req_msg = _net_nfc_dispatcher_queue_pop()) == NULL)
		{
			pthread_cond_wait(&g_dispatcher_queue_cond, &g_dispatcher_queue_lock);
			pthread_mutex_unlock(&g_dispatcher_queue_lock);
			continue;
		}
		pthread_mutex_unlock(&g_dispatcher_queue_lock);

		//DEBUG_SERVER_MSG("net_nfc_controller get command = [%d]", req_msg->request_type);

		switch(req_msg->request_type)
		{
		case NET_NFC_MESSAGE_SERVICE_CLEANER:
			{
				DEBUG_SERVER_MSG("client is terminated abnormally");

				if(g_se_prev_type == SECURE_ELEMENT_TYPE_ESE)
				{
					net_nfc_error_e result = NET_NFC_OK;
					net_nfc_target_handle_s *ese_handle = g_se_setting.current_ese_handle;

					if(ese_handle != NULL)
					{

						DEBUG_SERVER_MSG("ese_handle was not freed and disconnected");

						if(net_nfc_controller_disconnect(ese_handle, &result) == false)
						{
							net_nfc_controller_exception_handler();
						}
#ifdef BROADCAST_MESSAGE
						net_nfc_server_set_server_state(	NET_NFC_SERVER_IDLE);
#endif
						g_se_setting.current_ese_handle = NULL;
					}

					if((g_se_prev_type != g_se_cur_type) || (g_se_prev_mode != g_se_cur_mode))
					{
						net_nfc_controller_set_secure_element_mode(g_se_prev_type , g_se_prev_mode, &result);

						g_se_cur_type = g_se_prev_type;
						g_se_cur_mode = g_se_prev_mode;
					}
				}
				else if(g_se_prev_type == SECURE_ELEMENT_TYPE_UICC)
				{
					net_nfc_service_tapi_deinit();
				}
				else
				{
					DEBUG_SERVER_MSG("SE type is not valid");
				}
			}
			break;

		case NET_NFC_MESSAGE_SEND_APDU_SE:
			{
				net_nfc_request_send_apdu_t *detail = (net_nfc_request_send_apdu_t *)req_msg;

				if (detail->handle == (net_nfc_target_handle_s *)UICC_TARGET_HANDLE)
				{
					data_s apdu_data = {NULL, 0};

					if (net_nfc_util_duplicate_data(&apdu_data, &detail->data) == false)
						break;

					net_nfc_service_transfer_apdu(&apdu_data, detail->trans_param);

					net_nfc_util_free_data(&apdu_data);
				}
				else if (detail->handle == g_se_setting.current_ese_handle)
				{
					data_s *data = NULL;
					net_nfc_error_e result = NET_NFC_OK;
					net_nfc_transceive_info_s info;
					bool success = true;

					info.dev_type = NET_NFC_ISO14443_A_PICC;
					if (net_nfc_util_duplicate_data(&info.trans_data, &detail->data) == false)
						break;

					if ((success = net_nfc_controller_transceive(detail->handle, &info, &data, &result)) == true)
					{
						if (data != NULL)
						{
							DEBUG_SERVER_MSG("trasceive data recieved [%d], Success = %d", data->length, success);
						}
					}
					else
					{
						DEBUG_SERVER_MSG("trasceive is failed = [%d]", result);
					}
					net_nfc_util_free_data(&info.trans_data);

					net_nfc_response_send_apdu_t resp = { 0 };

					resp.result = result;
					resp.trans_param = detail->trans_param;

					if (success && data != NULL)
					{
						resp.data.length = data->length;

						DEBUG_MSG("send response send apdu msg");
						_net_nfc_send_response_msg(req_msg->request_type, (void *)&resp, sizeof(net_nfc_response_send_apdu_t),
							data->buffer, data->length, NULL);
					}
					else
					{
						DEBUG_MSG("send response send apdu msg");
						_net_nfc_send_response_msg(req_msg->request_type, (void *)&resp, sizeof(net_nfc_response_send_apdu_t), NULL);
					}
				}
				else
				{
					DEBUG_SERVER_MSG("invalid se handle");

					net_nfc_response_send_apdu_t resp = { 0 };
					resp.result = NET_NFC_INVALID_PARAM;
					resp.trans_param = detail->trans_param;

					_net_nfc_send_response_msg(req_msg->request_type, (void *)&resp, sizeof(net_nfc_response_send_apdu_t), NULL);
				}

			}
			break;

		case NET_NFC_MESSAGE_CLOSE_INTERNAL_SE:
			{
				net_nfc_request_close_internal_se_t *detail = (net_nfc_request_close_internal_se_t *)req_msg;
				net_nfc_response_close_internal_se_t resp = { 0 };

				resp.trans_param = detail->trans_param;
				resp.result = NET_NFC_OK;

				if (detail->handle == (net_nfc_target_handle_s *)UICC_TARGET_HANDLE)
				{
					/*deinit TAPI*/
					DEBUG_SERVER_MSG("UICC is current secure element");
					net_nfc_service_tapi_deinit();

				}
				else if (detail->handle == g_se_setting.current_ese_handle)
				{

					if (net_nfc_controller_disconnect(detail->handle, &(resp.result)) == false)
					{
						net_nfc_controller_exception_handler();
					}

#ifdef BROADCAST_MESSAGE
					net_nfc_server_set_server_state(NET_NFC_SERVER_IDLE);
#endif
					g_se_setting.current_ese_handle = NULL;
				}
				else
				{
					DEBUG_SERVER_MSG("invalid se handle received handle = [0x%x] and current handle = [0x%x]", detail->handle, g_se_setting.current_ese_handle);
				}

				if ((g_se_prev_type != g_se_cur_type) || (g_se_prev_mode != g_se_cur_mode))
				{
					/*return back se mode*/
					net_nfc_controller_set_secure_element_mode(g_se_prev_type, g_se_prev_mode, &(resp.result));

					g_se_cur_type = g_se_prev_type;
					g_se_cur_mode = g_se_prev_mode;
				}

				_net_nfc_send_response_msg(req_msg->request_type, (void *)&resp, sizeof(net_nfc_response_close_internal_se_t), NULL);
			}
			break;

		case NET_NFC_MESSAGE_OPEN_INTERNAL_SE:
			{
				net_nfc_error_e result = NET_NFC_OK;
				net_nfc_request_open_internal_se_t *detail = (net_nfc_request_open_internal_se_t *)req_msg;

				g_se_prev_type = g_se_cur_type;
				g_se_prev_mode = g_se_cur_mode;

				if (detail->se_type == SECURE_ELEMENT_TYPE_UICC)
				{
					/*off ESE*/
					net_nfc_controller_set_secure_element_mode(SECURE_ELEMENT_TYPE_ESE, SECURE_ELEMENT_OFF_MODE, &result);

					/*Off UICC. UICC SHOULD not be detected by external reader when being communicated in internal process*/
					net_nfc_controller_set_secure_element_mode(SECURE_ELEMENT_TYPE_UICC, SECURE_ELEMENT_OFF_MODE, &result);

					g_se_cur_type = SECURE_ELEMENT_TYPE_UICC;
					g_se_cur_mode = SECURE_ELEMENT_OFF_MODE;

					net_nfc_response_open_internal_se_t resp = { 0 };

					/*Init tapi api and return back response*/
					if (net_nfc_service_tapi_init() != true)
					{
						net_nfc_service_tapi_deinit();
						resp.result = NET_NFC_INVALID_STATE;
					}
					else
					{
						resp.result = NET_NFC_OK;
					}

					resp.handle = (net_nfc_target_handle_s *)UICC_TARGET_HANDLE;
					resp.trans_param = detail->trans_param;
					resp.se_type = SECURE_ELEMENT_TYPE_UICC;

					_net_nfc_send_response_msg(req_msg->request_type, (void *)&resp, sizeof(net_nfc_response_set_se_t), NULL);
				}
				else
				{
					/*Connect NFC-WI to ESE*/
					net_nfc_controller_set_secure_element_mode(SECURE_ELEMENT_TYPE_ESE, SECURE_ELEMENT_WIRED_MODE, &result);

					/*off UICC*/
					net_nfc_controller_set_secure_element_mode(SECURE_ELEMENT_TYPE_UICC, SECURE_ELEMENT_OFF_MODE, &result);

					g_se_cur_type = SECURE_ELEMENT_TYPE_ESE;
					g_se_cur_mode = SECURE_ELEMENT_WIRED_MODE;
					g_se_setting.open_request_trans_param = detail->trans_param;

					if (result != NET_NFC_OK)
					{
						net_nfc_response_open_internal_se_t resp = { 0 };
						resp.result = result;
						_net_nfc_send_response_msg(req_msg->request_type, (void *)&resp, sizeof(net_nfc_response_set_se_t), NULL);
					}
				}
			}
			break;

		case NET_NFC_MESSAGE_SET_SE:
			{
				net_nfc_request_set_se_t *detail = (net_nfc_request_set_se_t *)req_msg;
				net_nfc_response_set_se_t resp = { 0 };

				resp.trans_param = detail->trans_param;
				resp.se_type = detail->se_type;

				if (detail->se_type == SECURE_ELEMENT_TYPE_UICC)
				{
					g_se_cur_type = SECURE_ELEMENT_TYPE_UICC;
					g_se_cur_mode = SECURE_ELEMENT_VIRTUAL_MODE;

					/*turn on UICC*/
					net_nfc_controller_set_secure_element_mode(SECURE_ELEMENT_TYPE_UICC, SECURE_ELEMENT_VIRTUAL_MODE, &(resp.result));

					/*turn off ESE*/
					net_nfc_controller_set_secure_element_mode(SECURE_ELEMENT_TYPE_ESE, SECURE_ELEMENT_OFF_MODE, &(resp.result));

				}
				else if (detail->se_type == SECURE_ELEMENT_TYPE_ESE)
				{
					g_se_cur_type = SECURE_ELEMENT_TYPE_ESE;
					g_se_cur_mode = SECURE_ELEMENT_VIRTUAL_MODE;

					/*Turn on UICC*/
					net_nfc_controller_set_secure_element_mode(SECURE_ELEMENT_TYPE_UICC, SECURE_ELEMENT_OFF_MODE, &(resp.result));

					/*turn off ESE*/
					net_nfc_controller_set_secure_element_mode(SECURE_ELEMENT_TYPE_ESE, SECURE_ELEMENT_VIRTUAL_MODE, &(resp.result));

				}
				else
				{
					g_se_cur_type = SECURE_ELEMENT_TYPE_INVALD;
					g_se_cur_mode = SECURE_ELEMENT_OFF_MODE;

					/*turn on UICC*/
					net_nfc_controller_set_secure_element_mode(SECURE_ELEMENT_TYPE_UICC, SECURE_ELEMENT_OFF_MODE, &(resp.result));

					/*turn off ESE*/
					net_nfc_controller_set_secure_element_mode(SECURE_ELEMENT_TYPE_ESE, SECURE_ELEMENT_OFF_MODE, &(resp.result));

				}

				_net_nfc_send_response_msg(req_msg->request_type, (void *)&resp, sizeof(net_nfc_response_set_se_t), NULL);
			}
			break;

		case NET_NFC_MESSAGE_GET_SE:
			{
				net_nfc_request_get_se_t *detail = (net_nfc_request_get_se_t *)req_msg;
				net_nfc_response_get_se_t resp = { 0 };

				resp.result = NET_NFC_OK;
				resp.trans_param = detail->trans_param;
				resp.se_type = g_se_cur_type;

				_net_nfc_send_response_msg(req_msg->request_type, (void *)&resp, sizeof(net_nfc_response_get_se_t), NULL);
			}
			break;

		case NET_NFC_MESSAGE_P2P_SEND :
			{
				net_nfc_request_p2p_send_t *exchanger = (net_nfc_request_p2p_send_t *)req_msg;

				if (exchanger != NULL)
				{
					if (net_nfc_service_send_exchanger_msg(exchanger) != NET_NFC_OK)
					{
						DEBUG_SERVER_MSG("net_nfc_service_send_exchanger_msg is failed");

						/*send result to client*/
						int client_context = 0;
						if (net_nfc_server_get_current_client_context(&client_context) == true)
						{
							if (net_nfc_server_check_client_is_running(&client_context) == true)
							{
								int client_type = 0;
								if (net_nfc_server_get_client_type(client_context, &client_type) == true)
								{
									net_nfc_response_p2p_send_t resp_msg = { 0, };
									resp_msg.result = NET_NFC_P2P_SEND_FAIL;

									if (_net_nfc_send_response_msg(NET_NFC_MESSAGE_P2P_SEND, &resp_msg, sizeof(net_nfc_response_p2p_send_t), NULL) == true)
									{
										DEBUG_SERVER_MSG("send exchange failed message to client");
									}
								}
							}
						}
					}
				}
			}
			break;

		case NET_NFC_MESSAGE_TRANSCEIVE :
			{
				net_nfc_transceive_info_s info;
				net_nfc_request_transceive_t *trans = (net_nfc_request_transceive_t *)req_msg;
				data_s *data = NULL;
				bool success;

				if (net_nfc_util_duplicate_data(&info.trans_data, &trans->info.trans_data) == true)
				{
					net_nfc_response_transceive_t resp = { 0, };

					info.dev_type = trans->info.dev_type;

					resp.result = NET_NFC_OK;
					resp.trans_param = trans->trans_param;

					DEBUG_MSG("call transceive");
					if ((success = net_nfc_controller_transceive(trans->handle, &info, &data, &(resp.result))) == true)
					{
						if (data != NULL)
							DEBUG_SERVER_MSG("trasceive data recieved [%d], Success = %d", data->length, success);
					}
					else
					{
						DEBUG_SERVER_MSG("trasceive is failed = [%d]", resp.result);
					}

					if (_net_nfc_check_client_handle())
					{
						if (success && data != NULL)
						{
							resp.data.length = data->length;

							DEBUG_MSG("send response trans msg");
							_net_nfc_send_response_msg(req_msg->request_type, (void *)&resp, sizeof(net_nfc_response_transceive_t),
								data->buffer, data->length, NULL);
						}
						else
						{
							DEBUG_MSG("send response trans msg");
							_net_nfc_send_response_msg(req_msg->request_type, (void *)&resp, sizeof(net_nfc_response_transceive_t), NULL);
						}
					}

					net_nfc_util_free_data(&info.trans_data);
				}
			}
			break;

		case NET_NFC_MESSAGE_MAKE_READ_ONLY_NDEF:
			{
				net_nfc_response_make_read_only_ndef_t resp = { 0, };
				net_nfc_request_make_read_only_ndef_t *make_readOnly = (net_nfc_request_make_read_only_ndef_t *)req_msg;

				resp.result = NET_NFC_OK;
				resp.trans_param = make_readOnly->trans_param;

				net_nfc_controller_make_read_only_ndef(make_readOnly->handle, &(resp.result));

				if (_net_nfc_check_client_handle())
				{
					_net_nfc_send_response_msg(req_msg->request_type, (void *)&resp, sizeof(net_nfc_response_write_ndef_t), NULL);
				}
			}
			break;

		case NET_NFC_MESSAGE_IS_TAG_CONNECTED:
			{
				net_nfc_response_is_tag_connected_t resp = { 0, };
				net_nfc_request_is_tag_connected_t *detail = (net_nfc_request_is_tag_connected_t *)req_msg;
				net_nfc_current_target_info_s* target_info = NULL;

				target_info = net_nfc_server_get_tag_info();

				if(target_info != NULL)
				{
					resp.result = NET_NFC_OK;
					resp.devType = target_info->devType;
				}
				else
				{
					resp.result = NET_NFC_NOT_CONNECTED;
					resp.devType = NET_NFC_UNKNOWN_TARGET;
				}

				resp.trans_param = detail->trans_param;

				_net_nfc_send_response_msg(req_msg->request_type, (void *)&resp, sizeof(net_nfc_response_is_tag_connected_t), NULL);
			}
			break;

		case NET_NFC_MESSAGE_GET_CURRENT_TAG_INFO:
			{
				net_nfc_response_get_current_tag_info_t resp = { 0, };
				net_nfc_request_get_current_tag_info_t *detail = (net_nfc_request_get_current_tag_info_t *)req_msg;
				net_nfc_current_target_info_s *target_info = NULL;
				net_nfc_error_e result = NET_NFC_OK;

				resp.trans_param = detail->trans_param;

				target_info = net_nfc_server_get_tag_info();

				if(target_info != NULL)
				{
					bool success = true;
					data_s* recv_data = NULL;

					memset(&resp, 0x00, sizeof(net_nfc_response_get_current_tag_info_t));

					if(target_info->devType != NET_NFC_NFCIP1_TARGET  && target_info->devType != NET_NFC_NFCIP1_INITIATOR)
					{
#ifdef BROADCAST_MESSAGE
						net_nfc_server_set_server_state(	NET_NFC_TAG_CONNECTED);
#endif
						DEBUG_SERVER_MSG("tag is connected");

						uint8_t ndef_card_state = 0;
						int max_data_size = 0;
						int real_data_size = 0;

						if(net_nfc_controller_check_ndef(target_info->handle, &ndef_card_state, &max_data_size, &real_data_size, &result) == true)
						{
							resp.ndefCardState = ndef_card_state;
							resp.maxDataSize = max_data_size;
							resp.actualDataSize = real_data_size;
							resp.is_ndef_supported = 1;
						}
						else
						{
							resp.is_ndef_supported = 0;
							resp.ndefCardState = 0;
							resp.maxDataSize = 0;
							resp.actualDataSize = 0;
						}

						resp.devType = target_info->devType;
						resp.handle = target_info->handle;
						resp.number_of_keys = target_info->number_of_keys;

						net_nfc_util_duplicate_data(&resp.target_info_values, &target_info->target_info_values);

						if(resp.is_ndef_supported)
						{
							if(net_nfc_controller_read_ndef (target_info->handle, &recv_data, &(resp.result)) == true) {
								DEBUG_SERVER_MSG("net_nfc_controller_read_ndef is success");

								resp.raw_data.length = recv_data->length;

								success = _net_nfc_send_response_msg (req_msg->request_type, (void *)&resp,  sizeof (net_nfc_response_get_current_tag_info_t),
									(void *)(resp.target_info_values.buffer), resp.target_info_values.length,
									(void *)(recv_data->buffer), recv_data->length, NULL);
							}
							else {
								DEBUG_SERVER_MSG("net_nfc_controller_read_ndef is fail");

								resp.raw_data.length = 0;

								success = _net_nfc_send_response_msg (req_msg->request_type, (void *)&resp,  sizeof (net_nfc_response_get_current_tag_info_t),
									(void *)(resp.target_info_values.buffer), resp.target_info_values.length, NULL);
							}
						}
						else
						{
							resp.raw_data.length = 0;

							success = _net_nfc_send_response_msg (req_msg->request_type, (void *)&resp,  sizeof (net_nfc_response_get_current_tag_info_t),
								(void *)(resp.target_info_values.buffer), resp.target_info_values.length, NULL);
						}

						net_nfc_util_free_data(&resp.target_info_values);

					}
				}
				else
				{
					resp.result = NET_NFC_NOT_CONNECTED;
					_net_nfc_send_response_msg(req_msg->request_type, (void *)&resp, sizeof(net_nfc_response_get_current_tag_info_t), NULL);


				}
			}
			break;

		case NET_NFC_MESSAGE_GET_CURRENT_TARGET_HANDLE:
			{
				net_nfc_response_get_current_target_handle_t resp = { 0, };
				net_nfc_request_get_current_target_handle_t *detail = (net_nfc_request_get_current_target_handle_t *)req_msg;
				net_nfc_current_target_info_s *target_info = NULL;
				net_nfc_error_e result = NET_NFC_OK;

				resp.trans_param = detail->trans_param;

				target_info = net_nfc_server_get_tag_info();

				if(target_info != NULL)
				{
					resp.handle = target_info->handle;
					resp.devType = target_info->devType;
					resp.result = NET_NFC_OK;

				}
				else
				{
					resp.result = NET_NFC_NOT_CONNECTED;
				}

				_net_nfc_send_response_msg(req_msg->request_type, (void *)&resp, sizeof(net_nfc_response_get_current_target_handle_t), NULL);

			}
			break;

		case NET_NFC_GET_SERVER_STATE:
			{
				net_nfc_response_get_server_state_t resp = { 0, };

				resp.state = net_nfc_get_server_state();
				resp.result = NET_NFC_OK;

				_net_nfc_send_response_msg(req_msg->request_type, (void *)&resp, sizeof(net_nfc_response_get_server_state_t), NULL);
			}
			break;

		case NET_NFC_MESSAGE_READ_NDEF:
			{
				net_nfc_response_read_ndef_t resp = { 0, };
				net_nfc_request_read_ndef_t *read = (net_nfc_request_read_ndef_t*)req_msg;
				data_s *data = NULL;
				bool success;

				resp.trans_param = read->trans_param;
				resp.result = NET_NFC_OK;

				success = net_nfc_controller_read_ndef(read->handle, &data, &(resp.result));

				if (_net_nfc_check_client_handle())
				{
					if (success)
					{
						resp.data.length = data->length;
						_net_nfc_send_response_msg(req_msg->request_type, (void *)&resp, sizeof(net_nfc_response_read_ndef_t),
							data->buffer, data->length, NULL);
					}
					else
					{
						resp.data.length = 0;
						resp.data.buffer = NULL;
						_net_nfc_send_response_msg(req_msg->request_type, (void *)&resp, sizeof(net_nfc_response_read_ndef_t), NULL);
					}
				}
			}
			break;

		case NET_NFC_MESSAGE_WRITE_NDEF:
			{
				net_nfc_request_write_ndef_t *write_ndef = (net_nfc_request_write_ndef_t *)req_msg;
				data_s data = { NULL, 0 };

				if (net_nfc_util_duplicate_data(&data, &write_ndef->data) == true)
				{
					net_nfc_response_write_ndef_t resp = { 0, };

					resp.result = NET_NFC_OK;
					resp.trans_param = write_ndef->trans_param;

					net_nfc_controller_write_ndef(write_ndef->handle, &data, &(resp.result));
					if (_net_nfc_check_client_handle())
					{
						_net_nfc_send_response_msg(req_msg->request_type, (void *)&resp, sizeof(net_nfc_response_write_ndef_t), NULL);
					}

					net_nfc_util_free_data(&data);
				}
			}
			break;

		case NET_NFC_MESSAGE_SIM_TEST:
			{
				net_nfc_response_test_t resp = { 0, };
				net_nfc_request_test_t *sim_test = (net_nfc_request_test_t *)req_msg;
				net_nfc_error_e result = NET_NFC_OK;

				resp.result = NET_NFC_OK;
				resp.trans_param = sim_test->trans_param;

				if (net_nfc_controller_test_mode_off(&result) == true)
				{
					if (net_nfc_controller_test_mode_on(&result) == true)
					{
						if (net_nfc_controller_sim_test(&result) == true)
						{
							DEBUG_SERVER_MSG("net_nfc_controller_sim_test Result [SUCCESS]");
						}
						else
						{
							resp.result = NET_NFC_UNKNOWN_ERROR;
							DEBUG_SERVER_MSG("net_nfc_controller_sim_test Result [ERROR1]");
						}
					}
					else
					{
						resp.result = NET_NFC_UNKNOWN_ERROR;
						DEBUG_SERVER_MSG("net_nfc_controller_sim_test Result [ERROR2]");
					}
				}
				else
				{
					resp.result = NET_NFC_UNKNOWN_ERROR;
					DEBUG_SERVER_MSG("net_nfc_controller_sim_test Result [ERROR3]");
				}

				DEBUG_SERVER_MSG("SEND RESPONSE!!");
				_net_nfc_send_response_msg(req_msg->request_type, (void *)&resp, sizeof(net_nfc_response_test_t), NULL);
			}
			break;

		case NET_NFC_MESSAGE_SERVICE_DEINIT:
			{
				net_nfc_controller_deinit();
			}
			break;

		case NET_NFC_MESSAGE_SERVICE_INIT:
			{
				net_nfc_error_e result;

				if(net_nfc_controller_init(&result) == true)
				{
					if(net_nfc_controller_register_listener(net_nfc_service_target_detected_cb, net_nfc_service_se_transaction_cb, net_nfc_service_llcp_event_cb, &result) == true)
					{
						if(net_nfc_controller_confiure_discovery(NET_NFC_DISCOVERY_MODE_CONFIG, NET_NFC_ALL_ENABLE, &result) == true)
						{
							DEBUG_SERVER_MSG("now, nfc is ready");
						}
						else
						{
							DEBUG_ERR_MSG("net_nfc_controller_confiure_discovery failed [%d]", result);

							if (vconf_set_int(NET_NFC_VCONF_KEY_PROGRESS, result) != 0)
							{
								DEBUG_ERR_MSG("vconf_set_int(NET_NFC_VCONF_KEY_PROGRESS, ERROR) != 0");
							}
							exit(result);
						}
					}
					else
					{
						DEBUG_ERR_MSG("net_nfc_controller_register_listener failed [%d]", result);

						if (vconf_set_int(NET_NFC_VCONF_KEY_PROGRESS, result) != 0)
						{
							DEBUG_ERR_MSG("vconf_set_int(NET_NFC_VCONF_KEY_PROGRESS, ERROR) != 0");
						}
						exit(result);
					}

					net_nfc_llcp_config_info_s config = {128, 1, 100, 0};
					if(net_nfc_controller_llcp_config(&config, &result) == true)
					{
						/*We need to check the stack that supports the llcp or not.*/
						DEBUG_SERVER_MSG("llcp is enabled");
					}
					else
					{
						DEBUG_SERVER_MSG("llcp is disabled")
					}

					DEBUG_SERVER_MSG("set ESE");

					if(net_nfc_controller_set_secure_element_mode(SECURE_ELEMENT_TYPE_ESE, SECURE_ELEMENT_OFF_MODE, &result) == true)
					{
						DEBUG_SERVER_MSG("set ESE is ok");
					}
					else
					{
						DEBUG_SERVER_MSG("failed to set ESE as enable = [%d]", result);
					}

					DEBUG_SERVER_MSG("set UICC");

					if(net_nfc_controller_set_secure_element_mode(SECURE_ELEMENT_TYPE_UICC, SECURE_ELEMENT_OFF_MODE, &result) == true)
					{
						DEBUG_SERVER_MSG("set UICC is ok");
					}
					else
					{
						DEBUG_SERVER_MSG("failed to set UICC as enable = [%d]", result);
					}

					g_se_cur_type = SECURE_ELEMENT_TYPE_UICC;
					g_se_cur_mode = SECURE_ELEMENT_VIRTUAL_MODE;
					g_se_prev_type = SECURE_ELEMENT_TYPE_UICC;
					g_se_prev_mode = SECURE_ELEMENT_VIRTUAL_MODE;

					if (vconf_set_int(NET_NFC_VCONF_KEY_PROGRESS, 0) != 0)
					{
						DEBUG_ERR_MSG("vconf_set_int(NET_NFC_VCONF_KEY_PROGRESS, SUCCESS) != 0");
					}
					else
					{
						DEBUG_SERVER_MSG("update vconf key NET_NFC_VCONF_KEY_PROGRESS [SUCCESS]");
					}
				}
				else
				{
					DEBUG_ERR_MSG("net_nfc_controller_init failed [%d]", result);

					if (vconf_set_int(NET_NFC_VCONF_KEY_PROGRESS, result) != 0)
					{
						DEBUG_ERR_MSG("vconf_set_int(NET_NFC_VCONF_KEY_PROGRESS, ERROR) != 0");
					}
					/*We have to considerate when initialization is failed.*/
					exit(result);
				}
			}
			break;

		case NET_NFC_MESSAGE_SERVICE_STANDALONE_TARGET_DETECTED:
			{
				net_nfc_service_standalone_mode_target_detected(req_msg);
			}
			break;

		case NET_NFC_MESSAGE_SERVICE_RESTART_POLLING_LOOP:
			{
				net_nfc_error_e result = NET_NFC_OK;

				if (net_nfc_controller_confiure_discovery(NET_NFC_DISCOVERY_MODE_RESUME, NET_NFC_ALL_ENABLE, &result) == true)
				{
					DEBUG_SERVER_MSG("now, nfc polling loop is running again");
				}
			}
			break;

		case NET_NFC_MESSAGE_SERVICE_LLCP_ACCEPT:
			{
				net_nfc_service_llcp_process_accept(req_msg);
			}
			break;

		case NET_NFC_MESSAGE_SERVICE_LLCP_DEACTIVATED:
			{
				net_nfc_service_llcp_disconnect_target(req_msg);
			}
			break;

		case NET_NFC_MESSAGE_SERVICE_LLCP_SOCKET_ERROR:
			{
				net_nfc_service_llcp_process_socket_error(req_msg);
			}
			break;

		case NET_NFC_MESSAGE_SERVICE_LLCP_SOCKET_ACCEPTED_ERROR:
			{
				net_nfc_service_llcp_process_accepted_socket_error(req_msg);
			}
			break;

		case NET_NFC_MESSAGE_SERVICE_LLCP_SEND :
			{
				net_nfc_service_llcp_process_send_socket(req_msg);
			}
			break;

		case NET_NFC_MESSAGE_SERVICE_LLCP_SEND_TO :
			{
				net_nfc_service_llcp_process_send_to_socket(req_msg);
			}
			break;

		case NET_NFC_MESSAGE_SERVICE_LLCP_RECEIVE :
			{
				net_nfc_service_llcp_process_receive_socket(req_msg);
			}
			break;

		case NET_NFC_MESSAGE_SERVICE_LLCP_RECEIVE_FROM :
			{
				net_nfc_service_llcp_process_receive_from_socket(req_msg);
			}
			break;

		case NET_NFC_MESSAGE_SERVICE_LLCP_CONNECT :
			{
				net_nfc_service_llcp_process_connect_socket(req_msg);
			}
			break;

		case NET_NFC_MESSAGE_SERVICE_LLCP_CONNECT_SAP :
			{
				net_nfc_service_llcp_process_connect_sap_socket(req_msg);
			}
			break;

		case NET_NFC_MESSAGE_SERVICE_LLCP_DISCONNECT :
			{
				net_nfc_service_llcp_process_disconnect_socket(req_msg);
			}
			break;

		case NET_NFC_MESSAGE_SERVICE_SE :
			{
				net_nfc_request_set_se_t *detail = (net_nfc_request_set_se_t *)req_msg;
				net_nfc_error_e result = NET_NFC_OK;
				int mode;

				if (detail == NULL)
					break;

				mode = (int)detail->se_type;

				if(mode == NET_NFC_SE_CMD_UICC_ON )
				{
					/*turn on UICC*/
					net_nfc_controller_set_secure_element_mode(SECURE_ELEMENT_TYPE_UICC, SECURE_ELEMENT_VIRTUAL_MODE, &result);

					/*turn off ESE*/
					net_nfc_controller_set_secure_element_mode(SECURE_ELEMENT_TYPE_ESE , SECURE_ELEMENT_OFF_MODE, &result);
				}
				else if(mode == NET_NFC_SE_CMD_ESE_ON)
				{
					/*turn off UICC*/
					net_nfc_controller_set_secure_element_mode(SECURE_ELEMENT_TYPE_UICC, SECURE_ELEMENT_OFF_MODE, &result);

					/*turn on ESE*/
					net_nfc_controller_set_secure_element_mode(SECURE_ELEMENT_TYPE_ESE , SECURE_ELEMENT_VIRTUAL_MODE, &result);
				}
				else if(mode == NET_NFC_SE_CMD_ALL_OFF)
				{
					/*turn off both*/
					net_nfc_controller_set_secure_element_mode(SECURE_ELEMENT_TYPE_UICC, SECURE_ELEMENT_OFF_MODE, &result);

					/*turn on ESE*/
					net_nfc_controller_set_secure_element_mode(SECURE_ELEMENT_TYPE_ESE , SECURE_ELEMENT_OFF_MODE, &result);
				}
				else
				{
					/*turn off both*/
					net_nfc_controller_set_secure_element_mode(SECURE_ELEMENT_TYPE_UICC, SECURE_ELEMENT_VIRTUAL_MODE, &result);

					/*turn on ESE*/
					net_nfc_controller_set_secure_element_mode(SECURE_ELEMENT_TYPE_ESE , SECURE_ELEMENT_VIRTUAL_MODE, &result);
				}
			}
			break;

		case NET_NFC_MESSAGE_SERVICE_TERMINATION :
			{
				net_nfc_service_termination(req_msg);
			}
			break;

		case NET_NFC_MESSAGE_SERVICE_SLAVE_TARGET_DETECTED :
			{
				net_nfc_service_slave_mode_target_detected(req_msg);
			}
			break;

		case NET_NFC_MESSAGE_SERVICE_SLAVE_ESE_DETECTED :
			{
				net_nfc_service_se_detected(req_msg);
			}
			break;

		case NET_NFC_MESSAGE_FORMAT_NDEF :
			{
				net_nfc_request_format_ndef_t *detail = (net_nfc_request_format_ndef_t *)req_msg;
				data_s data = { NULL, 0 };

				if (net_nfc_util_duplicate_data(&data, &detail->key) == true)
				{
					net_nfc_response_format_ndef_t resp = { 0 };

					resp.result = NET_NFC_OK;
					resp.trans_param = detail->trans_param;

					net_nfc_controller_format_ndef(detail->handle, &data, &(resp.result));
					if (_net_nfc_check_client_handle())
					{
						_net_nfc_send_response_msg(req_msg->request_type, (void *)&resp, sizeof(net_nfc_response_format_ndef_t), NULL);
					}

					net_nfc_util_free_data(&data);
				}
			}
			break;

		case NET_NFC_MESSAGE_LLCP_LISTEN :
			{
				net_nfc_response_listen_socket_t resp = { 0 };
				net_nfc_request_listen_socket_t *detail = (net_nfc_request_listen_socket_t *)req_msg;
				net_nfc_response_llcp_socket_error_t *error = NULL;
				bool success = false;

				_net_nfc_manager_util_alloc_mem(error, sizeof (net_nfc_response_llcp_socket_error_t));

				if (detail == NULL || error == NULL)
				{
					DEBUG_SERVER_MSG("ERROR: invalid detail info or allocation is failed");
					break;
				}

				resp.result = NET_NFC_IPC_FAIL;
				error->client_socket = detail->client_socket;
				error->handle = detail->handle;

				success = net_nfc_controller_llcp_create_socket(&(detail->oal_socket), detail->type, detail->miu, detail->rw, &(resp.result), error);

				if (success == true)
				{
					error->oal_socket = resp.oal_socket = detail->oal_socket;
					success = net_nfc_controller_llcp_bind(detail->oal_socket, detail->sap, &(resp.result));
				}

				if (success == true)
				{
					DEBUG_SERVER_MSG("OAL socket in Listen :%d", detail->oal_socket);
					success = net_nfc_controller_llcp_listen(detail->handle, detail->service_name.buffer, detail->oal_socket, &(resp.result), error);
				}

				resp.oal_socket = detail->oal_socket;
				resp.client_socket = detail->client_socket;
				resp.trans_param = detail->trans_param;

				if (_net_nfc_check_client_handle())
				{
					_net_nfc_send_response_msg(req_msg->request_type, (void *)&resp, sizeof(net_nfc_response_listen_socket_t), NULL);
				}
			}
			break;

		case NET_NFC_MESSAGE_LLCP_CONNECT :
			{
				net_nfc_response_connect_socket_t *resp = NULL;
				net_nfc_request_connect_socket_t *detail = (net_nfc_request_connect_socket_t *)req_msg;
				net_nfc_response_llcp_socket_error_t *error = NULL;
				bool success = false;

				_net_nfc_manager_util_alloc_mem(error, sizeof (net_nfc_response_llcp_socket_error_t));
				_net_nfc_manager_util_alloc_mem(resp, sizeof (net_nfc_response_connect_socket_t));

				if (detail == NULL || resp == NULL || error == NULL)
				{
					DEBUG_SERVER_MSG("ERROR: invalid detail info or allocation is failed");
					break;
				}

				error->client_socket = detail->client_socket;
				error->handle = detail->handle;
				resp->result = NET_NFC_IPC_FAIL;

				success = net_nfc_controller_llcp_create_socket(&(detail->oal_socket), detail->type, detail->miu, detail->rw, &(resp->result), error);

				if (success == true)
				{

					error->oal_socket = resp->oal_socket = detail->oal_socket;
					DEBUG_SERVER_MSG("connect client socket [%d]", detail->client_socket);
					resp->client_socket = detail->client_socket;
					resp->trans_param = detail->trans_param;

					success = net_nfc_controller_llcp_connect_by_url(detail->handle, detail->oal_socket, detail->service_name.buffer, &(resp->result), resp);
				}

				if (success == false)
				{
					DEBUG_SERVER_MSG("connect client socket is failed");

					if (_net_nfc_check_client_handle())
					{
						_net_nfc_send_response_msg(req_msg->request_type, (void *)resp, sizeof(net_nfc_response_connect_socket_t), NULL);
					}
					_net_nfc_manager_util_free_mem(resp);
				}
			}
			break;

		case NET_NFC_MESSAGE_LLCP_CONNECT_SAP :
			{
				net_nfc_response_connect_sap_socket_t *resp = NULL;
				net_nfc_request_connect_sap_socket_t *detail = (net_nfc_request_connect_sap_socket_t *)req_msg;
				bool success = false;

				_net_nfc_manager_util_alloc_mem(resp, sizeof (net_nfc_response_connect_sap_socket_t));

				if (detail == NULL || resp == NULL)
				{
					DEBUG_SERVER_MSG("ERROR: invalid detail info or allocation is failed");
					break;
				}

				resp->result = NET_NFC_IPC_FAIL;
				success = net_nfc_controller_llcp_create_socket(&(detail->oal_socket), detail->type, detail->miu, detail->rw, &(resp->result), NULL);

				if (success == true)
				{
					resp->oal_socket = detail->oal_socket;
					resp->client_socket = detail->client_socket;
					resp->trans_param = detail->trans_param;

					success = net_nfc_controller_llcp_connect(detail->handle, detail->oal_socket, detail->sap, &(resp->result), resp);
				}

				if (success == false)
				{
					if (_net_nfc_check_client_handle())
					{
						_net_nfc_send_response_msg(req_msg->request_type, (void *)resp, sizeof(net_nfc_response_connect_sap_socket_t), NULL);
					}
					_net_nfc_manager_util_free_mem(resp);
				}
			}
			break;

		case NET_NFC_MESSAGE_LLCP_SEND :
			{
				net_nfc_request_send_socket_t *detail = (net_nfc_request_send_socket_t *)req_msg;
				data_s data = { NULL, 0 };

				if (detail == NULL)
				{
					DEBUG_SERVER_MSG("ERROR: invalid detail info");
					break;
				}

				if (net_nfc_util_duplicate_data(&data, &detail->data) == true)
				{
					net_nfc_response_send_socket_t *resp = NULL;

					_net_nfc_manager_util_alloc_mem(resp, sizeof (net_nfc_response_send_socket_t));
					if (resp == NULL)
					{
						DEBUG_SERVER_MSG("ERROR: allocation is failed");
						net_nfc_util_free_data(&data);
						break;
					}

					resp->result = NET_NFC_IPC_FAIL;
					resp->client_socket = detail->client_socket;
					resp->trans_param = detail->trans_param;

					if (net_nfc_controller_llcp_send(detail->handle, detail->oal_socket, &data, &(resp->result), resp) == false)
					{
						if (_net_nfc_check_client_handle())
						{
							_net_nfc_send_response_msg(req_msg->request_type, (void *)resp, sizeof(net_nfc_response_send_socket_t), NULL);
						}
						_net_nfc_manager_util_free_mem(resp);
					}

					net_nfc_util_free_data(&data);
				}
			}
			break;

		case NET_NFC_MESSAGE_LLCP_RECEIVE :
			{
				net_nfc_response_receive_socket_t *resp = NULL;
				net_nfc_request_receive_socket_t *detail = (net_nfc_request_receive_socket_t *)req_msg;
				bool success = false;

				_net_nfc_manager_util_alloc_mem(resp, sizeof (net_nfc_response_receive_socket_t));

				if (detail == NULL || resp == NULL)
				{
					DEBUG_SERVER_MSG("ERROR: invalid detail info or allocation is failed");
					break;
				}

				resp->result = NET_NFC_IPC_FAIL;
				resp->client_socket = detail->client_socket;
				resp->trans_param = detail->trans_param;
				_net_nfc_manager_util_alloc_mem(resp->data.buffer, detail->req_length);
				resp->data.length = detail->req_length;

				if (resp->data.buffer != NULL)
				{
					success = net_nfc_controller_llcp_recv(detail->handle, detail->oal_socket, &(resp->data), &(resp->result), resp);
				}
				else
				{
					resp->result = NET_NFC_ALLOC_FAIL;
				}

				if (success == false)
				{
					if (_net_nfc_check_client_handle())
					{
						_net_nfc_send_response_msg(req_msg->request_type, (void *)resp, sizeof(net_nfc_response_receive_socket_t), NULL);
					}
					_net_nfc_manager_util_free_mem(resp->data.buffer);
					_net_nfc_manager_util_free_mem(resp);
				}
			}
			break;

		case NET_NFC_MESSAGE_SERVICE_LLCP_CLOSE :
			{
				net_nfc_response_close_socket_t resp = { 0, };
				net_nfc_request_close_socket_t *detail = (net_nfc_request_close_socket_t *)req_msg;

				resp.result = NET_NFC_IPC_FAIL;
				if (detail != NULL)
				{
					net_nfc_controller_llcp_socket_close(detail->oal_socket, &(resp.result));

					resp.client_socket = detail->client_socket;
					resp.trans_param = detail->trans_param;

					if (_net_nfc_check_client_handle())
					{
						_net_nfc_send_response_msg(req_msg->request_type, (void *)&resp, sizeof(net_nfc_response_close_socket_t), NULL);
					}
				}
			}
			break;

		case NET_NFC_MESSAGE_LLCP_DISCONNECT :	/* change resp to local variable. if there is some problem, check this first. */
			{
				net_nfc_response_disconnect_socket_t resp = { 0, };
				net_nfc_request_disconnect_socket_t *detail = (net_nfc_request_disconnect_socket_t *)req_msg;

				resp.result = NET_NFC_IPC_FAIL;
				if (detail != NULL)
				{
					resp.client_socket = detail->client_socket;
					resp.trans_param = detail->trans_param;

					if (false == net_nfc_controller_llcp_disconnect(detail->handle, detail->oal_socket, &(resp.result), &resp))
					{
						if (_net_nfc_check_client_handle())
						{
							_net_nfc_send_response_msg(req_msg->request_type, (void *)&resp, sizeof(net_nfc_response_disconnect_socket_t), NULL);
						}
					}
				}
			}
			break;

		case NET_NFC_MESSAGE_LLCP_ACCEPTED :
			{
				net_nfc_request_accept_socket_t *detail = (net_nfc_request_accept_socket_t *)req_msg;
				net_nfc_response_accept_socket_t resp = { 0, };

				resp.result = NET_NFC_IPC_FAIL;
				if (detail != NULL)
				{
					net_nfc_controller_llcp_accept(detail->incomming_socket, &(resp.result));

					resp.client_socket = detail->client_socket;
					resp.trans_param = detail->trans_param;

					if (_net_nfc_check_client_handle())
					{
						_net_nfc_send_response_msg(req_msg->request_type, (void *)&resp, sizeof(net_nfc_response_accept_socket_t), NULL);
					}
				}
			}
			break;

		case NET_NFC_MESSAGE_LLCP_CONFIG :
			{
				net_nfc_request_config_llcp_t *detail = (net_nfc_request_config_llcp_t *)req_msg;
				net_nfc_response_config_llcp_t resp = { 0, };

				resp.result = NET_NFC_IPC_FAIL;
				if (detail != NULL)
				{
					net_nfc_controller_llcp_config(&(detail->config), &(resp.result));
					resp.trans_param = detail->trans_param;

					if (_net_nfc_check_client_handle())
					{
						_net_nfc_send_response_msg(req_msg->request_type, (void *)&resp, sizeof(net_nfc_response_config_llcp_t), NULL);
					}
				}
			}
			break;

		case NET_NFC_MESSAGE_CONNECTION_HANDOVER :
			{
				net_nfc_request_connection_handover_t *detail = (net_nfc_request_connection_handover_t *)req_msg;
				net_nfc_response_connection_handover_t response = { 0, };

				if (detail != NULL)
				{
					if ((response.result = net_nfc_service_llcp_handover_send_request_msg(detail)) != NET_NFC_OK)
					{
						response.event = NET_NFC_OPERATION_FAIL;
						response.type = NET_NFC_CONN_HANDOVER_CARRIER_UNKNOWN;

						_net_nfc_send_response_msg(req_msg->request_type, (void *)&response, sizeof(net_nfc_response_connection_handover_t), NULL);
					}
				}
			}
			break;

		case NET_NFC_MESSAGE_SERVICE_WATCH_DOG :
			{
				net_nfc_service_watch_dog(req_msg);
				continue;
			}
			break;

			default:
			break;
		}

		/*need to free req_msg*/
		_net_nfc_manager_util_free_mem(req_msg);
	}
	return (void *)NULL;
}


void net_nfc_dispatcher_cleanup_queue(void)
{
	pthread_mutex_lock(&g_dispatcher_queue_lock);

	DEBUG_SERVER_MSG("cleanup dispatcher Q start");

	net_nfc_request_msg_t *req_msg = NULL;

	while((req_msg = _net_nfc_dispatcher_queue_pop()) != NULL)
	{
		DEBUG_SERVER_MSG("abandon request");
		_net_nfc_manager_util_free_mem(req_msg);
	}

	DEBUG_SERVER_MSG("cleanup dispatcher Q end");

	pthread_mutex_unlock(&g_dispatcher_queue_lock);
}

void net_nfc_dispatcher_put_cleaner(void)
{
	net_nfc_request_msg_t *req_msg = NULL;

	_net_nfc_util_alloc_mem(req_msg, sizeof(net_nfc_request_msg_t));
	if(req_msg != NULL)
	{
		DEBUG_SERVER_MSG("put cleaner request");

		req_msg->length = sizeof(net_nfc_request_msg_t);
		req_msg->request_type = NET_NFC_MESSAGE_SERVICE_CLEANER;
		net_nfc_dispatcher_queue_push(req_msg);
	}
}
