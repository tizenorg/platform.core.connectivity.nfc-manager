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


#include "net_nfc.h"
#include "net_nfc_typedef.h"
#include "net_nfc_util_private.h"
#include "net_nfc_util_defines.h"
#include "net_nfc_debug_private.h"
#include "net_nfc_client_ipc_private.h"
#include "net_nfc_exchanger_private.h"
#include "net_nfc_client_util_private.h"
#include "net_nfc_client_nfc_private.h"

#include <pthread.h>
#include <glib.h>
#include <dbus/dbus-glib.h>

#include "vconf.h"

#ifdef SECURITY_SERVER
#include <security-server.h>
#endif

#ifndef NET_NFC_EXPORT_API
#define NET_NFC_EXPORT_API __attribute__((visibility("default")))
#endif

#define BUFFER_LENGTH_MAX 1024

static client_context_t g_client_context = {NULL, NET_NFC_OK, PTHREAD_MUTEX_INITIALIZER, NET_NFC_ALL_ENABLE, NULL, false};

static net_nfc_set_activation_completed_cb g_net_nfc_set_activation_completed_cb = NULL;

void net_nfc_set_state_response_cb(net_nfc_message_e message, net_nfc_error_e result, void* data, void* user_param, void * trans_data );


static pthread_t g_set_thread = (pthread_t)0;
static pthread_t g_set_send_thread = (pthread_t)0;

static int gv_state = 0;
static bool gv_set_state_lock = false;

static pthread_mutex_t g_client_lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct _client_context_s
{
	pthread_cond_t *client_cond;
	void *user_context;
	int result;
} client_context_s;





