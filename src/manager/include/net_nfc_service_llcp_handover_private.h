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

#ifndef NET_NFC_SERVICE_LLCP_HANDOVER_PRVIATE_H_
#define NET_NFC_SERVICE_LLCP_HANDOVER_PRVIATE_H_

#include "net_nfc_typedef_private.h"

typedef struct _carrier_record
{
	ndef_record_s *record;
	net_nfc_conn_handover_carrier_type_e type;
	net_nfc_conn_handover_carrier_state_e state;
} carrier_record_s;

net_nfc_error_e net_nfc_service_llcp_handover_send_request_msg(net_nfc_request_connection_handover_t *msg);

bool net_nfc_service_llcp_connection_handover_selector(net_nfc_llcp_state_t *state, net_nfc_error_e *result);
bool net_nfc_service_llcp_connection_handover_requester(net_nfc_llcp_state_t *state, net_nfc_error_e *result);

#endif /* NET_NFC_SERVICE_LLCP_HANDOVER_PRVIATE_H_ */
