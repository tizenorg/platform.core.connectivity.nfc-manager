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
#ifndef __NET_NFC_TARGET_INFO_H__
#define __NET_NFC_TARGET_INFO_H__

#include "net_nfc_typedef.h"

/**

  @addtogroup NET_NFC_MANAGER_INFO
  @{
  This document is for the APIs reference document

  NFC Manager defines are defined in <nfc-typedef.h>

  These APIs help to get infomation of detected target. these target info handler holds
  - type of target
  - target ID
  - ndef message supporting
  - max data size  (if this tag is ndef message tag)
  - actual data size (if this tag is ndef message tag)
  */

/**
  type getter from targte info handler

  \par Sync (or) Async: Sync
  This is a Synchronous API

  @param[in]	target_info 	target info handler
  @param[out]	type			tag type these type is one of the enum "net_nfc_target_type_e" defined

  @return		return the result of calling this functions

  @exception 	NET_NFC_NULL_PARAMETER		parameter(s) has(have) illigal NULL pointer(s)

*/

net_nfc_error_e net_nfc_get_tag_type(net_nfc_target_info_h target_info,
		net_nfc_target_type_e *type);

/**
  type getter from targte info handler

  \par Sync (or) Async: Sync
  This is a Synchronous API

  @param[in]	target_info 	target info handler
  @param[out]	handle		target handle  that is generated by nfc-manager

  @return		return the result of calling this functions

  @exception 	NET_NFC_NULL_PARAMETER		parameter(s) has(have) illigal NULL pointer(s)

*/

net_nfc_error_e net_nfc_get_tag_handle(net_nfc_target_info_h target_info,
		net_nfc_target_handle_h *handle);

/**
  this API returns the NDEF support boolean value.
  The TRUE value will be returned if the detected target is support NDEF, or return FALSE

  \par Sync (or) Async: Sync
  This is a Synchronous API

  @param[in]	target_info 	target info handler
  @param[out]	is_support	boolean value of NDEF supporting

  @return		return the result of calling this functions

  @exception 	NET_NFC_NULL_PARAMETER		parameter(s) has(have) illigal NULL pointer(s)

*/

net_nfc_error_e net_nfc_get_tag_ndef_support(net_nfc_target_info_h target_info,
		bool *is_support);

/**
  The max size getter from targte info handler. This max size indicates the maximum size of NDEF message that can be stored in this detected tag.

  \par Sync (or) Async: Sync
  This is a Synchronous API

  @param[in]	target_info 	target info handler
  @param[out]	max_size		max size of NDEF message that can be stored in this detected tag.

  @return		return the result of calling this functions

  @exception 	NET_NFC_NULL_PARAMETER		parameter(s) has(have) illigal NULL pointer(s)

*/

net_nfc_error_e net_nfc_get_tag_max_data_size(net_nfc_target_info_h target_info,
		uint32_t *max_size);

/**
  this function return the actual NDEF message size that stored in the tag

  \par Sync (or) Async: Sync
  This is a Synchronous API

  @param[in]	target_info 		target info handler
  @param[out]	actual_data		the actual NDEF message size that stored in the tag

  @return		return the result of calling this functions

  @exception 	NET_NFC_NULL_PARAMETER		parameter(s) has(have) illigal NULL pointer(s)

*/

net_nfc_error_e net_nfc_get_tag_actual_data_size(
		net_nfc_target_info_h target_info, uint32_t *actual_data);


/**
  this function return keys which will be used to get a tag information

  \par Sync (or) Async: Sync
  This is a Synchronous API. keys will be freed by user.

  @param[in]	target_info 		target info handler
  @param[out]	keys 			pointer of double array. it will be array of string.
  @param[out]	number_of_keys	length of array.

  @exception 	NET_NFC_NULL_PARAMETER		parameter(s) has(have) illigal NULL pointer(s)
  @exception 	NET_NFC_ALLOC_FAIL			memory allocation is failed
  @exception 	NET_NFC_NO_DATA_FOUND		No data is returned

  @code
  void	user_cb(net_nfc_message_e message, net_nfc_error_e result, void* data, void* trans_param)
  {

    switch(message)
    {
      case NET_NFC_MESSAGE_TAG_DISCOVERED:
        if(info != NULL)
        {
          net_nfc_target_info_h target_info = (net_nfc_target_info_h)data;

          char **keys;
          int keys_length;

          if(net_nfc_get_tag_info_keys(target_info, &keys, &keys_length) == true)
          {
            int index = 0;
            for(; index < keys_length; index++)
            {
              char* key = keys[index];
            }
          }

          free(keys);
        }
    }
  }
  @endcode

  @return		return the result of calling this functions

*/

