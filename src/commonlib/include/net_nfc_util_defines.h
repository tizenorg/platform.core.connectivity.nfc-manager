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

#ifndef __NET_NFC_UTIL_DEFINES__
#define __NET_NFC_UTIL_DEFINES__

#define NET_NFC_UTIL_MSG_TYPE_REQUEST 0
#define NET_NFC_UTIL_MSG_TYPE_RESPONSE 1

#define CONN_HANOVER_MAJOR_VER 1
#define CONN_HANOVER_MINOR_VER 2

#define CONN_HANDOVER_BT_CARRIER_MIME_NAME "application/vnd.bluetooth.ep.oob"
#define CONN_HANDOVER_WIFI_BSS_CARRIER_MIME_NAME "application/vnd.wfa.wsc"
#define CONN_HANDOVER_WIFI_IBSS_CARRIER_MIME_NAME "application/vnd.wfa.wsc;mode=ibss"

#define BLUETOOTH_ADDRESS_LENGTH 6
#define HIDDEN_BT_ADDR_FILE "/opt/etc/.bd_addr"

#define NET_NFC_VCONF_KEY_PROGRESS "db/nfc/progress"

/* define vconf key */
#define NET_NFC_DISABLE_LAUNCH_POPUP_KEY "memory/nfc/popup_disabled"

#endif
