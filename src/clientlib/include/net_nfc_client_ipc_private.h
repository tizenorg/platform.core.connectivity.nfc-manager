/*
  * Copyright 2012  Samsung Electronics Co., Ltd
  *
  * Licensed under the Flora License, Version 1.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at

  *     http://www.tizenopensource.org/license
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
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

