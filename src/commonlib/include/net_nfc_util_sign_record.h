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

#ifndef __NET_NFC_UTIL_SIGN_RECORD_H
#define __NET_NFC_UTIL_SIGN_RECORD_H

#include "net_nfc_typedef_private.h"

/*
 * sign ndef record and ndef message
 */
net_nfc_error_e net_nfc_util_sign_records(ndef_message_s *msg, int begin_index, int end_index, char *cert_file, char *password);
net_nfc_error_e net_nfc_util_sign_ndef_message(ndef_message_s *msg, char *cert_file, char *password);

/*
 * check validity of ndef record and ndef message
 */
net_nfc_error_e net_nfc_util_verify_signature_ndef_message(ndef_message_s *msg);
net_nfc_error_e net_nfc_util_verify_signature_records(ndef_record_s *begin_record, ndef_record_s *sign_record);


#endif /* __NET_NFC_UTIL_SIGN_RECORD_H */

