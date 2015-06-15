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

#include <glib.h>
#include <vconf.h>

#include "net_nfc_typedef.h"
#include "net_nfc_debug_internal.h"
#include "net_nfc_server_common.h"
#include "net_nfc_server_manager.h"
#include "net_nfc_server_se.h"
#include "net_nfc_server_route_table.h"
#include "net_nfc_server_vconf.h"

static void net_nfc_server_vconf_lock_state_changed(keynode_t *key,
						void *user_data);

static void net_nfc_server_vconf_pm_state_changed(keynode_t *key,
						void *user_data);

#ifdef ENABLE_TELEPHONY
static void net_nfc_server_vconf_flight_mode_changed(keynode_t *key,
						void *user_data);
#endif

static void net_nfc_server_vconf_lock_state_changed(keynode_t *key,
						void *user_data)
{

	gint state = 0;
	gint result;
	gint lock_state = 0;

	result = vconf_get_bool(VCONFKEY_NFC_STATE, &state);
	if (result != 0)
		DEBUG_ERR_MSG("can not get %s", "VCONFKEY_NFC_STATE");

	if (state == false)
	{
		DEBUG_MSG("NFC off");
		return;
	}

	if (vconf_get_int(VCONFKEY_IDLE_LOCK_STATE, &lock_state) != 0)
		DEBUG_ERR_MSG("%s does not exist", "VCONFKEY_IDLE_LOCK_STATE");


	if (lock_state == VCONFKEY_IDLE_UNLOCK ||
		            lock_state == VCONFKEY_IDLE_LOCK)
	{
		net_nfc_server_restart_polling_loop();
	}

}

static void net_nfc_server_vconf_pm_state_changed(keynode_t *key,
						void *user_data)
{
	gint state = 0;
	gint pm_state = 0;
	gint lock_screen_set = 0;
	gint result;

	result = vconf_get_bool(VCONFKEY_NFC_STATE, &state);

	if (result != 0)
		DEBUG_ERR_MSG("can not get %s", "VCONFKEY_NFC_STATE");

	if (state == false)
	{
		DEBUG_MSG("NFC off");
		return;
	}

	result = vconf_get_int(VCONFKEY_PM_STATE, &pm_state);

	if (result != 0)
		DEBUG_ERR_MSG("can not get %s", "VCONFKEY_PM_STATE");

	DEBUG_SERVER_MSG("pm_state : %d", pm_state);

	result = vconf_get_int(VCONFKEY_SETAPPL_SCREEN_LOCK_TYPE_INT, &lock_screen_set);

	if (result != 0)
		DEBUG_ERR_MSG("can not get %s", "VCONFKEY_SETAPPL_SCREEN_LOCK_TYPE_INT");

	DEBUG_SERVER_MSG("lock_screen_set : %d", lock_screen_set);

#if 0
	if ( lock_screen_set == SETTING_SCREEN_LOCK_TYPE_NONE &&
		(pm_state == VCONFKEY_PM_STATE_NORMAL ||
		            pm_state == VCONFKEY_PM_STATE_LCDOFF) )
#endif
	if (pm_state == VCONFKEY_PM_STATE_NORMAL ||
		pm_state == VCONFKEY_PM_STATE_LCDOFF)
	{
		net_nfc_server_restart_polling_loop();
	}
}

static void net_nfc_server_vconf_se_type_changed(keynode_t *key,
						void *user_data)
{
	net_nfc_server_se_policy_apply();
}

void net_nfc_server_vconf_init(void)
{
	vconf_notify_key_changed(VCONFKEY_IDLE_LOCK_STATE,
			net_nfc_server_vconf_lock_state_changed,
			NULL);
#if 1
	vconf_notify_key_changed(VCONFKEY_PM_STATE,
			net_nfc_server_vconf_pm_state_changed,
			NULL);
#endif

	vconf_notify_key_changed(VCONFKEY_NFC_SE_TYPE,
			net_nfc_server_vconf_se_type_changed,
			NULL);
}

void net_nfc_server_vconf_deinit(void)
{
	vconf_ignore_key_changed(VCONFKEY_IDLE_LOCK_STATE,
			net_nfc_server_vconf_lock_state_changed);

	vconf_ignore_key_changed(VCONFKEY_NFC_SE_TYPE,
			net_nfc_server_vconf_se_type_changed);
}

bool net_nfc_check_csc_vconf(void)
{
	int state = 0;;
	gint result = 0;

	if (state == true)
	{
		DEBUG_ERR_MSG("This is CSC Booting!!");
		return true;
	}
	else
	{
		DEBUG_ERR_MSG("This is Normal Booting!!");
		return false;
	}
}

bool net_nfc_check_start_polling_vconf(void)
{
	gint lock_state = 0;
	gint lock_screen_set = 0;
	gint pm_state = 0;

	if (vconf_get_int(VCONFKEY_IDLE_LOCK_STATE, &lock_state) != 0)
		DEBUG_ERR_MSG("%s does not exist", "VCONFKEY_IDLE_LOCK_STATE");

	if (vconf_get_int(VCONFKEY_SETAPPL_SCREEN_LOCK_TYPE_INT, &lock_screen_set) != 0)
		DEBUG_ERR_MSG("%s does not exist", "VCONFKEY_SETAPPL_SCREEN_LOCK_TYPE_INT");

	if (vconf_get_int(VCONFKEY_PM_STATE, &pm_state) != 0)
		DEBUG_ERR_MSG("%s does not exist", "VCONFKEY_PM_STATE");


	DEBUG_SERVER_MSG("lock_screen_set:%d ,pm_state:%d,lock_state:%d",
		lock_screen_set , pm_state , lock_state);

		if (lock_state == VCONFKEY_IDLE_UNLOCK)
		{
			DEBUG_SERVER_MSG("polling enable");
			return TRUE;
		}

		if (lock_state == VCONFKEY_IDLE_LOCK || pm_state == VCONFKEY_PM_STATE_LCDOFF)
		{
			DEBUG_SERVER_MSG("polling disabled");
			return FALSE;
		}

	return FALSE;
}
