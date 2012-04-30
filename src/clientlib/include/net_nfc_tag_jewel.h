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

#ifndef __NET_NFC_TAG_JEWEL_H__
#define __NET_NFC_TAG_JEWEL_H__

#include "net_nfc_typedef.h"


#ifdef __cplusplus
 extern "C" {
#endif

/**

@addtogroup NET_NFC_MANAGER_TAG
@{

	read uid from jewel tag.

	\par Sync (or) Async: Sync
	This is a Asynchronous API

	@param[in] 	handle		target handle of detected tag

	@return		return the result of the calling the function

	@exception NET_NFC_NULL_PARAMETER	parameter has illigal NULL pointer
	@exception NET_NFC_ALLOC_FAIL 	memory allocation is failed
	@exception NET_NFC_NOT_INITIALIZED	Try to operate without initialization
	@exception NET_NFC_BUSY		Device is too busy to handle your request
	@exception NET_NFC_OPERATION_FAIL	Operation is failed because of the internal oal error
	@exception NET_NFC_RF_TIMEOUT	Timeout is raised while communicate with tag
	@exception NET_NFC_NOT_SUPPORTED	you may recieve this error if you request not supported command
	@exception NET_NFC_INVALID_HANDLE	target handle is not valid
	@exception NET_NFC_TAG_READ_FAILED	received chunked data is not valied (damaged data is recieved) or error ack is recieved
	@exception NET_NFC_NO_DATA_FOUND	mendantory tag info (UID, etc) is not founded

*/

net_nfc_error_e net_nfc_jewel_read_id (net_nfc_target_handle_h handle, void* trans_param);

/**
	read one byte of specific address .

	\par Sync (or) Async: Sync
	This is a Asynchronous API

	@param[in] 	handle		target handle of detected tag
	@param[in] 	block		block number. (block 0 ~ block E)
	@param[in] 	byte			byte number. Each block has 8 bytes. (byte 0 ~ byte 7)

	@return		return the result of the calling the function

	@exception NET_NFC_NULL_PARAMETER	parameter has illigal NULL pointer
	@exception NET_NFC_ALLOC_FAIL 	memory allocation is failed
	@exception NET_NFC_NOT_INITIALIZED	Try to operate without initialization
	@exception NET_NFC_BUSY		Device is too busy to handle your request
	@exception NET_NFC_OPERATION_FAIL	Operation is failed because of the internal oal error
	@exception NET_NFC_RF_TIMEOUT	Timeout is raised while communicate with tag
	@exception NET_NFC_NOT_SUPPORTED	you may recieve this error if you request not supported command
	@exception NET_NFC_INVALID_HANDLE	target handle is not valid
	@exception NET_NFC_TAG_READ_FAILED	received chunked data is not valied (damaged data is recieved) or error ack is recieved
	@exception NET_NFC_NO_DATA_FOUND	mendantory tag info (UID, etc) is not founded

*/

net_nfc_error_e net_nfc_jewel_read_byte (net_nfc_target_handle_h handle, uint8_t block, uint8_t byte, void* trans_param);

/**
	read all byte from tag .

	\par Sync (or) Async: Sync
	This is a Asynchronous API

	@param[in] 	handle		target handle of detected tag

	@return		return the result of the calling the function

	@exception NET_NFC_NULL_PARAMETER	parameter has illigal NULL pointer
	@exception NET_NFC_ALLOC_FAIL 	memory allocation is failed
	@exception NET_NFC_NOT_INITIALIZED	Try to operate without initialization
	@exception NET_NFC_BUSY		Device is too busy to handle your request
	@exception NET_NFC_OPERATION_FAIL	Operation is failed because of the internal oal error
	@exception NET_NFC_RF_TIMEOUT	Timeout is raised while communicate with tag
	@exception NET_NFC_NOT_SUPPORTED	you may recieve this error if you request not supported command
	@exception NET_NFC_INVALID_HANDLE	target handle is not valid
	@exception NET_NFC_TAG_READ_FAILED	received chunked data is not valied (damaged data is recieved) or error ack is recieved
	@exception NET_NFC_NO_DATA_FOUND	mendantory tag info (UID, etc) is not founded

*/


net_nfc_error_e net_nfc_jewel_read_all (net_nfc_target_handle_h handle, void* trans_param);


/**
	operate erase and write cycle . If any of BLOCK-0 to BLOCK-D is locked then write with erase is barred form thoes blocks.
	Additionally 0, D, E blocks are automatically in the lock condition. so write with erase is always barred from thoes blocks.

	\par Sync (or) Async: Sync
	This is a Asynchronous API

	@param[in] 	handle		target handle of detected tag
	@param[in] 	block		block number. (block 0 ~ block E)
	@param[in] 	data			the data to write
	@param[in] 	byte			byte number. Each block has 8 bytes. (byte 0 ~ byte 7)

	@return		return the result of the calling the function

	@exception NET_NFC_NULL_PARAMETER	parameter has illigal NULL pointer
	@exception NET_NFC_ALLOC_FAIL 	memory allocation is failed
	@exception NET_NFC_NOT_INITIALIZED	Try to operate without initialization
	@exception NET_NFC_BUSY		Device is too busy to handle your request
	@exception NET_NFC_OPERATION_FAIL	Operation is failed because of the internal oal error
	@exception NET_NFC_RF_TIMEOUT	Timeout is raised while communicate with tag
	@exception NET_NFC_NOT_SUPPORTED	you may recieve this error if you request not supported command
	@exception NET_NFC_INVALID_HANDLE	target handle is not valid
	@exception NET_NFC_TAG_WRITE_FAILED	read only tag, or error ack is received from tag
	@exception NET_NFC_NO_DATA_FOUND	mendantory tag info (UID, etc) is not founded

*/

net_nfc_error_e net_nfc_jewel_write_with_erase (net_nfc_target_handle_h handle, uint8_t block, uint8_t byte, uint8_t data, void* trans_param);


/**
	operate no erase and write cycle .

	The WRITE-NE command is available for three main purposes
		- Lock . to set the ��lock bit�� for a block
		- OTP . to set One-Time-Programmable bits (bytes 2 . 7 of Block-E), where between one and eight OTP bits can be set with a singleWRITE-NE command
		- A fast-write in order to reduce overall time to write data to memory blocks for the first time given that the original condition of memory is zero

	\par Sync (or) Async: Sync
	This is a Asynchronous API

	@param[in] 	handle		target handle of detected tag
	@param[in] 	block		block number. (block 0 ~ block E)
	@param[in] 	data			the data to write
	@param[in] 	byte			byte number. Each block has 8 bytes. (byte 0 ~ byte 7)

	@return		return the result of the calling the function

	@exception NET_NFC_NULL_PARAMETER	parameter has illigal NULL pointer
	@exception NET_NFC_ALLOC_FAIL 	memory allocation is failed
	@exception NET_NFC_NOT_INITIALIZED	Try to operate without initialization
	@exception NET_NFC_BUSY		Device is too busy to handle your request
	@exception NET_NFC_OPERATION_FAIL	Operation is failed because of the internal oal error
	@exception NET_NFC_RF_TIMEOUT	Timeout is raised while communicate with tag
	@exception NET_NFC_NOT_SUPPORTED	you may recieve this error if you request not supported command
	@exception NET_NFC_INVALID_HANDLE	target handle is not valid
	@exception NET_NFC_TAG_WRITE_FAILED	read only tag, or error ack is received from tag
	@exception NET_NFC_NO_DATA_FOUND	mendantory tag info (UID, etc) is not founded

*/

net_nfc_error_e net_nfc_jewel_write_with_no_erase (net_nfc_target_handle_h handle, uint8_t block, uint8_t byte, uint8_t data, void* trans_param);

/**
@} */


#ifdef __cplusplus
}
#endif


#endif

