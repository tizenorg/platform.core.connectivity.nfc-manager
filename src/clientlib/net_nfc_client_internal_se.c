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

#include "net_nfc_tag.h"
#include "net_nfc_typedef_private.h"
#include "net_nfc_client_ipc_private.h"
#include "net_nfc_debug_private.h"
#include "net_nfc_util_private.h"
#include "net_nfc_client_nfc_private.h"
#include "net_nfc_client_util_private.h"

#include <string.h>
#include <pthread.h>

#ifndef NET_NFC_EXPORT_API
#define NET_NFC_EXPORT_API __attribute__((visibility("default")))
#endif

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_set_secure_element_type(net_nfc_se_type_e se_type, void* trans_param)
{
	net_nfc_error_e ret;
	net_nfc_request_set_se_t request = { 0, };

	if (se_type < NET_NFC_SE_TYPE_NONE || se_type > NET_NFC_SE_TYPE_UICC)
	{
		return NET_NFC_INVALID_PARAM;
	}

	request.length = sizeof(net_nfc_request_set_se_t);
	request.request_type = NET_NFC_MESSAGE_SET_SE;
	request.se_type = se_type;
	request.trans_param = trans_param;

	ret = _net_nfc_client_send_reqeust((net_nfc_request_msg_t *)&request, NULL);

	return ret;
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_get_secure_element_type(void* trans_param)
{
	net_nfc_error_e ret;
	net_nfc_request_get_se_t request = { 0, };

	request.length = sizeof(net_nfc_request_get_se_t);
	request.request_type = NET_NFC_MESSAGE_GET_SE;
	request.trans_param = trans_param;

	ret = _net_nfc_client_send_reqeust((net_nfc_request_msg_t *)&request, NULL);

	return ret;
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_open_internal_secure_element(net_nfc_se_type_e se_type, void* trans_param)
{
	net_nfc_error_e ret;
	net_nfc_request_open_internal_se_t request = { 0, };

	if (se_type < NET_NFC_SE_TYPE_NONE || se_type > NET_NFC_SE_TYPE_UICC)
	{
		return NET_NFC_INVALID_PARAM;
	}

	request.length = sizeof(net_nfc_request_open_internal_se_t);
	request.request_type = NET_NFC_MESSAGE_OPEN_INTERNAL_SE;
	request.se_type = se_type;
	request.trans_param = trans_param;

	ret = _net_nfc_client_send_reqeust((net_nfc_request_msg_t *)&request, NULL);

	return ret;
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_close_internal_secure_element(net_nfc_target_handle_h handle, void *trans_param)
{
	net_nfc_error_e ret;
	net_nfc_request_close_internal_se_t request = { 0, };

	if (handle == NULL)
	{
		return NET_NFC_NULL_PARAMETER;
	}

	request.length = sizeof(net_nfc_request_close_internal_se_t);
	request.request_type = NET_NFC_MESSAGE_CLOSE_INTERNAL_SE;
	request.handle = (net_nfc_target_handle_s *)handle;
	request.trans_param = trans_param;

	ret = _net_nfc_client_send_reqeust((net_nfc_request_msg_t *)&request, NULL);

	return ret;
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_send_apdu(net_nfc_target_handle_h handle, data_h apdu, void* trans_param)
{
	net_nfc_error_e ret;
	net_nfc_request_send_apdu_t *request = NULL;
	uint32_t length = 0;
	data_s *apdu_data = (data_s *)apdu;

	if (handle == NULL || apdu_data == NULL || apdu_data->buffer == NULL || apdu_data->length == 0)
	{
		return NET_NFC_NULL_PARAMETER;
	}

	length = sizeof(net_nfc_request_send_apdu_t) + apdu_data->length;

	_net_nfc_client_util_alloc_mem(request, length);
	if (request == NULL)
	{
		return NET_NFC_ALLOC_FAIL;
	}

	request->length = length;
	request->request_type = NET_NFC_MESSAGE_SEND_APDU_SE;
	request->handle = (net_nfc_target_handle_s *)handle;
	request->trans_param = trans_param;

	request->data.length = apdu_data->length;
	memcpy(&request->data.buffer, apdu_data->buffer, request->data.length);

	ret = _net_nfc_client_send_reqeust((net_nfc_request_msg_t *)request, NULL);

	_net_nfc_client_util_free_mem(request);

	return ret;
}

