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

#include <net_nfc_typedef.h>


#ifndef __NET_NFC_DATA_H__
#define __NET_NFC_DATA_H__

#ifdef __cplusplus
extern "C" {
#endif


/**

@addtogroup NET_NFC_MANAGER_INFO
@{
	This document is for the APIs reference document

        NFC Manager defines are defined in <nfc-manager-def.h>

        @li @c #net_nfc_initialize                  Initialize the nfc device.

*/

/**
	create data handler only.

	@param[out] 	data		data handler

	@return 	return the result of this operation

	@exception NET_NFC_NULL_PARAMETER		parameter(s) has(have) illigal NULL pointer(s)
	@exception NET_NFC_ALLOC_FAIL			memory allocation is failed
*/

net_nfc_error_e net_nfc_create_data_only (data_h* data);

/**
	create data handler with initial values, bytes will be copied into the data handler.

	@param[out] 	data		data handler
	@param[in] 	bytes	binary data
	@param[in]	length	size of binary data;

	@return 	return the result of this operation

	@exception NET_NFC_NULL_PARAMETER		parameter(s) has(have) illigal NULL pointer(s)
	@exception NET_NFC_ALLOC_FAIL			memory allocation is failed
*/
net_nfc_error_e net_nfc_create_data (data_h* data, const uint8_t* bytes, const uint32_t length);

/**
	get the byes and length from data handler. data handler assume bytes may have '0x0' value.
	that's why this function also provides the length.

	@param[in] 	data		data handler
	@param[out]	bytes	binary pointer (it returns the direct pointer of handler's data) do not free this
	@param[out]	length	length of the binary data;

	@return 	return the result of this operation

	@exception NET_NFC_NULL_PARAMETER		parameter(s) has(have) illigal NULL pointer(s)
*/
net_nfc_error_e net_nfc_get_data (const data_h data, uint8_t** bytes, uint32_t * length);

/**
	replace the data handler with given bytes. binary data (bytes) will be copied to data hander.
	application should free or use local variable for given byte pointer.

	@param[in] 	data		data handler
	@param[in] 	bytes	binary data
	@param[in]	length	size of binary data

	@return 	return the result of this operation

	@exception NET_NFC_NULL_PARAMETER		parameter(s) has(have) illigal NULL pointer(s)
*/

net_nfc_error_e net_nfc_set_data (const data_h data, const uint8_t* bytes, const uint32_t length);

/**
	get length of data handler's bytes.

	@param[in] 	data		data handler

	@return 	length of bytes length

	@exception 	0 is returned if data is NULL
*/

uint32_t net_nfc_get_data_length (const data_h data);

/**
	get pointer of the handler's bytes (do not free this. it wll be freed when the application call "net_nfc_free_data" function

	@param[in] 	data		data handler

	@return 	the pointer of bytes.

	@exception 	NULL is returned if data is NULL
*/

uint8_t * net_nfc_get_data_buffer (const data_h data);

/**
	free data handler. (it also free the copied bytes)

	@param[in] 	data		data handler

	@return 	return the result of this operation

	@exception NET_NFC_NULL_PARAMETER		parameter(s) has(have) illigal NULL pointer(s)
*/

net_nfc_error_e net_nfc_free_data (data_h data);



/**
@}
*/
#ifdef __cplusplus
}
#endif


#endif

