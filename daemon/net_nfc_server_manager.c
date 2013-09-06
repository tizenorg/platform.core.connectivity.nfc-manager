/*
 * Copyright (c) 2012, 2013 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Flora License, Version 1.1 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://floralicense.org/license/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <vconf.h>

#include "net_nfc_debug_internal.h"
#include "net_nfc_typedef_internal.h"

#include "net_nfc_gdbus.h"
#include "net_nfc_server_common.h"
#include "net_nfc_server_manager.h"
#include "net_nfc_server_se.h"
#include "net_nfc_server_llcp.h"
#include "net_nfc_server_context.h"
#include "net_nfc_server_controller.h"
#include "net_nfc_server_process_snep.h"
#include "net_nfc_server_process_npp.h"
#include "net_nfc_server_process_handover.h"


typedef struct _ManagerActivationData ManagerActivationData;

struct _ManagerActivationData
{
	NetNfcGDbusManager *manager;
	GDBusMethodInvocation *invocation;
	gboolean is_active;
};


static NetNfcGDbusManager *manager_skeleton = NULL;

static net_nfc_error_e manager_active(void);

static net_nfc_error_e manager_deactive(void);

static void manager_handle_active_thread_func(gpointer user_data);

static gboolean manager_handle_set_active(NetNfcGDbusManager *manager,
		GDBusMethodInvocation *invocation,
		gboolean arg_is_active,
		GVariant *smack_privilege,
		gpointer user_data);

static gboolean manager_handle_get_server_state(NetNfcGDbusManager *manager,
		GDBusMethodInvocation *invocation,
		GVariant *smack_privilege,
		gpointer user_data);


static void manager_active_thread_func(gpointer user_data);


/* reimplementation of net_nfc_service_init()*/
static net_nfc_error_e manager_active(void)
{
	net_nfc_error_e result;
	int ret;
	int se_type;

	if (net_nfc_controller_is_ready(&result) == false)
	{
		DEBUG_ERR_MSG("net_nfc_controller_is_ready failed [%d]", result);

		return result;
	}

	/* keep_SE_select_value */
	ret = vconf_get_int(VCONFKEY_NFC_SE_TYPE, &se_type);
	if (ret == 0)
	{
		DEBUG_SERVER_MSG("manager_active  se_type [%d]",se_type);
		result = net_nfc_server_se_change_se(se_type);
	}

	/* register default snep server */
	net_nfc_server_snep_default_server_register();

	/* register default npp server */
	net_nfc_server_npp_default_server_register();

	/* register default handover server */
	net_nfc_server_handover_default_server_register();

	if (net_nfc_controller_configure_discovery(
				NET_NFC_DISCOVERY_MODE_START,
				NET_NFC_ALL_ENABLE,
				&result) == TRUE)
	{
		/* vconf on */
		if (vconf_set_bool(VCONFKEY_NFC_STATE, TRUE) != 0)
		{
			DEBUG_ERR_MSG("vconf_set_bool is failed");

			result = NET_NFC_OPERATION_FAIL;
		}
	}
	else
	{
		DEBUG_ERR_MSG("net_nfc_controller_configure_discovery is failed, [%d]", result);
	}

	return result;
}

/* reimplementation of net_nfc_service_deinit()*/
static net_nfc_error_e manager_deactive(void)
{
	net_nfc_error_e result;

	if (net_nfc_controller_is_ready(&result) == false)
	{
		DEBUG_ERR_MSG("net_nfc_controller_is_ready failed [%d]", result);

		return result;
	}

	/* unregister all services */
	net_nfc_server_llcp_unregister_all();

	/* keep_SE_select_value do not need to update vconf and gdbus_se_setting */
	//	result = net_nfc_server_se_change_se(SECURE_ELEMENT_TYPE_INVALID);

	{
		net_nfc_error_e result_ese, result_uicc;

		/*turn off ESE*/
		net_nfc_controller_set_secure_element_mode(
				SECURE_ELEMENT_TYPE_ESE,
				SECURE_ELEMENT_OFF_MODE,
				&result_ese);

		/*turn off UICC*/
		net_nfc_controller_set_secure_element_mode(
				SECURE_ELEMENT_TYPE_UICC,
				SECURE_ELEMENT_OFF_MODE,
				&result_uicc);

	}

	if (net_nfc_controller_configure_discovery(
				NET_NFC_DISCOVERY_MODE_STOP,
				NET_NFC_ALL_DISABLE,
				&result) == TRUE)
	{
		/* vconf off */
		if (vconf_set_bool(VCONFKEY_NFC_STATE, FALSE) != 0)
		{
			DEBUG_ERR_MSG("vconf_set_bool is failed");

			result = NET_NFC_OPERATION_FAIL;
		}
	}
	else
	{
		DEBUG_ERR_MSG("net_nfc_controller_configure_discovery is failed, [%d]", result);
	}

	return result;
}

static void manager_handle_active_thread_func(gpointer user_data)
{
	ManagerActivationData *data = (ManagerActivationData *)user_data;
	net_nfc_error_e result;

	g_assert(data != NULL);
	g_assert(data->manager != NULL);
	g_assert(data->invocation != NULL);

	if (data->is_active)
		result = manager_active();
	else
		result = manager_deactive();

	net_nfc_gdbus_manager_complete_set_active(data->manager,
			data->invocation,
			result);

	if (result == NET_NFC_OK) {
		INFO_MSG("nfc %s", data->is_active ?
				"activated" : "deactivated");

		net_nfc_gdbus_manager_emit_activated(data->manager,
				data->is_active);
	} else {
		DEBUG_ERR_MSG("activation change failed, [%d]", result);
	}

	g_object_unref(data->invocation);
	g_object_unref(data->manager);

	g_free(data);

	/* shutdown process if it doesn't need */
	if (data->is_active == false &&
			net_nfc_server_gdbus_is_server_busy() == false) {

		net_nfc_server_controller_deinit();
	}
}


