/*
 * Copyright (c) 2012, 2013 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Flora License, Version 1.0 (the "License");
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

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <sys/time.h>
#include <stdarg.h>
#include <signal.h>
#include <glib-object.h>

#include "net_nfc_typedef_internal.h"
#include "net_nfc_debug_internal.h"
#include "net_nfc_util_defines.h"
#include "net_nfc_util_internal.h"
#include "net_nfc_util_hce.h"
#include "net_nfc_client_hce.h"
#include "net_nfc_client_hce_ipc.h"

/* static variable */
static pthread_mutex_t g_client_ipc_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t cb_lock = PTHREAD_MUTEX_INITIALIZER;

static int hce_client_socket = -1;
static GIOChannel *hce_client_channel = NULL;
static guint hce_client_src_id = 0;

/* static function */

/////////////////////
static void __set_non_block_socket(int socket)
{
	int flags;

	flags = fcntl(socket, F_GETFL);
	flags |= O_NONBLOCK;

	if (fcntl(socket, F_SETFL, flags) < 0) {
		DEBUG_ERR_MSG("fcntl, executing nonblock error");
	}
}

static bool __receive_data_from_server(int socket, data_s *data)
{
	bool result;
	ssize_t ret;
	uint32_t len;

	/* first, receive length */
	ret = recv(socket, (void *)&len, sizeof(len), 0);
	if (ret != sizeof(len)) {
		DEBUG_ERR_MSG("recv failed, socket [%d], result [%d]", socket, ret);

		return false;
	}

	if (len > 1024) {
		DEBUG_ERR_MSG("too large message, socket [%d], len [%d]", socket, len);

		return false;
	}

	result = net_nfc_util_init_data(data, len);
	if (result == true) {
		ssize_t offset = 0;

		/* second, receive buffer */
		do {
			ret = recv(socket, data->buffer + offset, data->length - offset, 0);
			if (ret == -1) {
				break;
			}

			offset += ret;
		} while (offset < len);

		if (offset != len) {
			DEBUG_ERR_MSG("recv failed, socket [%d], offset [%d], len [%d]", socket, offset, len);

			net_nfc_util_clear_data(data);

			result = false;
		} else {
			DEBUG_CLIENT_MSG("recv success, length [%d]", offset);
		}
	} else {
		DEBUG_ERR_MSG("net_nfc_util_init_data failed");
	}

	return result;
}

static bool __process_server_message()
{
	bool result;
	data_s data;

	if (__receive_data_from_server(hce_client_socket, &data) == true) {
		net_nfc_hce_data_t *header;
		data_s temp;

		header = (net_nfc_hce_data_t *)data.buffer;

		temp.buffer = header->data;
		temp.length = data.length - sizeof(net_nfc_hce_data_t);

		net_nfc_client_hce_process_received_event(header->type,
			(net_nfc_target_handle_h)header->handle, (data_h)&temp);

		result = true;
	} else {
		DEBUG_ERR_MSG("__receive_data_from_client failed");

		result = false;
	}

	return result;
}

static bool __send_data_to_server(int socket, data_s *data)
{
	ssize_t ret;

	ret = send(socket, data->buffer, data->length, 0);
	if (ret == -1) {
		DEBUG_ERR_MSG("send failed, socket [%d]", socket);

		return false;
	}

	return true;
}

bool net_nfc_server_hce_ipc_send_to_server(int type,
	net_nfc_target_handle_s *handle, data_s *data)
{
	bool ret;
	data_s temp;
	uint32_t len = sizeof(net_nfc_hce_data_t);

	if (data != NULL && data->length > 0) {
		len += data->length;
	}

	ret = net_nfc_util_init_data(&temp, len + sizeof(len));
	if (ret == true) {
		net_nfc_hce_data_t *header;

		*(uint32_t *)(temp.buffer) = len;
		header = (net_nfc_hce_data_t *)(temp.buffer + sizeof(len));

		header->type = type;
		header->handle = (int)handle;

		if (data != NULL && data->length > 0) {
			memcpy(header->data, data->buffer, data->length);
		}

		ret = __send_data_to_server(hce_client_socket, &temp);

		net_nfc_util_clear_data(&temp);
	} else {
		DEBUG_ERR_MSG("net_nfc_util_init_data failed");
	}

	return ret;
}

/******************************************************************************/

inline void net_nfc_client_ipc_lock()
{
	pthread_mutex_lock(&g_client_ipc_mutex);
}

inline void net_nfc_client_ipc_unlock()
{
	pthread_mutex_unlock(&g_client_ipc_mutex);
}

bool net_nfc_client_hce_ipc_is_initialized()
{
	return (hce_client_socket != -1 && hce_client_channel != NULL);
}

