/*
 * Copyright (c) 2012, 2013 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Flora License, Version 1.1 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://floralicense.org/license/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __NET_NFC_SERVER_HCE_IPC_H__
#define __NET_NFC_SERVER_HCE_IPC_H__

#include <gio/gio.h>

#include "net_nfc_typedef_internal.h"


/******************************************************************************/
/* internal */
bool net_nfc_server_hce_ipc_init();

void net_nfc_server_hce_ipc_deinit();


bool net_nfc_server_hce_send_to_client(const char *id, int type,
	net_nfc_target_handle_s *handle, data_s *data);

bool net_nfc_server_hce_send_to_all_client(int type,
	net_nfc_target_handle_s *handle, data_s *data);

#endif //__NET_NFC_SERVER_HCE_IPC_H__