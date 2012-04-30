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
