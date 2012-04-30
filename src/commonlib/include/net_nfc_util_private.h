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

#ifndef __NET_NFC_UTIL_H__
#define __NET_NFC_UTIL_H__

#include <stdio.h>
#include <libgen.h>

#include "net_nfc_typedef_private.h"

typedef enum
{
	CRC_A = 0x00,
	CRC_B,
} CRC_type_e;

/* Memory utils */
/* allocation memory */
void __net_nfc_util_alloc_mem(void** mem, int size, char * filename, unsigned int line);
#define	 _net_nfc_util_alloc_mem(mem,size) __net_nfc_util_alloc_mem((void **)&mem,size, basename(__FILE__), __LINE__)

/* free memory, after free given memory it set NULL. Before proceed free, this function also check NULL */
void __net_nfc_util_free_mem(void** mem, char * filename, unsigned int line);
#define	 _net_nfc_util_free_mem(mem) __net_nfc_util_free_mem((void **)&mem, basename(__FILE__), __LINE__)

bool net_nfc_util_alloc_data(data_s *data, uint32_t length);
bool net_nfc_util_duplicate_data(data_s *dest, net_nfc_data_s *src);
void net_nfc_util_free_data(data_s *data);

void net_nfc_util_mem_free_detail_msg(int msg_type, int message, void *data);

net_nfc_conn_handover_carrier_state_e net_nfc_util_get_cps(net_nfc_conn_handover_carrier_type_e carrier_type);

uint8_t *net_nfc_util_get_local_bt_address();
void net_nfc_util_enable_bluetooth(void);

void net_nfc_util_play_target_detect_sound(void);

bool net_nfc_util_strip_string(char *buffer, int buffer_length);

void net_nfc_util_compute_CRC(CRC_type_e CRC_type, uint8_t *buffer, uint32_t length);

const char *net_nfc_util_get_schema_string(int index);

#endif
