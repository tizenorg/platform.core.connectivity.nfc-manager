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


#ifndef NET_NFC_EXPORT_API
#define NET_NFC_EXPORT_API __attribute__((visibility("default")))
#endif

NET_NFC_EXPORT_API void net_nfc_test_read_test_cb(net_nfc_message_e message, net_nfc_error_e result, void* data, void* user_param, void * trans_data)
{
	int user_context;

	DEBUG_CLIENT_MSG("user_param = [%d] trans_param = [%d] , message[%d]", user_param, trans_data , message);

	switch(message)
	{
		case NET_NFC_MESSAGE_TAG_DISCOVERED:
		{
			if(result == NET_NFC_OK)
			{
				DEBUG_CLIENT_MSG("net_nfc_test_read_cb SUCCESS!!!!! [%d ]" , result);
			}
			else
			{
				DEBUG_CLIENT_MSG("net_nfc_test_read_cb FAIL!!!!![%d]" , result);
			}

		break;
		}
		default:
			break;
	}
}

NET_NFC_EXPORT_API void net_nfc_test_sim_test_cb(net_nfc_message_e message, net_nfc_error_e result, void* data, void* user_param, void * trans_data)
{
	int user_context;

	DEBUG_CLIENT_MSG("user_param = [%d] trans_param = [%d] data = [%d]", user_param, trans_data , data);

	switch(message)
	{
		case NET_NFC_MESSAGE_SIM_TEST:
		{
			if(result == NET_NFC_OK)
			{
				DEBUG_CLIENT_MSG("net_nfc_test_sim_test_cb SUCCESS!!!!! [%d ]" , result);
			}
			else
			{
				DEBUG_CLIENT_MSG("net_nfc_test_sim_test_cb FAIL!!!!![%d]" , result);
			}



		break;
		}
		default:
			break;
	}
}
