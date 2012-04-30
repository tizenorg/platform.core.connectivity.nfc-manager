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

#ifndef NET_NFC_SERVER_IPC_H
#define NET_NFC_SERVER_IPC_H

#include <glib-object.h>

#include "net_nfc_typedef_private.h"

#define MAX_CLIENTS 10

typedef struct net_nfc_server_info_t
{
	uint32_t server_src_id ;
	uint32_t client_src_id ; /* current client src id */

	GIOChannel* server_channel ;
	GIOChannel* client_channel ; /* current client channel */

	int server_sock_fd ;
	int client_sock_fd ; /* current client sock fd*/

	server_state_e state;
#ifdef BROADCAST_MESSAGE
	net_nfc_server_received_message_s* received_message;
#endif

	int connected_client_count;

	net_nfc_current_target_info_s*	target_info;
}net_nfc_server_info_t;

bool net_nfc_server_ipc_initialize();
bool net_nfc_server_ipc_finalize();
bool net_nfc_server_recv_message_from_client(int client_sock_fd, void* message, int length);
bool net_nfc_server_get_current_client_context(void* client_context);
bool net_nfc_server_check_client_is_running(void* client_context);

net_nfc_target_handle_s* net_nfc_server_get_current_client_target_handle(int socket_fd);
bool net_nfc_server_set_current_client_target_handle(int socket_fd, net_nfc_target_handle_s* handle);

bool net_nfc_server_get_client_type(int socket, int* client_type);
bool net_nfc_server_set_client_type(int socket, int type);
bool net_nfc_server_set_server_state(server_state_e state);
server_state_e net_nfc_server_get_server_state();

bool _net_nfc_send_response_msg (int msg_type, ...);
bool _net_nfc_check_client_handle ();
server_state_e net_nfc_get_server_state();
void net_nfc_server_set_tag_info(void * info);
net_nfc_current_target_info_s* net_nfc_server_get_tag_info();
void net_nfc_server_free_current_tag_info();


#endif

