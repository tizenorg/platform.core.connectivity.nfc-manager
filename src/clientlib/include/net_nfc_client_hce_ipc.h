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

#ifndef __NET_NFC_CLIENT_HCE_IPC_H__
#define __NET_NFC_CLIENT_HCE_IPC_H__

#include "net_nfc_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

/* internal */
net_nfc_error_e net_nfc_client_hce_ipc_init();
void net_nfc_client_hce_ipc_deinit();
bool net_nfc_client_hce_ipc_is_initialized();

bool net_nfc_server_hce_ipc_send_to_server(int type,
	net_nfc_target_handle_s *handle, data_s *data);

#ifdef __cplusplus
}
#endif

#endif //__NET_NFC_CLIENT_HCE_IPC_H__
