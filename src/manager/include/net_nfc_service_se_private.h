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


#ifndef NET_NFC_SERVICE_SE_PRIVATE_H
#define NET_NFC_SERVICE_SE_PRIVATE_H

#include "net_nfc_typedef_private.h"

typedef struct _se_setting_t
{
	net_nfc_target_handle_s *current_ese_handle;
	void *open_request_trans_param;
}
se_setting_t;

void net_nfc_service_se_detected(net_nfc_request_msg_t *req_msg);

/* TAPI SIM API */

bool net_nfc_service_tapi_init(void);
void net_nfc_service_tapi_deinit(void);
bool net_nfc_service_transfer_apdu(data_s *apdu, void *trans_param);
bool net_nfc_service_request_atr(void *trans_param);

#endif
