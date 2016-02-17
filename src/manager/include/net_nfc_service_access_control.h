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

#ifndef __NET_NFC_MANAGER_ACCESS_CONTROL_H__
#define __NET_NFC_MANAGER_ACCESS_CONTROL_H__

typedef enum
{
	NET_NFC_ACCESS_CONTROL_PLATFORM = 0x01,
	NET_NFC_ACCESS_CONTROL_UICC = 0x02,
	NET_NFC_ACCESS_CONTROL_ESE = 0x04,
} net_nfc_access_control_type_e;

bool net_nfc_service_check_access_control(pid_t pid,
	net_nfc_access_control_type_e type);

bool net_nfc_service_access_control_is_authorized_nfc_access(uint8_t se_type,
	const char *package, const uint8_t *aid, const uint32_t len);

bool net_nfc_service_access_control_is_authorized_nfc_access_by_pid(
	uint8_t se_type, pid_t pid, const uint8_t *aid, const uint32_t len);

#endif //__NET_NFC_MANAGER_ACCESS_CONTROL_H__