static net_nfc_error_e _finalize_client_socket()
{
	net_nfc_error_e result = NET_NFC_OK;

	net_nfc_client_ipc_lock();

	if (hce_client_src_id > 0) {
		g_source_remove(hce_client_src_id);
		hce_client_src_id = 0;
	}

	if (hce_client_channel != NULL) {
		g_io_channel_unref(hce_client_channel);
		hce_client_channel = NULL;
	}

	if (hce_client_socket != -1) {
		shutdown(hce_client_socket, SHUT_WR);
		close(hce_client_socket);
		hce_client_socket = -1;

		INFO_MSG("client socket closed");
	}

	net_nfc_client_ipc_unlock();

	return result;
}

static gboolean __on_io_event_cb(GIOChannel *channel, GIOCondition condition,
	gpointer data)
{
	if ((G_IO_ERR & condition) || (G_IO_HUP & condition)) {
		DEBUG_CLIENT_MSG("client socket is closed");

		/* clean up client context */
		net_nfc_client_hce_ipc_deinit();

		return FALSE;
	} else if (G_IO_NVAL & condition) {
		DEBUG_CLIENT_MSG("INVALID socket");

		return FALSE;
	} else if (G_IO_IN & condition) {
		if(channel != hce_client_channel) {
			DEBUG_CLIENT_MSG("unknown channel");

			return FALSE;
		}

		DEBUG_CLIENT_MSG("message from server to client socket");

		if (__process_server_message() == false) {
			DEBUG_ERR_MSG("__process_server_message failed");

			net_nfc_client_hce_ipc_deinit();

			return FALSE;
		}
	} else {
		DEBUG_CLIENT_MSG("IO ERROR. socket is closed ");

		/* clean up client context */
		net_nfc_client_hce_ipc_deinit();

		return FALSE;
	}

	return TRUE;
}

net_nfc_error_e net_nfc_client_hce_ipc_init()
{
	net_nfc_error_e result = NET_NFC_OK;
	struct sockaddr_un saddrun_rv;
	socklen_t len_saddr = 0;
	pthread_mutexattr_t attr;

	if (net_nfc_client_hce_ipc_is_initialized() == true)
	{
		DEBUG_CLIENT_MSG("client is already initialized");

		return NET_NFC_ALREADY_INITIALIZED;
	}

	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
	pthread_mutex_init(&cb_lock, &attr);

	memset(&saddrun_rv, 0, sizeof(struct sockaddr_un));

	net_nfc_client_ipc_lock();

	hce_client_socket = socket(AF_UNIX, SOCK_STREAM, 0);
	if (hce_client_socket == -1)
	{
		DEBUG_ERR_MSG("get socket is failed");

		result = NET_NFC_IPC_FAIL;
		goto ERROR;
	}

	__set_non_block_socket(hce_client_socket);

	saddrun_rv.sun_family = AF_UNIX;
	strncpy(saddrun_rv.sun_path, NET_NFC_HCE_SERVER_DOMAIN, sizeof(saddrun_rv.sun_path) - 1);

	len_saddr = sizeof(saddrun_rv.sun_family) + strlen(NET_NFC_HCE_SERVER_DOMAIN);

	if (connect(hce_client_socket, (struct sockaddr *)&saddrun_rv, len_saddr) < 0) {
		DEBUG_ERR_MSG("error is occured");
		result = NET_NFC_IPC_FAIL;

		goto ERROR;
	}

	GIOCondition condition = (GIOCondition)(G_IO_ERR | G_IO_HUP | G_IO_IN);

	hce_client_channel = g_io_channel_unix_new(hce_client_socket);
	if (hce_client_channel == NULL) {
		DEBUG_ERR_MSG(" g_io_channel_unix_new is failed ");
		result = NET_NFC_IPC_FAIL;

		goto ERROR;
	}

	hce_client_src_id = g_io_add_watch(hce_client_channel, condition,
		__on_io_event_cb, NULL);
	if (hce_client_src_id < 1) {
		DEBUG_ERR_MSG(" g_io_add_watch is failed ");
		result = NET_NFC_IPC_FAIL;

		goto ERROR;
	}

	DEBUG_CLIENT_MSG("client socket is ready");

	net_nfc_client_ipc_unlock();

	return NET_NFC_OK;

ERROR :
	DEBUG_ERR_MSG("error while initializing client ipc");

	net_nfc_client_ipc_unlock();

	_finalize_client_socket();

	return result;
}

void net_nfc_client_hce_ipc_deinit()
{
	if (net_nfc_client_hce_ipc_is_initialized() == true) {
		_finalize_client_socket();
	}
}
