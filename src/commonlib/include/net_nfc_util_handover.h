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

#ifndef __NET_NFC_UTIL_HANDOVER__
#define __NET_NFC_UTIL_HANDOVER__

#include "net_nfc_typedef_private.h"

#ifdef __cplusplus
extern "C"
{
#endif

net_nfc_error_e net_nfc_util_create_carrier_config(net_nfc_carrier_config_s **config, net_nfc_conn_handover_carrier_type_e type);

net_nfc_error_e net_nfc_util_add_carrier_config_property(net_nfc_carrier_config_s *config, uint16_t attribute, uint16_t size, uint8_t *data);

net_nfc_error_e net_nfc_util_remove_carrier_config_property(net_nfc_carrier_config_s *config, uint16_t attribute);

net_nfc_error_e net_nfc_util_get_carrier_config_property(net_nfc_carrier_config_s *config, uint16_t attribute, uint16_t *size, uint8_t **data);

net_nfc_error_e net_nfc_util_append_carrier_config_group(net_nfc_carrier_config_s *config, net_nfc_carrier_property_s *group);

net_nfc_error_e net_nfc_util_remove_carrier_config_group(net_nfc_carrier_config_s *config, net_nfc_carrier_property_s *group);

net_nfc_error_e net_nfc_util_get_carrier_config_group(net_nfc_carrier_config_s *config, int index, net_nfc_carrier_property_s **group);

net_nfc_error_e net_nfc_util_free_carrier_config(net_nfc_carrier_config_s *config);

net_nfc_error_e net_nfc_util_create_carrier_config_group(net_nfc_carrier_property_s **group, uint16_t attribute);

net_nfc_error_e net_nfc_util_add_carrier_config_group_property(net_nfc_carrier_property_s *group, uint16_t attribute, uint16_t size, uint8_t *data);

net_nfc_error_e net_nfc_util_get_carrier_config_group_property(net_nfc_carrier_property_s *group, uint16_t attribute, uint16_t *size, uint8_t **data);

net_nfc_error_e net_nfc_util_remove_carrier_config_group_property(net_nfc_carrier_property_s *group, uint16_t attribute);

net_nfc_error_e net_nfc_util_free_carrier_group(net_nfc_carrier_property_s *group);

net_nfc_error_e net_nfc_util_create_ndef_record_with_carrier_config(ndef_record_s **record, net_nfc_carrier_config_s *config);

net_nfc_error_e net_nfc_util_create_carrier_config_from_config_record(net_nfc_carrier_config_s **config, ndef_record_s *record);

net_nfc_error_e net_nfc_util_append_carrier_config_record(ndef_message_s *message, ndef_record_s *record, net_nfc_conn_handover_carrier_state_e power_status);

net_nfc_error_e net_nfc_util_remove_carrier_config_record(ndef_message_s *message, ndef_record_s *record);

net_nfc_error_e net_nfc_util_get_carrier_config_record(ndef_message_s *message, int index, ndef_record_s **record);

net_nfc_error_e net_nfc_util_get_handover_random_number(ndef_message_s *message, unsigned short *random_number);

net_nfc_error_e net_nfc_util_get_alternative_carrier_record_count(ndef_message_s *message, unsigned int *count);

net_nfc_error_e net_nfc_util_get_alternative_carrier_power_status(ndef_message_s *message, int index, net_nfc_conn_handover_carrier_state_e *power_state);

net_nfc_error_e net_nfc_util_set_alternative_carrier_power_status(ndef_message_s *message, int index, net_nfc_conn_handover_carrier_state_e power_status);

net_nfc_error_e net_nfc_util_get_alternative_carrier_type_from_record(ndef_record_s *record, net_nfc_conn_handover_carrier_type_e *type);

/**
 this function will get carrier type.

 @param[in] 	carrier_info 			connection handover carrier info handler
 @param[in] 	carrier_type 			record type. it can be a NET_NFC_CONN_HANDOVER_CARRIER_BT or NET_NFC_CONN_HANDOVER_CARRIER_WIFI or NET_NFC_CONN_HANDOVER_CARRIER_UNKNOWN.

 @return		return the result of the calling the function

 @exception NET_NFC_NULL_PARAMETER		parameter(s) has(have) illegal NULL pointer(s)
 */
net_nfc_error_e net_nfc_util_get_alternative_carrier_type(ndef_message_s *message, int index, net_nfc_conn_handover_carrier_type_e *power_state);

net_nfc_error_e net_nfc_util_create_handover_request_message(ndef_message_s **message);

net_nfc_error_e net_nfc_util_create_handover_select_message(ndef_message_s **message);

net_nfc_error_e net_nfc_util_create_handover_error_record(ndef_record_s **record, uint8_t reason, uint32_t data);

net_nfc_error_e net_nfc_util_get_selector_power_status(ndef_message_s *message, net_nfc_conn_handover_carrier_state_e *power_state);

#ifdef __cplusplus
}
#endif

#endif
