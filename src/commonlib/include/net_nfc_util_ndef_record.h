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

#ifndef __NET_NFC_UTIL_NDEF_RECORD__
#define __NET_NFC_UTIL_NDEF_RECORD__

#include "net_nfc_typedef_private.h"

/*
 create record structure with basic info
 */
net_nfc_error_e net_nfc_util_create_record(net_nfc_record_tnf_e recordType, data_s *typeName, data_s *id, data_s *payload, ndef_record_s **record);

/*
 create text type record
 */
net_nfc_error_e net_nfc_util_create_text_type_record(const char *text, const char *lang_code_str, net_nfc_encode_type_e encode, ndef_record_s **record);

/*
 this utility function help to create uri type record
 */
net_nfc_error_e net_nfc_util_create_uri_type_record(const char *uri, net_nfc_schema_type_e protocol_schema, ndef_record_s **record);

/*
 free ndef record. it free all the buffered data
 */
net_nfc_error_e net_nfc_util_free_record(ndef_record_s *record);

/*
 convert schema enum value to character string.
 */
net_nfc_error_e net_nfc_util_set_record_id(ndef_record_s *record, uint8_t *data, int length);

/*
 get total bytes of ndef record in serial form
 */
uint32_t net_nfc_util_get_record_length(ndef_record_s *record);

#endif
