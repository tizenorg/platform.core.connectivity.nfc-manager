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

#include "net_nfc_debug_private.h"
#include "net_nfc_util_private.h"
#include "net_nfc_util_ndef_message.h"

#include "net_nfc_ndef_message.h"
#include "net_nfc.h" // to use net_nfc_data


#ifndef NET_NFC_MANAGER_DATA_PATH
#define NET_NFC_MANAGER_DATA_PATH				"/opt/data/nfc-manager-daemon"
#endif

#ifndef NET_NFC_MANAGER_DATA_PATH_MESSAGE
#define NET_NFC_MANAGER_DATA_PATH_MESSAGE		"message"
#endif

#ifndef NET_NFC_MANAGER_NDEF_FILE_NAME
#define NET_NFC_MANAGER_NDEF_FILE_NAME			"ndef-message.txt"
#endif


#ifndef NET_NFC_EXPORT_API
#define NET_NFC_EXPORT_API __attribute__((visibility("default")))
#endif


/* public functions */
NET_NFC_EXPORT_API net_nfc_error_e net_nfc_create_ndef_message (ndef_message_h* ndef_message)
{
	return net_nfc_util_create_ndef_message ((ndef_message_s **) ndef_message);
}



NET_NFC_EXPORT_API net_nfc_error_e net_nfc_create_rawdata_from_ndef_message (ndef_message_h ndef_message, data_h* rawdata)
{
	if (ndef_message == NULL || rawdata == NULL){
		return NET_NFC_NULL_PARAMETER;
	}
	int count;
	net_nfc_error_e result;

	result = net_nfc_get_ndef_message_byte_length (ndef_message,&count);
	if (result  != NET_NFC_OK){
		return result;
	}

	result = net_nfc_create_data (rawdata ,NULL, count);
	if (result != NET_NFC_OK) {
		return result;
	}
	return net_nfc_util_convert_ndef_message_to_rawdata((ndef_message_s*)ndef_message, (data_s*)*rawdata);
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_create_ndef_message_from_rawdata (ndef_message_h* ndef_message, data_h  rawdata)
{
	if (ndef_message == NULL || rawdata == NULL ){
		return NET_NFC_NULL_PARAMETER;
	}

	net_nfc_error_e result;
	result = net_nfc_create_ndef_message (ndef_message);
	if (result  != NET_NFC_OK){
		return result;
	}

	return net_nfc_util_convert_rawdata_to_ndef_message((data_s*)rawdata, (ndef_message_s*)*ndef_message);
}


NET_NFC_EXPORT_API net_nfc_error_e net_nfc_get_ndef_message_byte_length(ndef_message_h ndef_message, int * length)
{
	if (ndef_message == NULL || length == NULL){
		return NET_NFC_NULL_PARAMETER;
	}

	* length = net_nfc_util_get_ndef_message_length((ndef_message_s*)ndef_message);
	return NET_NFC_OK;
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_append_record_to_ndef_message(ndef_message_h ndef_message, ndef_record_h  record)
{
	if (ndef_message == NULL || record == NULL){
		return NET_NFC_NULL_PARAMETER;
	}

	return net_nfc_util_append_record((ndef_message_s*)ndef_message, (ndef_record_s *)record);
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_free_ndef_message(ndef_message_h ndef_message)
{
	if (ndef_message == NULL){
		return NET_NFC_NULL_PARAMETER;
	}
	net_nfc_error_e error = net_nfc_util_free_ndef_message((ndef_message_s *)ndef_message);
	if (error != NET_NFC_OK){
		return error;
	}
	return error;
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_get_ndef_message_record_count (ndef_message_h ndef_message, int * count)
{
	if (ndef_message == NULL || count == NULL){
		return NET_NFC_NULL_PARAMETER;
	}
	ndef_message_s * ndef_message_private = (ndef_message_s *)ndef_message;

	* count = ndef_message_private->recordCount;

	return NET_NFC_OK;
}

NET_NFC_EXPORT_API void net_nfc_ndef_print_message (ndef_message_h ndef_message )
{
	net_nfc_util_print_ndef_message ((ndef_message_s *)(ndef_message) );
}


NET_NFC_EXPORT_API net_nfc_error_e net_nfc_search_record_by_type (ndef_message_h ndef_message, net_nfc_record_tnf_e tnf, data_h type, ndef_record_h* record)
{
	return net_nfc_util_search_record_by_type ((ndef_message_s*)ndef_message, tnf, (data_s *)type, (ndef_record_s**)record);
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_append_record_by_index (ndef_message_h ndef_message, int index, ndef_record_h record)
{
	return net_nfc_util_append_record_by_index ((ndef_message_s *) ndef_message, index, (ndef_record_s *) record);
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_get_record_by_index (ndef_message_h ndef_message, int index, ndef_record_h*  record)
{
	return net_nfc_util_get_record_by_index ((ndef_message_s*) ndef_message, index, (ndef_record_s**) record);
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_remove_record_by_index (ndef_message_h ndef_message, int index)
{
	return net_nfc_util_remove_record_by_index ((ndef_message_s*)ndef_message, index);
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_retrieve_current_ndef_message(ndef_message_h* ndef_message)
{
	net_nfc_error_e result = NET_NFC_UNKNOWN_ERROR;
	char file_path[1024] = { 0, };
	FILE *fp = NULL;

	if (ndef_message == NULL)
	{
		return NET_NFC_NULL_PARAMETER;
	}

	snprintf(file_path, sizeof(file_path), "%s/%s/%s", NET_NFC_MANAGER_DATA_PATH, NET_NFC_MANAGER_DATA_PATH_MESSAGE, NET_NFC_MANAGER_NDEF_FILE_NAME);

	if ((fp = fopen(file_path, "r")) != NULL)
	{
		uint8_t *buffer = NULL;
		long int size = 0;

		/* rewind to start of file */
		fseek(fp, 0, SEEK_END);
		size = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		DEBUG_CLIENT_MSG("message length = [%d] \n", size);

		if ((buffer = (uint8_t *)calloc(1, size)) != NULL)
		{
			/* read fully */
			if ((size = fread(buffer, 1, size, fp)) > 0)
			{
				data_h data = NULL;
				if ((result = net_nfc_create_data(&data, buffer, size)) == NET_NFC_OK)
				{
					result = net_nfc_create_ndef_message_from_rawdata(ndef_message, data);

					net_nfc_free_data(data);
				}
			}

			free(buffer);
		}
		else
		{
			result = NET_NFC_ALLOC_FAIL;
		}

		fclose(fp);
	}
	else
	{
		result = NET_NFC_NO_NDEF_MESSAGE;
	}

	return result;
}


