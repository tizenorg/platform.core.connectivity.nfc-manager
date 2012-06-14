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


#define GALLERY_PKG_NAME_KEY NET_NFC_KEY_PREFIX"deb.com.samsung.gallery"
#define CONTACT_PKG_NAME_KEY NET_NFC_KEY_PREFIX"deb.com.samsung.contacts"
#define BROWSER_PKG_NAME_KEY NET_NFC_KEY_PREFIX"deb.com.samsung.browser"
#define VIDEO_PKG_NAME_KEY NET_NFC_KEY_PREFIX"deb.com.samsung.video-player"

#define NET_NFC_MODE_SE_KEY NET_NFC_KEY_PREFIX"se"
#define NET_NFC_EEPROM_WRITEN NET_NFC_KEY_PREFIX"eeprom"


#define NET_NFC_MODE_SE_ON 1
#define NET_NFC_MODE_SE_OFF 0

#define TIMEOUT 60 /* secs */

#define NET_NFC_KEY_PREFIX "memory/nfc/client/"

static guint g_source_id_gallery = 0;
static guint g_source_id_contact = 0;
static guint g_source_id_browser = 0;
static guint g_source_id_video = 0;

static bool is_EEPROM_writen = false;

// static function

static gboolean timedout_func(gpointer data);
static void net_nfc_service_vconf_changed_cb(keynode_t* key, void* data);

/////////////////////////////////////////////////////////////////////////



static gboolean timedout_func(gpointer data)
{
	DEBUG_SERVER_MSG("timed out. clean up the value of key = [%s] \n", (char *)data);

	vconf_set_str((char *)data, "");

	if(strcmp((char *)data, GALLERY_PKG_NAME_KEY) == 0){g_source_id_gallery = 0;}
	else if(strcmp((char *)data, CONTACT_PKG_NAME_KEY) == 0){g_source_id_contact = 0;}
	else if(strcmp((char *)data, BROWSER_PKG_NAME_KEY) == 0){g_source_id_browser = 0;}
	else if(strcmp((char *)data, VIDEO_PKG_NAME_KEY) == 0){g_source_id_video = 0;}

	return FALSE;
}

static void net_nfc_service_se_setting_cb(keynode_t* key, void* data)
{
	DEBUG_SERVER_MSG("se vconf value is changed");

	if(vconf_keynode_get_type(key) == VCONF_TYPE_INT)
	{
		net_nfc_request_set_se_t *msg = NULL;
		int value = vconf_keynode_get_int(key);

		if(value < NET_NFC_SE_CMD_UICC_ON  || value > NET_NFC_SE_CMD_ALL_ON)
		{
			DEBUG_ERR_MSG("invalid value for setting SE mode");
			return;
		}

		_net_nfc_util_alloc_mem(msg, sizeof(net_nfc_request_set_se_t));
		if(msg == NULL)
		{
			DEBUG_ERR_MSG("failed to malloc");
			return;
		}

		msg->length =  sizeof(net_nfc_request_set_se_t);
		msg->request_type = NET_NFC_MESSAGE_SERVICE_SE;
		msg->se_type = value;

		net_nfc_dispatcher_queue_push((net_nfc_request_msg_t *)msg);
	}
	else
	{
		DEBUG_ERR_MSG("unknown type for setting SE mode. only int type will be accepted");
	}
}

