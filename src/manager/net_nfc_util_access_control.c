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


#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "SEService.h"
#include "Reader.h"
#include "Session.h"
#include "ClientChannel.h"
#include "GPSEACL.h"
#include "SignatureHelper.h"

#include "net_nfc_debug_private.h"

static bool initialized = false;
static se_service_h se_service = NULL;
static reader_h readers[10] = { NULL, };
static session_h sessions[10] = { NULL, };
static channel_h channels[10] = { NULL, };
static gp_se_acl_h acls[10] = { NULL, };

static void _session_open_channel_cb(channel_h channel, int error, void *userData)
{
	if (error == 0 && channel != NULL)
	{
		int i = (int)userData;

		DEBUG_MSG("channel [%p], error [%d], userdata [%d]", channel, error, (int)userData);

		channels[i] = channel;
		acls[i] = gp_se_acl_create_instance(channel);

		initialized = true;
	}
	else
	{
		DEBUG_ERR_MSG("invalid handle");
	}
}

static void _reader_open_session_cb(session_h session, int error, void *userData)
{
	unsigned char aid[] = { 0xA0, 0x00, 0x00, 0x00, 0x63, 0x50, 0x4B, 0x43, 0x53, 0x2D, 0x31, 0x35 };

	if (error == 0 && session != NULL)
	{
		int i = (int)userData;

		sessions[i] = session;

		session_open_basic_channel(session, aid, sizeof(aid), _session_open_channel_cb, userData);
	}
	else
	{
		DEBUG_ERR_MSG("invalid handle");
	}
}

static void _se_service_connected_cb(se_service_h handle, void *data)
{
	if (handle != NULL)
	{
		int i;

		se_service = handle;

		se_service_get_readers(se_service, readers, (sizeof(readers) / sizeof(reader_h)));

		for(i = 0; readers[i] != NULL && i < (sizeof(readers) / sizeof(reader_h)); i++)
		{
			reader_open_session(readers[i], _reader_open_session_cb, (void *)i);
		}
	}
	else
	{
		DEBUG_ERR_MSG("invalid handle");
	}
}

bool net_nfc_util_access_control_is_initialized(void)
{
	return initialized;
}

void net_nfc_util_access_control_initialize(void)
{
	se_service_h handle = NULL;

	if (net_nfc_util_access_control_is_initialized() == false)
	{
		if ((handle = se_service_create_instance((void *)1, _se_service_connected_cb)) == NULL)
		{
		}
	}
}

void net_nfc_util_access_control_update_list(void)
{
	int i;

	if (net_nfc_util_access_control_is_initialized() == true)
	{
		for (i = 0; i < (sizeof(acls) / sizeof(gp_se_acl_h)); i++)
		{
			if (acls[i] != NULL)
			{
				gp_se_acl_update_acl(acls[i]);
			}
		}
	}
}

bool net_nfc_util_access_control_is_authorized_package(const char* pkg_name, uint8_t *aid, uint32_t length)
{
	bool result = false;

	DEBUG_MSG("aid : { %02X %02X %02X %02X ... }", aid[0], aid[1], aid[2], aid[3]);

	if (net_nfc_util_access_control_is_initialized() == true)
	{
		int i;
		uint8_t hash[20] = { 0, };
		uint32_t hash_length = sizeof(hash);

		if (signature_helper_get_certificate_hash(pkg_name, hash, &hash_length) == 0)
		{
			DEBUG_MSG("%s hash : { %02X %02X %02X %02X ... }", pkg_name, hash[0], hash[1], hash[2], hash[3]);
			for (i = 0; result == false && i < (sizeof(acls) / sizeof(gp_se_acl_h)); i++)
			{
				if (acls[i] != NULL)
				{
					DEBUG_MSG("acl[%d] exists : %s", i, reader_get_name(readers[i]));
					result = gp_se_acl_is_authorized_access(acls[i], aid, length, hash, hash_length);
					DEBUG_MSG("result = %s", (result) ? "true" : "false");
				}
			}
		}
		else
		{
			/* hash not found */
			DEBUG_ERR_MSG("hash doesn't exist : %s", pkg_name);
		}
	}

	return result;
}

void net_nfc_util_access_control_release(void)
{
	int i;

	for (i = 0; i < (sizeof(acls) / sizeof(gp_se_acl_h)); i++)
	{
		if (acls[i] != NULL)
		{
			gp_se_acl_destroy_instance(acls[i]);
			acls[i] = NULL;
		}
	}

	if (se_service != NULL)
	{
		se_service_destroy_instance(se_service);
		se_service = NULL;
		memset(readers, 0, sizeof(readers));
		memset(sessions, 0, sizeof(sessions));
		memset(channels, 0, sizeof(channels));
	}

	initialized = false;
}
