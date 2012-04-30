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

#ifndef __NET_NFC_CLIENT_UTIL_PRIVATE__
#define __NET_NFC_CLENT_UTIL_PRIVATE__

/* Memory utils */
/* free memory, after free given memory it set NULL. Before proceed free, this function also check NULL */
void __net_nfc_client_util_free_mem(void** mem, char * filename, unsigned int line);
/* allocation memory */
void __net_nfc_client_util_alloc_mem(void** mem, int size, char * filename, unsigned int line);
#define	 _net_nfc_client_util_alloc_mem(mem,size) __net_nfc_client_util_alloc_mem((void **)&mem,size,__FILE__, __LINE__)
#define	 _net_nfc_client_util_free_mem(mem) __net_nfc_client_util_free_mem((void **)&mem,__FILE__, __LINE__)



#endif