net_nfc_error_e net_nfc_get_tag_info_keys(net_nfc_target_info_h target_info,
		char ***keys, int *number_of_keys);

/**
  this function return value which is matched key

  \par Sync (or) Async: Sync
  This is a Synchronous API

  @param[in]	target_info 		target info handler
  @param[in]	key				key to retrieve
  @param[out]	value			value which is matched with key

  @code
  void	user_cb(net_nfc_message_e message, net_nfc_error_e result, void* data, void* trans_param)
  {
    switch(message)
    {
      case NET_NFC_MESSAGE_TAG_DISCOVERED:
        if(info != NULL)
        {
          net_nfc_target_info_h target_info = (net_nfc_target_info_h)data;

          char** keys;
          int keys_length;

          if(net_nfc_get_tag_info_keys(target_info, &keys, &keys_length) == NET_NFC_OK)
          {
            int index = 0;
            for(; index < keys_length; index++)
            {
              char* key = keys[index];
              data_h value;
              net_nfc_get_tag_info_value(target_info, key, &value);
              net_nfc_free_data(value);
            }
          }
        }
    }
  }
  @endcode

  @return		return the result of calling this functions

  @exception 	NET_NFC_NULL_PARAMETER		parameter(s) has(have) illegal NULL pointer(s)
  @exception 	NET_NFC_NO_DATA_FOUND		No data is returned
  */

net_nfc_error_e net_nfc_get_tag_info_value(net_nfc_target_info_h target_info,
		const char *key, data_h *value);

/**
  Duplicate a handle of target information

  ** IMPORTANT : After using duplicated handle, you should release a handle returned from this function.
  **             You can release a handle by net_nfc_release_tag_info function.

  \par Sync (or) Async: Sync
  This is a Synchronous API

  @param[in]	origin 		The original handle you want to duplicate
  @param[out]	result 		The result of this function.

  @code
  void user_cb(net_nfc_message_e message, net_nfc_error_e result, void *data, void *trans_param)
  {
    switch(message)
    {
      case NET_NFC_MESSAGE_TAG_DISCOVERED:
        net_nfc_target_info_h target_info = (net_nfc_target_info_h)data;
        net_nfc_target_info_h handle = NULL;

        net_nfc_duplicate_target_info(target_info, &handle);

        // do something...

        net_nfc_release_tag_info(handle);
        break;
    }
  }
  @endcode

  @return		return the result of calling this functions
  @exception 	NET_NFC_NULL_PARAMETER		parameter(s) has(have) illegal NULL pointer(s)
*/

net_nfc_error_e net_nfc_duplicate_target_info(net_nfc_target_info_h origin,
		net_nfc_target_info_h *result);

/**
  After using net_nfc_target_info_h handle, you should release its resource by this function.

  ** IMPORTANT : Never use this function in user callback you registered by net_nfc_set_response_callback function
  **             This function is for the result of net_nfc_duplicate_target_info or net_nfc_get_current_tag_info_sync

  \par Sync (or) Async: Sync
  This is a Synchronous API

  @param[in]	target_info 		target info handle

  @code
  net_nfc_target_info_h handle;

  net_nfc_get_current_tag_info_sync(&handle);

  // do something...

  net_nfc_release_tag_info(handle);
  @endcode

  @return		return the result of calling this functions

  @exception 	NET_NFC_NULL_PARAMETER		parameter(s) has(have) illegal NULL pointer(s)
*/

net_nfc_error_e net_nfc_release_tag_info(net_nfc_target_info_h target_info);


/**
@}
*/

#endif //__NET_NFC_TARGET_INFO_H__
