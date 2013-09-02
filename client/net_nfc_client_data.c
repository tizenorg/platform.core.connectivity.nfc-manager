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

#include <string.h>

#include "net_nfc_debug_internal.h"
#include "net_nfc_data.h"
#include "net_nfc_typedef_internal.h"
#include "net_nfc_util_internal.h"

API net_nfc_error_e net_nfc_create_data_only(data_h* data)
{
	return net_nfc_create_data(data, NULL, 0);
}

API net_nfc_error_e net_nfc_create_data(data_h *data,
		const uint8_t *bytes, size_t length)
{
	data_s *tmp_data = NULL;

	if (data == NULL)
		return NET_NFC_NULL_PARAMETER;

	*data = NULL;

	_net_nfc_util_alloc_mem(tmp_data, sizeof(data_s));
	if (tmp_data == NULL)
		return NET_NFC_ALLOC_FAIL;

	if (0 < length)
	{
		_net_nfc_util_alloc_mem(tmp_data->buffer, length);
		if (tmp_data->buffer == NULL)
		{
			_net_nfc_util_free_mem(tmp_data);
			return NET_NFC_ALLOC_FAIL;
		}

		tmp_data->length = length;

		if (bytes != NULL)
		{
			memcpy(tmp_data->buffer, bytes, length);
		}
	}

	*data = (data_h)tmp_data;

	return NET_NFC_OK;
}

API net_nfc_error_e net_nfc_get_data(const data_h data,
		uint8_t **bytes, size_t *length)
{
	data_s *tmp_data = (data_s *)data;

	if (bytes == NULL || length == NULL)
		return NET_NFC_NULL_PARAMETER;

	*bytes = NULL;
	*length = 0;

	if (data == NULL)
		return NET_NFC_NULL_PARAMETER;

	*bytes = tmp_data->buffer;
	*length = tmp_data->length;

	return NET_NFC_OK;
}

API net_nfc_error_e net_nfc_set_data(data_h data,
		const uint8_t *bytes, size_t length)
{
	data_s *tmp_data = (data_s *)data;

	if (data == NULL)
		return NET_NFC_NULL_PARAMETER;

	if (tmp_data->buffer == bytes && tmp_data->length == length)
		return NET_NFC_OK;

	if (tmp_data->buffer != NULL)
		_net_nfc_util_free_mem(tmp_data->buffer);

	if (length <= 0)
	{
		tmp_data->buffer = NULL;
		tmp_data->length = 0;
		return NET_NFC_OK;
	}

	if (length > 0)
		_net_nfc_util_alloc_mem((tmp_data)->buffer, length);

	if (bytes != NULL)
		memcpy(tmp_data->buffer, bytes, length);

	tmp_data->length = length;

	return NET_NFC_OK;
}

API size_t net_nfc_get_data_length(const data_h data)
{
	data_s *tmp_data = (data_s *)data;

	if (data == NULL)
		return 0;

	return tmp_data->length;
}

API uint8_t* net_nfc_get_data_buffer(const data_h data)
{
	data_s *tmp_data = (data_s *)data;

	if (data == NULL)
		return NULL;

	return tmp_data->buffer;
}

API net_nfc_error_e net_nfc_free_data(data_h data)
{
	data_s *tmp_data = (data_s *)data;

	if (data == NULL)
		return NET_NFC_NULL_PARAMETER;

	if (tmp_data->buffer != NULL)
		_net_nfc_util_free_mem(tmp_data->buffer);

	_net_nfc_util_free_mem(tmp_data);

	return NET_NFC_OK;
}