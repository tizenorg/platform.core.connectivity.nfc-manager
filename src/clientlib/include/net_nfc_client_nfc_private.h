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

#ifndef __NET_NFC_CLIENT_NET_NFC_PRIVATE_H__
#define __NET_NFC_CLIENT_NET_NFC_PRIVATE_H__

#include "net_nfc_typedef_private.h"

#ifdef __cplusplus
extern "C" {
#endif

client_context_t* net_nfc_get_client_context();
bool net_nfc_tag_is_connected();

/**
@} */

#ifdef __cplusplus
}
#endif


#endif