static void _net_nfc_reset_client_context()
{
	g_client_context.register_user_param = NULL;
	g_client_context.result = NET_NFC_OK;
	g_client_context.filter = NET_NFC_ALL_ENABLE;
	g_client_context.initialized = false;
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_initialize ()
{
	int cookies_size;
	char * cookies = NULL;
	net_nfc_error_e result = NET_NFC_OK;

	if(!g_thread_supported())
	{
		g_thread_init(NULL);
	}

	dbus_g_thread_init();

	g_type_init();

	pthread_mutex_lock(&g_client_context.g_client_lock);

	if(g_client_context.initialized == true){

		if(net_nfc_get_exchanger_cb() != NULL){

			result = NET_NFC_NOT_ALLOWED_OPERATION;
		}

		pthread_mutex_unlock(&g_client_context.g_client_lock);
		return result;
	}

	_net_nfc_reset_client_context();

#ifdef SECURITY_SERVER
	if((cookies_size = security_server_get_cookie_size()) != 0)
	{
		_net_nfc_client_util_alloc_mem (cookies,cookies_size);
		int error = 0;
		if((error = security_server_request_cookie(cookies, cookies_size)) != SECURITY_SERVER_API_SUCCESS)
		{
			DEBUG_CLIENT_MSG("security server request cookies error = [%d]", error);
			_net_nfc_client_util_free_mem(cookies);
		}
		else
		{
			char printf_buff[BUFFER_LENGTH_MAX] = {0,};
			int buff_count = BUFFER_LENGTH_MAX;
			int i = 0;

			for(; i < cookies_size; i++)
			{
				buff_count -= snprintf ((char *)(printf_buff + BUFFER_LENGTH_MAX - buff_count), buff_count, " %02X", cookies[i]);
			}
			DEBUG_CLIENT_MSG ("client send cookies >>>> %s \n", printf_buff);

			_net_nfc_client_set_cookies (cookies, cookies_size);
		}
	}
#endif

	result = net_nfc_client_socket_initialize();

	if(result != NET_NFC_OK){
		DEBUG_CLIENT_MSG("socket init is failed = [%d]", result);
		_net_nfc_client_free_cookies ();
	}
	else{
		DEBUG_CLIENT_MSG("socket is initialized");
		g_client_context.initialized = true;
	}

	pthread_mutex_unlock(&g_client_context.g_client_lock);

	return result;
}


/* this is sync call */
NET_NFC_EXPORT_API net_nfc_error_e net_nfc_deinitialize ()
{
	pthread_mutex_lock(&g_client_context.g_client_lock);

	net_nfc_error_e result = NET_NFC_OK;
	result = net_nfc_client_socket_finalize();

	_net_nfc_client_free_cookies ();
	_net_nfc_reset_client_context();

	pthread_mutex_unlock(&g_client_context.g_client_lock);
	return result;
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_set_response_callback (net_nfc_response_cb cb, void * user_param)
{
	if (cb == NULL) {
		return NET_NFC_NULL_PARAMETER;
	}
	net_nfc_error_e result = NET_NFC_OK;

	pthread_mutex_lock(&g_client_context.g_client_lock);

	g_client_context.register_user_param = user_param;
	result = _net_nfc_client_register_cb (cb);

	pthread_mutex_unlock(&g_client_context.g_client_lock);

	return result;
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_unset_response_callback (void)
{
	net_nfc_error_e result = NET_NFC_OK;

	pthread_mutex_lock(&g_client_context.g_client_lock);

	g_client_context.register_user_param = NULL;
	result = _net_nfc_client_unregister_cb ();

	pthread_mutex_unlock(&g_client_context.g_client_lock);

	return result;
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_set_launch_popup_state(int enable)
{
	net_nfc_error_e ret;
	net_nfc_request_set_launch_state_t request = {0,};

	request.length = sizeof(net_nfc_request_set_launch_state_t);
	request.request_type = NET_NFC_MESSAGE_SERVICE_SET_LAUNCH_STATE;
	request.set_launch_popup = enable;

	ret = _net_nfc_client_send_reqeust((net_nfc_request_msg_t *)&request, NULL);

	return ret;
}

NET_NFC_EXPORT_API bool net_nfc_get_launch_popup_state(void)
{
	bool enable = 0;

	/* check state of launch popup */
	if (vconf_get_bool(NET_NFC_DISABLE_LAUNCH_POPUP_KEY, &enable) == 0 && enable == TRUE)
	{
		return true;
	}

	DEBUG_MSG("skip launch popup");

	return false;
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_set_state(int state, net_nfc_set_activation_completed_cb callback)
{
	net_nfc_error_e ret = NET_NFC_UNKNOWN_ERROR;;

	DEBUG_MSG("net_nfc_set_state[Enter]");

	if(!g_thread_supported())
	{
		g_thread_init(NULL);
	}

	g_type_init();

	gv_state = state;

	if (gv_state == FALSE)/*Deinit*/
	{
		ret = net_nfc_send_deinit(NULL);

		DEBUG_CLIENT_MSG("Send the deinit msg!!");
	}
	else/*INIT*/
	{
		ret = net_nfc_send_init(NULL);

		DEBUG_CLIENT_MSG("Send the init msg!!");
	}

	DEBUG_CLIENT_MSG("Send the init/deinit msg!![%d]" , ret);

	return ret;
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_is_supported(int *state)
{
	net_nfc_error_e ret;

	if (state != NULL)
	{
		if (vconf_get_bool(VCONFKEY_NFC_FEATURE, state) == 0)
		{
			ret = NET_NFC_OK;
		}
		else
		{
			ret = NET_NFC_INVALID_STATE;
		}
	}
	else
	{
		ret = NET_NFC_NULL_PARAMETER;
	}


	return ret;
}



NET_NFC_EXPORT_API net_nfc_error_e net_nfc_get_state(int *state)
{
	net_nfc_error_e ret;

	if (state != NULL)
	{
		if (vconf_get_bool(VCONFKEY_NFC_STATE, state) == 0)
		{
			ret = NET_NFC_OK;
		}
		else
		{
			ret = NET_NFC_INVALID_STATE;
		}
	}
	else
	{
		ret = NET_NFC_NULL_PARAMETER;
	}


	return ret;
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_state_activate(int client_type)
{
	net_nfc_error_e ret;
	net_nfc_request_change_client_state_t request = {0,};

	request.length = sizeof(net_nfc_request_change_client_state_t);
	request.request_type = NET_NFC_MESSAGE_SERVICE_CHANGE_CLIENT_STATE;
	request.client_state = NET_NFC_CLIENT_ACTIVE_STATE;
	request.client_type = client_type;

	ret = _net_nfc_client_send_reqeust((net_nfc_request_msg_t *)&request, NULL);

	return ret;
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_state_deactivate(void)
{
	net_nfc_error_e ret;
	net_nfc_request_change_client_state_t request = {0,};

	request.length = sizeof(net_nfc_request_change_client_state_t);
	request.request_type = NET_NFC_MESSAGE_SERVICE_CHANGE_CLIENT_STATE;
	request.client_state = NET_NFC_CLIENT_INACTIVE_STATE;

	ret = _net_nfc_client_send_reqeust((net_nfc_request_msg_t *)&request, NULL);

	return ret;
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_sim_test(void)
{
	net_nfc_error_e ret;
	net_nfc_request_test_t request = {0,};

	request.length = sizeof(net_nfc_request_test_t);
	request.request_type = NET_NFC_MESSAGE_SIM_TEST;

	ret = _net_nfc_client_send_reqeust((net_nfc_request_msg_t *)&request, NULL);

	return ret;
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_prbs_test(int tech , int rate)
{
	net_nfc_error_e ret;
	net_nfc_request_test_t request = {0,};

	request.length = sizeof(net_nfc_request_test_t);
	request.request_type = NET_NFC_MESSAGE_PRBS_TEST;/*TEST MODE*/
	request.rate = rate;
	request.tech = tech;

	ret = _net_nfc_client_send_reqeust((net_nfc_request_msg_t *)&request, NULL);

	return ret;
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_get_firmware_version(void)
{
	net_nfc_error_e ret;
	net_nfc_request_msg_t request = { 0,};

	request.length = sizeof(net_nfc_request_msg_t);
	request.request_type = NET_NFC_MESSAGE_GET_FIRMWARE_VERSION;

	ret = _net_nfc_client_send_reqeust((net_nfc_request_msg_t *)&request, NULL);

	return ret;
}

client_context_t* net_nfc_get_client_context()
{
	return &g_client_context;
}

bool net_nfc_tag_is_connected()
{
	pthread_mutex_lock(&g_client_context.g_client_lock);

	if(g_client_context.target_info != NULL && g_client_context.target_info->handle != NULL){

		pthread_mutex_unlock(&g_client_context.g_client_lock);

		return true;
	}
	else{

		pthread_mutex_unlock(&g_client_context.g_client_lock);

		return false;
	}
}

net_nfc_error_e net_nfc_send_init(void* context)
{
	net_nfc_error_e ret;
	net_nfc_request_msg_t req_msg = {0,};


	req_msg.length = sizeof(net_nfc_request_msg_t);
	req_msg.request_type = NET_NFC_MESSAGE_SERVICE_INIT;
	req_msg.user_param = context;

	ret = _net_nfc_client_send_reqeust((net_nfc_request_msg_t *)&req_msg, NULL);

	return ret;
}

net_nfc_error_e net_nfc_send_deinit(void* context)
{
	net_nfc_error_e ret;
	net_nfc_request_msg_t req_msg = {0,};

	req_msg.length = sizeof(net_nfc_request_msg_t);
	req_msg.request_type = NET_NFC_MESSAGE_SERVICE_DEINIT;
	req_msg.user_param = context;

	ret = _net_nfc_client_send_reqeust((net_nfc_request_msg_t *)&req_msg, NULL);

	return ret;
}