static void net_nfc_service_vconf_changed_cb(keynode_t* key, void* data)
{
	DEBUG_SERVER_MSG("vconf value is changed");

	if(vconf_keynode_get_type(key) == VCONF_TYPE_STRING)
	{
		char* str = vconf_keynode_get_str(key);
		char* vconf_key = vconf_keynode_get_name(key);

		if(str != NULL && (strlen(str) > 0))
		{
			if( strcmp(vconf_key, GALLERY_PKG_NAME_KEY) == 0)
			{
				/* make g timer for key A */
				if(g_source_id_gallery != 0)
				{
					g_source_remove(g_source_id_gallery);
					g_source_id_gallery = 0;
				}

				g_source_id_gallery = g_timeout_add_seconds(TIMEOUT, timedout_func, GALLERY_PKG_NAME_KEY);
			}
			else if( strcmp(vconf_key, CONTACT_PKG_NAME_KEY) == 0)
			{
				/* make g timer for key B */
				if(g_source_id_contact != 0)
				{
					g_source_remove(g_source_id_contact);
					g_source_id_contact = 0;
				}
				g_source_id_contact = g_timeout_add_seconds(TIMEOUT, timedout_func, CONTACT_PKG_NAME_KEY);

			}
			else if( strcmp(vconf_key, BROWSER_PKG_NAME_KEY) == 0)
			{
				/* make g timer for key A */
				if(g_source_id_browser != 0)
				{
					g_source_remove(g_source_id_browser);
					g_source_id_browser = 0;
				}
				g_source_id_browser = g_timeout_add_seconds(TIMEOUT, timedout_func, BROWSER_PKG_NAME_KEY);

			}
			else if( strcmp(vconf_key, VIDEO_PKG_NAME_KEY) == 0)
			{
				/* make g timer for key A */
				if(g_source_id_video != 0)
				{
					g_source_remove(g_source_id_video);
					g_source_id_video = 0;
				}

				g_source_id_video = g_timeout_add_seconds(TIMEOUT, timedout_func, VIDEO_PKG_NAME_KEY);
			}
			else
			{
				DEBUG_SERVER_MSG("unknown key value is changed \n");
			}

		}
		else
		{

			if( strcmp(vconf_key, GALLERY_PKG_NAME_KEY) == 0)
			{
				/* make g timer for key A */
				if(g_source_id_gallery != 0)
				{
					g_source_remove(g_source_id_gallery);
					g_source_id_gallery = 0;
				}
			}
			else if( strcmp(vconf_key, CONTACT_PKG_NAME_KEY) == 0)
			{
				/* make g timer for key B */
				if(g_source_id_contact != 0)
				{
					g_source_remove(g_source_id_contact);
					g_source_id_contact = 0;
				}

			}
			else if( strcmp(vconf_key, BROWSER_PKG_NAME_KEY) == 0)
			{
				/* make g timer for key A */
				if(g_source_id_browser != 0)
				{
					g_source_remove(g_source_id_browser);
					g_source_id_browser = 0;
				}

			}
			else if( strcmp(vconf_key, VIDEO_PKG_NAME_KEY) == 0)
			{
				/* make g timer for key A */
				if(g_source_id_video != 0)
				{
					g_source_remove(g_source_id_video);
					g_source_id_video = 0;
				}
			}
			else
			{
				DEBUG_ERR_MSG("unknown key \n");
			}

			DEBUG_SERVER_MSG("value is cleard. so turn off the timer \n");
		}
	}
	else
	{
		DEBUG_ERR_MSG("unknown value type \n");
	}
}


bool _net_nfc_check_pprom_is_completed ()
{

	int vconf_val= false;

	if (is_EEPROM_writen) return true;
	if (vconf_get_bool (NET_NFC_EEPROM_WRITEN, &vconf_val) != 0)
	{
		vconf_val = false;
	}
	if (vconf_val == true) return true;
	return false;
}

void _net_nfc_set_pprom_is_completed ()
{
	vconf_set_bool (NET_NFC_EEPROM_WRITEN, true);
	is_EEPROM_writen = true;
}


void net_nfc_service_vconf_register_notify_listener()
{
	vconf_notify_key_changed(GALLERY_PKG_NAME_KEY, net_nfc_service_vconf_changed_cb, NULL);
	vconf_notify_key_changed(CONTACT_PKG_NAME_KEY, net_nfc_service_vconf_changed_cb, NULL);
	vconf_notify_key_changed(BROWSER_PKG_NAME_KEY, net_nfc_service_vconf_changed_cb, NULL);
	vconf_notify_key_changed(VIDEO_PKG_NAME_KEY, net_nfc_service_vconf_changed_cb, NULL);
	vconf_notify_key_changed(NET_NFC_MODE_SE_KEY, net_nfc_service_se_setting_cb, NULL);
}

void net_nfc_service_vconf_unregister_notify_listener()
{
	vconf_ignore_key_changed(GALLERY_PKG_NAME_KEY, net_nfc_service_vconf_changed_cb);
	vconf_ignore_key_changed(CONTACT_PKG_NAME_KEY, net_nfc_service_vconf_changed_cb);
	vconf_ignore_key_changed(BROWSER_PKG_NAME_KEY, net_nfc_service_vconf_changed_cb);
	vconf_ignore_key_changed(VIDEO_PKG_NAME_KEY, net_nfc_service_vconf_changed_cb);
	vconf_ignore_key_changed(NET_NFC_MODE_SE_KEY, net_nfc_service_se_setting_cb);
}
