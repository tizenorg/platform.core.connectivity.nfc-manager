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

#include "net_nfc.h"
#include "net_nfc_typedef.h"
#include "net_nfc_util_private.h"
#include "net_nfc_util_defines.h"
#include "net_nfc_debug_private.h"
#include "net_nfc_client_ipc_private.h"
#include "net_nfc_exchanger_private.h"
#include "net_nfc_client_util_private.h"
#include "net_nfc_client_nfc_private.h"
#include "../agent/include/dbus_agent_binding_private.h"

#include <pthread.h>
#include <glib.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

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
	net_nfc_error_e result = NET_NFC_OK;

	pthread_mutex_lock(&g_client_context.g_client_lock);

	if (vconf_set_bool(NET_NFC_DISABLE_LAUNCH_POPUP_KEY, enable ? 0 : 1) != 0)
	{
		result = NET_NFC_UNKNOWN_ERROR;
	}

	pthread_mutex_unlock(&g_client_context.g_client_lock);

	return result;
}

NET_NFC_EXPORT_API bool net_nfc_get_launch_popup_state(void)
{
	int disable = 0;

	/* check state of launch popup */
	if (vconf_get_bool(NET_NFC_DISABLE_LAUNCH_POPUP_KEY, &disable) == 0 && disable == TRUE)
	{
		DEBUG_MSG("skip launch popup");
		return false;
	}

	return true;
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_set_state(int state, net_nfc_set_activation_completed_cb callback)
{
	net_nfc_error_e ret = NET_NFC_UNKNOWN_ERROR;;
	uint32_t status;
	GError *error = NULL;
	DBusGConnection *connection = NULL;
	DBusGProxy *proxy = NULL;

	g_net_nfc_set_activation_completed_cb = callback;

	if(!g_thread_supported())
	{
		g_thread_init(NULL);
	}

	dbus_g_thread_init();
	g_type_init();

	connection = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);
	if (connection != NULL)
	{
		proxy = dbus_g_proxy_new_for_name(connection, "com.samsung.slp.nfc.agent", "/com/samsung/slp/nfc/agent", "com.samsung.slp.nfc.agent");
		if (proxy != NULL)
		{
			if (state == FALSE)
			{
				if (!com_samsung_slp_nfc_agent_terminate(proxy, &status, &error))
				{
					DEBUG_ERR_MSG("Termination is failed: %d", status);
					DEBUG_ERR_MSG("error : %s", error->message);
					g_error_free(error);
				}
				else
				{
					DEBUG_CLIENT_MSG("terminate is completed");

					if (vconf_set_bool(VCONFKEY_NFC_STATE, FALSE) != 0)
					{
						DEBUG_ERR_MSG("vconf_set_bool failed");
					}

					ret = NET_NFC_OK;
				}
			}
			else
			{
				if (!com_samsung_slp_nfc_agent_launch(proxy, &status, &error))
				{
					DEBUG_ERR_MSG("[%s(): %d] faile to launch nfc-manager\n", __FUNCTION__, __LINE__);
					DEBUG_ERR_MSG("[%s(): %d] error : %s\n", __FUNCTION__, __LINE__, error->message);
					g_error_free(error);
				}
				else
				{
					DEBUG_CLIENT_MSG("[%s(): %d] launch is completed\n", __FUNCTION__, __LINE__);

					if (vconf_set_bool(VCONFKEY_NFC_STATE, TRUE) != 0)
					{
						DEBUG_ERR_MSG("vconf_set_bool failed");
					}

					if (vconf_set_int(NET_NFC_VCONF_KEY_PROGRESS, 0) != 0)
					{
						DEBUG_ERR_MSG("vconf_set_bool failed");
					}

					ret = NET_NFC_OK;
				}
			}

			g_object_unref(proxy);
		}
		else
		{
			DEBUG_ERR_MSG("dbus_g_proxy_new_for_name returns NULL");
		}
	}
	else
	{
		DEBUG_ERR_MSG("dbus_g_bus_get returns NULL [%s]", error->message);
		g_error_free(error);
	}
	/* todo it will be move to the point of  init response */
	if (g_net_nfc_set_activation_completed_cb)
	{
		DEBUG_ERR_MSG("g_net_nfc_set_activation_completed_cb ret[%d]",ret);

		g_net_nfc_set_activation_completed_cb(ret,NULL);

		g_net_nfc_set_activation_completed_cb = NULL;
	}
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

