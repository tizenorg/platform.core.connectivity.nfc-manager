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

#include "net_nfc_ndef_message_handover.h"

#include "net_nfc_typedef_private.h"
#include "net_nfc_debug_private.h"
#include "net_nfc_util_private.h"
#include "net_nfc_util_handover.h"

#ifndef NET_NFC_EXPORT_API
#define NET_NFC_EXPORT_API __attribute__((visibility("default")))
#endif

#include <glib.h>

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_create_carrier_config (net_nfc_carrier_config_h * config, net_nfc_conn_handover_carrier_type_e type)
{
	return  net_nfc_util_create_carrier_config ((net_nfc_carrier_config_s **) config, type);
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_add_carrier_config_property (net_nfc_carrier_config_h config, uint16_t attribute, uint16_t size, uint8_t * data)
{
	return net_nfc_util_add_carrier_config_property ((net_nfc_carrier_config_s *) config, attribute, size, data);
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_remove_carrier_config_property (net_nfc_carrier_config_h config, uint16_t attribute)
{
	return net_nfc_util_remove_carrier_config_property ((net_nfc_carrier_config_s *) config, attribute);
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_get_carrier_config_property (net_nfc_carrier_config_h config, uint16_t attribute, uint16_t * size, uint8_t ** data)
{
	return net_nfc_util_get_carrier_config_property ((net_nfc_carrier_config_s *) config, attribute, size, data);
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_append_carrier_config_group (net_nfc_carrier_config_h config, net_nfc_property_group_h group)
{
	return net_nfc_util_append_carrier_config_group ((net_nfc_carrier_config_s *) config, (net_nfc_carrier_property_s *) group);
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_remove_carrier_config_group (net_nfc_carrier_config_h config, net_nfc_property_group_h group)
{
	return net_nfc_util_remove_carrier_config_group ((net_nfc_carrier_config_s *) config, (net_nfc_carrier_property_s *) group);
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_get_carrier_config_group (net_nfc_carrier_config_h config, int index, net_nfc_property_group_h * group)
{
	return net_nfc_util_get_carrier_config_group ((net_nfc_carrier_config_s *) config, index, (net_nfc_carrier_property_s **) group);
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_free_carrier_config (net_nfc_carrier_config_h config)
{
	return net_nfc_util_free_carrier_config ((net_nfc_carrier_config_s *) config);
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_create_carrier_config_group (net_nfc_property_group_h * group, uint16_t attribute)
{
	return net_nfc_util_create_carrier_config_group ((net_nfc_carrier_property_s **) group, attribute);
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_add_carrier_config_group_property (net_nfc_property_group_h group, uint16_t attribute, uint16_t size, uint8_t * data)
{
	return net_nfc_util_add_carrier_config_group_property ((net_nfc_carrier_property_s*) group, attribute, size, data);
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_get_carrier_config_group_property (net_nfc_property_group_h group, uint16_t attribute, uint16_t *size, uint8_t ** data)
{
	return net_nfc_util_get_carrier_config_group_property ((net_nfc_carrier_property_s*) group, attribute, size, data);
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_remove_carrier_config_group_property (net_nfc_property_group_h group, uint16_t attribute)
{
	return net_nfc_util_remove_carrier_config_group_property ((net_nfc_carrier_property_s*) group, attribute);
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_free_carrier_group (net_nfc_property_group_h group)
{
	return net_nfc_util_free_carrier_group ((net_nfc_carrier_property_s*) group);
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_create_ndef_record_with_carrier_config (ndef_record_h * record, net_nfc_carrier_config_h config)
{
	return net_nfc_util_create_ndef_record_with_carrier_config ((ndef_record_s**) record, (net_nfc_carrier_config_s *) config);
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_create_carrier_config_from_config_record (net_nfc_carrier_config_h * config, ndef_record_h record)
{
	return  net_nfc_util_create_carrier_config_from_config_record ((net_nfc_carrier_config_s **) config, (ndef_record_s *) record);

}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_append_carrier_config_record (ndef_message_h message, ndef_record_h record, net_nfc_conn_handover_carrier_state_e power_status)
{
	return net_nfc_util_append_carrier_config_record ((ndef_message_s *) message, (ndef_record_s *) record, power_status);
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_remove_carrier_config_record (ndef_message_h message, ndef_record_h record)
{
	return net_nfc_util_remove_carrier_config_record ((ndef_message_s *) message, (ndef_record_s *) record);
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_get_carrier_config_record (ndef_message_h message, int index, ndef_record_h * record)
{
	return net_nfc_util_get_carrier_config_record ((ndef_message_s *) message, index, (ndef_record_s **) record);
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_get_handover_random_number(ndef_message_h message, unsigned short* random_number)
{
	return net_nfc_util_get_handover_random_number((ndef_message_s *) message,  random_number);
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_get_alternative_carrier_record_count (ndef_message_h message,  unsigned int * count)
{
	return net_nfc_util_get_alternative_carrier_record_count ((ndef_message_s *) message,  count);
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_get_alternative_carrier_power_status (ndef_message_h message, int index, net_nfc_conn_handover_carrier_state_e * power_state)
{
	return net_nfc_util_get_alternative_carrier_power_status ((ndef_message_s *) message, index, power_state);
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_set_alternative_carrier_power_status (ndef_message_h message, int index, net_nfc_conn_handover_carrier_state_e power_status)
{
	return net_nfc_util_set_alternative_carrier_power_status ((ndef_message_s *) message, index, power_status);
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_get_alternative_carrier_type (ndef_message_h message, int index, net_nfc_conn_handover_carrier_type_e * type)
{
	return net_nfc_util_get_alternative_carrier_type ((ndef_message_s *) message,  index, type);
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_create_handover_request_message (ndef_message_h * message)
{
	return net_nfc_util_create_handover_request_message ((ndef_message_s **) message);
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_create_handover_select_message (ndef_message_h * message)
{
	return net_nfc_util_create_handover_select_message((ndef_message_s **) message);
}

NET_NFC_EXPORT_API net_nfc_error_e net_nfc_create_handover_error_record (ndef_record_h * record, uint8_t reason, uint32_t data)
{
	return net_nfc_util_create_handover_error_record ((ndef_record_s**) record, reason, data);
}


