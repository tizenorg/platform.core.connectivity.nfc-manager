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

#ifndef __NET_NFC_MANAGER_UTIL_PRIVATE__
#define __NET_NFC_MANAGER_UTIL_PRIVATE__


#define NET_NFC_SERVER_ADDRESS "127.0.0.1"
#define NET_NFC_SERVER_PORT 3000

#define NET_NFC_SERVER_DOMAIN "/tmp/nfc-manager-server-domain"

#define NET_NFC_MANAGER_DATA_PATH				"/opt/data/nfc-manager-daemon"
#define NET_NFC_MANAGER_DATA_PATH_MESSAGE		"message"
#define NET_NFC_MANAGER_DATA_PATH_CONFIG		"config"
#define NET_NFC_MANAGER_NDEF_FILE_NAME			"ndef-message.txt"
#define NET_NFC_MANAGER_LLCP_CONFIG_FILE_NAME	"nfc-manager-config.txt"

#define BUFFER_LENGTH_MAX 1024

#define READ_BUFFER_LENGTH_MAX BUFFER_LENGTH_MAX
#define WRITE_BUFFER_LENGTH_MAX BUFFER_LENGTH_MAX
#define NET_NFC_MAX_LLCP_SOCKET_BUFFER BUFFER_LENGTH_MAX

/* Memory utils */
/* free memory, after free given memory it set NULL. Before proceed free, this function also check NULL */
void __net_nfc_manager_util_free_mem(void** mem, char * filename, unsigned int line);
/* allocation memory */
void __net_nfc_manager_util_alloc_mem(void** mem, int size, char * filename, unsigned int line);
#define	 _net_nfc_manager_util_alloc_mem(mem,size) __net_nfc_manager_util_alloc_mem((void **)&mem,size,__FILE__, __LINE__)
#define	 _net_nfc_manager_util_free_mem(mem) __net_nfc_manager_util_free_mem((void **)&mem,__FILE__, __LINE__)



#endif