static gboolean manager_handle_set_active(NetNfcGDbusManager *manager,
		GDBusMethodInvocation *invocation,
		gboolean arg_is_active,
		GVariant *smack_privilege,
		gpointer user_data)
{
	ManagerActivationData *data;
	gboolean result;

	INFO_MSG(">>> REQUEST from [%s]",
			g_dbus_method_invocation_get_sender(invocation));

	/* check privilege and update client context */
	if (net_nfc_server_gdbus_check_privilege(invocation,
				smack_privilege,
				"nfc-manager::admin",
				"rw") == false) {
		DEBUG_ERR_MSG("permission denied, and finished request");

		return FALSE;
	}

	DEBUG_SERVER_MSG("is_active %d", arg_is_active);

	data = g_try_new0(ManagerActivationData, 1);
	if (data == NULL)
	{
		DEBUG_ERR_MSG("Memory allocation failed");
		g_dbus_method_invocation_return_dbus_error(invocation,
				"org.tizen.NetNfcService.AllocationError",
				"Can not allocate memory");
		return FALSE;
	}

	data->manager = g_object_ref(manager);
	data->invocation = g_object_ref(invocation);
	data->is_active = arg_is_active;

	result = net_nfc_server_controller_async_queue_push(
			manager_handle_active_thread_func, data);
	if (result == FALSE)
	{
		g_dbus_method_invocation_return_dbus_error(invocation,
				"org.tizen.NetNfcService.ThreadError",
				"can not push to controller thread");

		g_object_unref(data->invocation);
		g_object_unref(data->manager);

		g_free(data);
	}

	return result;
}

static gboolean manager_handle_get_server_state(NetNfcGDbusManager *manager,
		GDBusMethodInvocation *invocation,
		GVariant *smack_privilege,
		gpointer user_data)
{
	guint32 state;

	INFO_MSG(">>> REQUEST from [%s]",
			g_dbus_method_invocation_get_sender(invocation));

	/* check privilege and update client context */
	if (net_nfc_server_gdbus_check_privilege(invocation,
				smack_privilege,
				"nfc-manager::admin",
				"r") == false) {
		DEBUG_ERR_MSG("permission denied, and finished request");

		return FALSE;
	}

	state = net_nfc_server_get_state();

	net_nfc_gdbus_manager_complete_get_server_state(manager,
			invocation,
			NET_NFC_OK,
			state);
	return TRUE;
}

/* server side */
static void manager_active_thread_func(gpointer user_data)
{
	ManagerActivationData *data =
		(ManagerActivationData *)user_data;
	net_nfc_error_e ret;

	g_assert(data != NULL);

	if (data->is_active)
		ret = manager_active();
	else
		ret = manager_deactive();
	if (ret == NET_NFC_OK)
	{
		INFO_MSG("nfc %s",
				data->is_active ? "activated" : "deactivated");

		net_nfc_gdbus_manager_emit_activated(data->manager, data->is_active);
	}
	else
	{
		DEBUG_ERR_MSG("activation change failed, [%d]", ret);
	}

	g_free(data);
}

gboolean net_nfc_server_manager_init(GDBusConnection *connection)
{
	GError *error = NULL;

	if (manager_skeleton)
		g_object_unref(manager_skeleton);

	manager_skeleton = net_nfc_gdbus_manager_skeleton_new();

	g_signal_connect(manager_skeleton,
			"handle-set-active",
			G_CALLBACK(manager_handle_set_active),
			NULL);

	g_signal_connect(manager_skeleton,
			"handle-get-server-state",
			G_CALLBACK(manager_handle_get_server_state),
			NULL);

	if (g_dbus_interface_skeleton_export(
				G_DBUS_INTERFACE_SKELETON(manager_skeleton),
				connection,
				"/org/tizen/NetNfcService/Manager",
				&error) == FALSE)
	{
		DEBUG_ERR_MSG("Can not skeleton_export %s", error->message);

		g_error_free(error);

		net_nfc_server_manager_deinit();

		return FALSE;
	}

	return TRUE;
}

void net_nfc_server_manager_deinit(void)
{
	if (manager_skeleton)
	{
		g_object_unref(manager_skeleton);
		manager_skeleton = NULL;
	}
}

void net_nfc_server_manager_set_active(gboolean is_active)
{
	ManagerActivationData *data;

	if (manager_skeleton == NULL)
	{
		DEBUG_ERR_MSG("net_nfc_server_manager is not initialized");

		return;
	}

	DEBUG_SERVER_MSG("is_active %d", is_active);

	data = g_try_new0(ManagerActivationData, 1);
	if (data == NULL)
	{
		DEBUG_ERR_MSG("Memory allocation failed");

		return;
	}

	data->manager = g_object_ref(manager_skeleton);
	data->is_active = is_active;

	if (net_nfc_server_controller_async_queue_push(
				manager_active_thread_func,
				data) == FALSE)
	{
		DEBUG_ERR_MSG("can not push to controller thread");

		g_object_unref(data->manager);
		g_free(data);
	}
}

bool net_nfc_server_manager_get_active()
{
	int value;

	if (vconf_get_bool(VCONFKEY_NFC_STATE, &value) < 0)
		return false;

	return (!!value);
}
