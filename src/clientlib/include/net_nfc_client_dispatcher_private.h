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

#ifndef NET_NFC_CLIENT_DISPATCHER_H
#define NET_NFC_CLIENT_DISPATCHER_H

#include "net_nfc_typedef.h"
#include "net_nfc_typedef_private.h"

void net_nfc_client_call_dispatcher_in_ecore_main_loop(net_nfc_response_cb client_cb, net_nfc_response_msg_t* msg);
void net_nfc_client_call_dispatcher_in_g_main_loop(net_nfc_response_cb client_cb, net_nfc_response_msg_t* msg);
void net_nfc_client_call_dispatcher_in_current_context(net_nfc_response_cb client_cb, net_nfc_response_msg_t* msg);

#endif

