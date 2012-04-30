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
