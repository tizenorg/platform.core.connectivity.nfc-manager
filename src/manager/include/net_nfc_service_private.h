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

#ifndef NET_NFC_SERVICE_PRIVATE_H
#define NET_NFC_SERVICE_PRIVATE_H

#include "net_nfc_typedef_private.h"

bool net_nfc_service_slave_mode_target_detected(net_nfc_request_msg_t* msg);
bool net_nfc_service_standalone_mode_target_detected(net_nfc_request_msg_t* msg);
bool net_nfc_service_termination (net_nfc_request_msg_t* msg);

void net_nfc_service_target_detected_cb(void* info, void* user_context);
void net_nfc_service_se_transaction_cb(void* info, void* user_context);
void net_nfc_service_llcp_event_cb(void* info, void* user_context);

void net_nfc_service_msg_processing(data_s* data);

#endif
