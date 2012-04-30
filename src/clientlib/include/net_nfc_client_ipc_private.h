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

#ifndef NET_NFC_CLIENT_IPC_H
#define NET_NFC_CLIENT_IPC_H

#include "net_nfc_typedef.h"

net_nfc_error_e _net_nfc_client_send_reqeust(net_nfc_request_msg_t *msg, ...);

bool net_nfc_client_is_connected();
net_nfc_error_e net_nfc_client_socket_initialize();
net_nfc_error_e net_nfc_client_socket_finalize();

void _net_nfc_client_set_cookies(const char * cookie, size_t size);
void _net_nfc_client_free_cookies(void);

net_nfc_error_e _net_nfc_client_register_cb(net_nfc_response_cb cb);
net_nfc_error_e _net_nfc_client_unregister_cb(void);

#endif

