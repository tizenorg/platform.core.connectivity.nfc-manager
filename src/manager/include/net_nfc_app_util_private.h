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

#ifndef NET_NFC_APP_UTIL_H
#define NET_NFC_APP_UTIL_H

#include "net_nfc_typedef.h"
#include "net_nfc_typedef_private.h"
#include <aul.h>

#ifndef MESSAGE_STORAGE
#define MESSAGE_STORAGE "/opt/share/service/nfc-manager"
#endif

net_nfc_error_e net_nfc_app_util_store_ndef_message(data_s *data);
net_nfc_error_e net_nfc_app_util_process_ndef(data_s *data);
void net_nfc_app_util_aul_launch_app(char* package_name, bundle* kb);
void net_nfc_app_util_clean_storage(char* src_path);
bool net_nfc_app_util_is_dir(const char* path_name);
int net_nfc_app_util_appsvc_launch(const char *operation, const char *uri, const char *mime, const char *data);

#endif
