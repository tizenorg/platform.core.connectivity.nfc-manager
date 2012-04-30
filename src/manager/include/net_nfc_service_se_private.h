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

#ifndef NET_NFC_SERVICE_SE_PRIVATE_H
#define NET_NFC_SERVICE_SE_PRIVATE_H

#include "net_nfc_typedef_private.h"

typedef struct _se_setting_t{
	net_nfc_target_handle_s* current_ese_handle;
	void* open_request_trans_param;
}se_setting_t;

void net_nfc_service_se_detected(net_nfc_request_msg_t* req_msg);

/* TAPI SIM API */

bool net_nfc_service_tapi_init(void);
void net_nfc_service_tapi_deinit(void);
bool net_nfc_service_transfer_apdu(data_s* apdu, void* trans_param);
bool net_nfc_service_request_atr(void* trans_param);

#endif
