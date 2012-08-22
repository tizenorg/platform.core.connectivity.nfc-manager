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


#ifndef __NET_NFC_UTIL_NDEF_MESSAGE__
#define __NET_NFC_UTIL_NDEF_MESSAGE__

#include "net_nfc_typedef_private.h"

/*
 convert rawdata into ndef message structure
 */
net_nfc_error_e net_nfc_util_convert_rawdata_to_ndef_message(data_s *rawdata, ndef_message_s *ndef);

/*
 this util function converts into rawdata from ndef message structure
 */
net_nfc_error_e net_nfc_util_convert_ndef_message_to_rawdata(ndef_message_s *ndef, data_s *rawdata);

/*
 get total bytes of ndef message in serial form
 */
uint32_t net_nfc_util_get_ndef_message_length(ndef_message_s *message);

/*
 free ndef message. this function also free any defined buffer insdie structures
 */
net_nfc_error_e net_nfc_util_free_ndef_message(ndef_message_s *msg);

/*
 append record into ndef message
 */
net_nfc_error_e net_nfc_util_append_record(ndef_message_s *msg, ndef_record_s *record);

/*
 print out ndef structure value with printf function. this is for just debug purpose
 */
void net_nfc_util_print_ndef_message(ndef_message_s *msg);

net_nfc_error_e net_nfc_util_create_ndef_message(ndef_message_s **ndef_message);

net_nfc_error_e net_nfc_util_search_record_by_type(ndef_message_s *ndef_message, net_nfc_record_tnf_e tnf, data_s *type, ndef_record_s **record);

net_nfc_error_e net_nfc_util_append_record_by_index(ndef_message_s *ndef_message, int index, ndef_record_s *record);

net_nfc_error_e net_nfc_util_get_record_by_index(ndef_message_s *ndef_message, int index, ndef_record_s **record);

net_nfc_error_e net_nfc_util_remove_record_by_index(ndef_message_s *ndef_message, int index);

net_nfc_error_e net_nfc_util_search_record_by_id(ndef_message_s *ndef_message, data_s *id, ndef_record_s **record);

#endif

