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


#ifndef __SLP_NET_NFC_EXCHANGER_H__
#define __SLP_NET_NFC_EXCHANGER_H__


#include "net_nfc_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
@addtogroup NET_NFC_MANAGER_EXCHANGE
@{

*/


/**
	create net_nfc_exchagner raw type data handler with given values

	@param[out]	ex_data 		exchangner handler
	@param[in]	payload		the data will be deliver (NDEF message)

	@return 		result of this function call

	@exception NET_NFC_ALLOC_FAIL			memory allocation is failed
	@exception NET_NFC_NULL_PARAMETER		parameter(s) has(have) illegal NULL pointer(s)
*/
net_nfc_error_e net_nfc_create_exchanger_data(net_nfc_exchanger_data_h *ex_data, data_h payload);

/**
	this makes free exchagner data handler

	@param[in]	ex_data 		exchagner handler

	@return 		result of this function call

	@exception NET_NFC_NULL_PARAMETER		parameter(s) has(have) illigal NULL pointer(s)
*/

net_nfc_error_e net_nfc_free_exchanger_data (net_nfc_exchanger_data_h ex_data);

/**

	register callback function and initialize the exchange API. this function also make connection to the NFC-manager
	only one functionality of net_nfc_initialize and net_nfc_set_exchanger_cb is permitted in a running time.
	So, call net_nfc_deinitailze if net_nfc_initialize is called before when you want to register exchanger callback.


	@param[in]		callback 		callback function will be used in event
	@param[in]		user_param	this data will be deliver to callback function

	@return		return the result of the calling the function

	@exception 	NET_NFC_NOT_ALLOWED_OPERATION		net_nfc_initailzed is called before. so call net_nfc_deinitialize first.

*/
net_nfc_error_e net_nfc_set_exchanger_cb (net_nfc_exchanger_cb callback, void * user_param);

/**
	disconnect the connection from nfc-manager server and stop calling the callback function

	@return 	return the result of the calling the function

	@exception NET_NFC_NOT_REGISTERED	unset is requested but the callback was not registered before
*/

net_nfc_error_e net_nfc_unset_exchanger_cb (void);

/**
	discovered P2P device and send the NET_NFC_MESSAGE_P2P_SEND message to server  from client.


	@param[in]	ex_data 		       exchangner handler
	@param[in]	target_handle 		target device handle

	@return 	      return the result of the calling the function

	@exception NET_NFC_ALLOC_FAIL	memory allocation is failed
*/

net_nfc_error_e net_nfc_send_exchanger_data (net_nfc_exchanger_data_h ex_handle, net_nfc_target_handle_h target_handle);

/**
	request connection handover with discovered P2P device

	@param[in]	target_handle 		target device handle
	@param[in]	type 		specific alternative carrier type (if type is NET_NFC_CONN_HANDOVER_CARRIER_UNKNOWN, it will be selected available type of this target)

	@return 		result of this function call

	@exception NET_NFC_ALLOC_FAIL			memory allocation is failed
	@exception NET_NFC_NULL_PARAMETER		parameter(s) has(have) illigal NULL pointer(s)
*/
net_nfc_error_e net_nfc_exchanger_request_connection_handover(net_nfc_target_handle_h target_handle, net_nfc_conn_handover_carrier_type_e type);

/**
	get alternative carrier type from connection handover information handle.

	@param[in] 	info_handle		connection handover information handle
	@param[out] 	type	alternative carrier type

	@return 	return the result of this operation

	@exception NET_NFC_NULL_PARAMETER		parameter(s) has(have) illigal NULL pointer(s)
*/
net_nfc_error_e net_nfc_exchanger_get_alternative_carrier_type(net_nfc_connection_handover_info_h info_handle, net_nfc_conn_handover_carrier_type_e *type);

/**
	get alternative carrier dependant data from connection handover information handle.
	Bluetooth : target device address
	Wifi : target device ip address

	@param[in] 	info_handle		connection handover information handle
	@param[out] 	data	alternative carrier data

	@return 	return the result of this operation

	@exception NET_NFC_ALLOC_FAIL			memory allocation is failed
	@exception NET_NFC_NULL_PARAMETER		parameter(s) has(have) illigal NULL pointer(s)
*/
net_nfc_error_e net_nfc_exchanger_get_alternative_carrier_data(net_nfc_connection_handover_info_h info_handle, data_h *data);

/**
	this makes free alternative carrier data handler

	@param[in]	info_handle 		alternative carrier data handler

	@return 		result of this function call

	@exception NET_NFC_NULL_PARAMETER		parameter(s) has(have) illigal NULL pointer(s)
*/
net_nfc_error_e net_nfc_exchanger_free_alternative_carrier_data(net_nfc_connection_handover_info_h  info_handle);


/**
@}

*/


#ifdef __cplusplus
}
#endif


#endif
