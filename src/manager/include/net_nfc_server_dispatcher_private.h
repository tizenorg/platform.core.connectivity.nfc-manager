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

#ifndef __NET_NFC_SERVER_DISPATCHER__
#define __NET_NFC_SERVER_DISPATCHER__

#include "net_nfc_typedef_private.h"

void net_nfc_dispatcher_queue_push(net_nfc_request_msg_t* req_msg);
bool net_nfc_dispatcher_start_thread();
void net_nfc_dispatcher_cleanup_queue(void);
void net_nfc_dispatcher_put_cleaner(void);


#endif


