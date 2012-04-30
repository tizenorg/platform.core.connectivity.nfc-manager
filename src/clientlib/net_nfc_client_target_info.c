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

#include "net_nfc_typedef_private.h"
#include "net_nfc_target_info.h"
#include "net_nfc_data.h"
#include "net_nfc_debug_private.h"
#include <stdbool.h>

#ifndef NET_NFC_EXPORT_API
#define NET_NFC_EXPORT_API __attribute__((visibility("default")))
#endif


NET_NFC_EXPORT_API net_nfc_error_e net_nfc_get_tag_type (net_nfc_target_info_h target_info, net_nfc_target_type_e * type)
{
	if (target_info == NULL || type == NULL){
		return NET_NFC_NULL_PARAMETER;
	}
	net_nfc_target_info_s * target_info_private = (net_nfc_target_info_s*) target_info;

	*type = target_info_private->devType;
	return NET_NFC_OK;
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_get_tag_handle(net_nfc_target_info_h target_info, net_nfc_target_handle_h * handle)
{
	if (target_info == NULL || handle == NULL){
		return NET_NFC_NULL_PARAMETER;
	}
	net_nfc_target_info_s * target_info_private = (net_nfc_target_info_s*) target_info;

	*handle = (net_nfc_target_handle_h) target_info_private->handle;
	return NET_NFC_OK;
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_get_tag_ndef_support (net_nfc_target_info_h target_info, bool * is_support)
{
	if (target_info == NULL || is_support == NULL){
		return NET_NFC_NULL_PARAMETER;
	}
	net_nfc_target_info_s * target_info_private = (net_nfc_target_info_s*) target_info;

	*is_support = (bool) target_info_private->is_ndef_supported;
	return NET_NFC_OK;
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_get_tag_max_data_size (net_nfc_target_info_h target_info, uint32_t * max_size)
{
	if (target_info == NULL || max_size == NULL){
		return NET_NFC_NULL_PARAMETER;
	}
	net_nfc_target_info_s * target_info_private = (net_nfc_target_info_s*) target_info;

	*max_size = target_info_private->maxDataSize;
	return NET_NFC_OK;
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_get_tag_actual_data_size (net_nfc_target_info_h target_info, uint32_t * actual_data)
{
	if (target_info == NULL || actual_data == NULL){
		return NET_NFC_NULL_PARAMETER;
	}
	net_nfc_target_info_s * target_info_private = (net_nfc_target_info_s*) target_info;

	*actual_data = target_info_private->actualDataSize;
	return NET_NFC_OK;
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_get_tag_info_keys(net_nfc_target_info_h target_info, char** keys[], int* number_of_keys)
{
	if(keys == NULL || number_of_keys == NULL || target_info == NULL)
	{
		return NET_NFC_NULL_PARAMETER;
	}

	net_nfc_target_info_s* handle = (net_nfc_target_info_s*)target_info;

	if(handle->tag_info_list == NULL)
	{
		return NET_NFC_NO_DATA_FOUND;
	}

	if(handle->number_of_keys <= 0)
	{
		return NET_NFC_NO_DATA_FOUND;
	}

	int i = 0;

	if (handle->keylist != NULL){
		*keys = handle->keylist;
		return NET_NFC_OK;
	}

	*keys = (char **)calloc(handle->number_of_keys, sizeof(char *));

	if(*keys == NULL)
		return NET_NFC_ALLOC_FAIL;

	net_nfc_tag_info_s* tag_info = handle->tag_info_list;

	for(; i < handle->number_of_keys; i++, tag_info++)
	{
		(*keys)[i] = tag_info->key;
	}

	*number_of_keys = handle->number_of_keys;

	DEBUG_CLIENT_MSG("number of keys = [%d]", handle->number_of_keys);

	handle->keylist = *keys;

	return NET_NFC_OK;
}


NET_NFC_EXPORT_API net_nfc_error_e net_nfc_get_tag_info_value(net_nfc_target_info_h target_info, const char* key, data_h* value)
{
	if(target_info == NULL || key == NULL || value == NULL)
	{
		return NET_NFC_NULL_PARAMETER;
	}

	net_nfc_target_info_s* handle = (net_nfc_target_info_s*)target_info;

	if(handle->tag_info_list == NULL)
	{
		return NET_NFC_NO_DATA_FOUND;
	}

	int i = 0;

	net_nfc_tag_info_s* tag_info = handle->tag_info_list;

	for(; i < handle->number_of_keys; i++, tag_info++)
	{
		if(strcmp(key, tag_info->key) == 0)
		{
			if(tag_info->value == NULL)
			{
				return NET_NFC_NO_DATA_FOUND;
			}
			else
			{
				*value = tag_info->value;
				break;
			}
		}
	}

	if(i == handle->number_of_keys)
		return NET_NFC_NOT_SUPPORTED;

	return NET_NFC_OK;
}
