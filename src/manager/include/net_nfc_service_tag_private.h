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

#ifndef NET_NFC_SERVICE_TAG_PRIVATE_H
#define NET_NFC_SERVICE_TAG_PRIVATE_H

#include "net_nfc_typedef_private.h"

data_s* net_nfc_service_tag_process(net_nfc_target_handle_s* handle, int devType, net_nfc_error_e* result);
void net_nfc_service_clean_tag_context(net_nfc_request_target_detected_t* stand_alone, net_nfc_error_e result);
void net_nfc_service_watch_dog(net_nfc_request_msg_t* req_msg);

#endif
