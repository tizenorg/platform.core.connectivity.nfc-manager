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


#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <glib-object.h>
#include <unistd.h>
#include <pthread.h>

#include "net_nfc_typedef_internal.h"
#include "net_nfc_debug_internal.h"
#include "net_nfc_util_defines.h"
#include "net_nfc_util_internal.h"
#include "net_nfc_util_hce.h"
#include "net_nfc_server_context_internal.h"
#include "net_nfc_server_hce.h"
#include "net_nfc_server_hce_ipc.h"

/* define */
typedef struct _net_nfc_hce_client_t
{
	int socket;
	GIOChannel *channel;
	guint source_id;
	pid_t pid;

	char *id;
}
net_nfc_hce_client_t;

#define NET_NFC_MANAGER_OBJECT	"nfc-manager"
#define NET_NFC_CARD_EMUL_LABEL	"nfc-manager::card_emul"

/////////////////////////////

/* static variable */
static int hce_server_socket = -1;
static GIOChannel *hce_server_channel;
static guint hce_server_src_id;

static GHashTable *hce_clients;

/////////////////

/*define static function*/
static void __set_non_block_socket(int socket);
static pid_t __get_pid_by_socket(int socket);
static bool __do_client_connect_request();

////////////////////////////////////////////////////////////////////////////////

static void __set_non_block_socket(int socket)
{
	int flags;

	flags = fcntl(socket, F_GETFL);
	flags |= O_NONBLOCK;

	if (fcntl(socket, F_SETFL, flags) < 0) {
		DEBUG_ERR_MSG("fcntl, executing nonblock error");
	}
}

static pid_t __get_pid_by_socket(int socket)
{
	struct ucred uc;
	socklen_t uc_len = sizeof(uc);
	pid_t pid;

	if (getsockopt(socket, SOL_SOCKET, SO_PEERCRED, &uc, &uc_len) == 0) {
		pid = uc.pid;
	} else {
		pid = -1;
	}

	return pid;
}

static void __on_client_value_destroy(gpointer data)
{
	net_nfc_hce_client_t *client = (net_nfc_hce_client_t *)data;

	if (data != NULL) {
		if (client->id != NULL) {
			g_free(client->id);
		}

		if (client->source_id > 0) {
			g_source_remove(client->source_id);
		}

		if (client->channel != NULL) {
			g_io_channel_unref(client->channel);
		}

		if (client->socket != -1) {
			shutdown(client->socket, SHUT_RDWR);
			close(client->socket);
		}

		g_free(data);
	}

	DEBUG_SERVER_MSG("client removed, [%d]", g_hash_table_size(hce_clients));
}

static void __hce_client_init()
{
	if (hce_clients == NULL) {
		hce_clients = g_hash_table_new_full(g_direct_hash,
			g_direct_equal, NULL, __on_client_value_destroy);
	}
}

static void __hce_client_add(int socket, GIOChannel *channel,
	guint source_id)
{
	net_nfc_hce_client_t *client;

	client = g_try_new0(net_nfc_hce_client_t, 1);
	if (client != NULL) {

		client->pid = __get_pid_by_socket(socket);
		if (client->pid != -1) {
			net_nfc_client_context_info_t *context;

			context = net_nfc_server_gdbus_get_client_context_by_pid(
				client->pid);
			if (context != NULL) {
				client->id = g_strdup(context->id);
			} else {
				DEBUG_ERR_MSG("net_nfc_server_gdbus_get_client_context_by_pid failed");
			}
		} else {
			DEBUG_ERR_MSG("__get_pid_by_socket failed");
		}

		client->socket = socket;
		client->channel = channel;
		client->source_id = source_id;

		g_hash_table_insert(hce_clients, (gpointer)channel, client);

		DEBUG_SERVER_MSG("client added, [%d]", g_hash_table_size(hce_clients));
	} else {
		DEBUG_ERR_MSG("alloc failed");
	}
}

static void __hce_client_del(GIOChannel *channel)
{
	g_hash_table_remove(hce_clients, (gconstpointer)channel);
}

static void __hce_client_clear()
{
	g_hash_table_unref(hce_clients);
}

static net_nfc_hce_client_t *__hce_client_find(GIOChannel *channel)
{
	return (net_nfc_hce_client_t *)g_hash_table_lookup(hce_clients,
		(gconstpointer)channel);
}

static net_nfc_hce_client_t *__hce_client_find_by_id(const char *id)
{
	net_nfc_hce_client_t *result = NULL;
	char *key;
	net_nfc_hce_client_t *value;
	GHashTableIter iter;

	g_hash_table_iter_init(&iter, hce_clients);

	while (g_hash_table_iter_next(&iter, (gpointer *)&key,
		(gpointer *)&value) == true) {
		if (value->id != NULL && g_strcmp0(value->id, id) == 0) {
			result = value;
			break;
		}
	}

	return result;
}

/******************************************************************************/

static bool __receive_data_from_client(int socket, data_s *data)
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
			DEBUG_ERR_MSG("recv failed, socket [%d]", socket);

			net_nfc_util_clear_data(data);

			result = false;
		} else {
			DEBUG_SERVER_MSG("recv success, length [%d]", offset);
		}
	} else {
		DEBUG_ERR_MSG("net_nfc_util_init_data failed");
	}

	return result;
}

static bool __process_client_message(GIOChannel *channel)
{
	bool result;
	data_s data;
	net_nfc_hce_client_t *client;

	client = __hce_client_find(channel);
	if (client != NULL) {
		if (__receive_data_from_client(client->socket, &data) == true) {
			net_nfc_hce_data_t *header;
			data_s temp;

			header = (net_nfc_hce_data_t *)data.buffer;

			temp.buffer = header->data;
			temp.length = data.length - sizeof(net_nfc_hce_data_t);

			net_nfc_server_hce_handle_send_apdu_response(
				(net_nfc_target_handle_s *)header->handle, &temp);

			result = true;
		} else {
			DEBUG_ERR_MSG("__receive_data_from_client failed");

			result = false;
		}
	} else {
		DEBUG_ERR_MSG("client not found");

		result = false;
	}

	return result;
}

static gboolean __on_client_io_event_cb(GIOChannel *channel,
	GIOCondition condition, gpointer data)
{
	if ((G_IO_ERR & condition) || (G_IO_HUP & condition)) {
		DEBUG_SERVER_MSG("client socket is closed");

		__hce_client_del(channel);

		return FALSE;
	} else if (G_IO_NVAL & condition) {
		DEBUG_SERVER_MSG("INVALID socket");

		return FALSE;
	} else if (G_IO_IN & condition) {
		if (__process_client_message(channel) == false) {
			DEBUG_SERVER_MSG("__process_client_message failed");

			__hce_client_del(channel);

			return FALSE;
		}
	}

	return TRUE;
}

static bool __do_client_connect_request()
{
	socklen_t addrlen = 0;
	int socket = -1;
	GIOChannel *channel = NULL;
	guint source_id = 0;
	GIOCondition condition = (GIOCondition)(G_IO_ERR | G_IO_HUP | G_IO_IN);

	socket = accept(hce_server_socket, NULL, &addrlen);
	if (socket < 0) {
		DEBUG_ERR_MSG("accept failed, [%d]", errno);

		return false;
	}

	channel = g_io_channel_unix_new(socket);
	if (channel == NULL) {
		DEBUG_ERR_MSG("create new g io channel is failed");

		goto ERROR;
	}

	source_id = g_io_add_watch(channel, condition,
		__on_client_io_event_cb, NULL);
	if (source_id == 0) {
		DEBUG_ERR_MSG("add io callback is failed");

		goto ERROR;
	}

	__hce_client_add(socket, channel, source_id);

	INFO_MSG("client is accepted successfully, [%d]", socket);

	return true;

ERROR :
	if (channel != NULL) {
		g_io_channel_unref(channel);
	}

	if (socket != -1) {
		shutdown(socket, SHUT_RDWR);
		close(socket);
	}

	return false;
}

static gboolean __on_io_event_cb(GIOChannel *channel, GIOCondition condition,
	gpointer data)
{
	if ((G_IO_ERR & condition) || (G_IO_HUP & condition)) {
		DEBUG_SERVER_MSG("server socket is closed");

		net_nfc_server_hce_ipc_deinit();

		return FALSE;
	} else if (G_IO_NVAL & condition) {
		DEBUG_SERVER_MSG("INVALID socket");

		return FALSE;
	} else if (G_IO_IN & condition) {
		__do_client_connect_request();
	}

	return TRUE;
}

bool net_nfc_server_hce_ipc_init()
{
	struct sockaddr_un saddrun_rv;
	int ret;

	/* initialize server context */
	__hce_client_init();

	hce_server_socket = -1;
	hce_server_channel = (GIOChannel *)NULL;
	hce_server_src_id = 0;

	///////////////////////////////

	(void)remove(NET_NFC_HCE_SERVER_DOMAIN);

	memset(&saddrun_rv, 0, sizeof(struct sockaddr_un));

	hce_server_socket = net_nfc_util_get_fd_from_systemd();
	if (hce_server_socket == -1) {
		DEBUG_ERR_MSG("create socket failed, [%d]", errno);

		return false;
	}

	__set_non_block_socket(hce_server_socket);

	if (listen(hce_server_socket, SOMAXCONN) < 0) {
		DEBUG_ERR_MSG("listen is failed");

		goto ERROR;
	}

	GIOCondition condition = (GIOCondition)(G_IO_ERR | G_IO_HUP | G_IO_IN);

	hce_server_channel = g_io_channel_unix_new(hce_server_socket);
	if (hce_server_channel == NULL) {
		DEBUG_ERR_MSG("g_io_channel_unix_new is failed");

		goto ERROR;
	}

	hce_server_src_id = g_io_add_watch(hce_server_channel,
		condition, __on_io_event_cb, NULL);
	if (hce_server_src_id < 1) {
		DEBUG_ERR_MSG("g_io_add_watch is failed");

		goto ERROR;
	}

	DEBUG_SERVER_MSG("hce ipc is initialized");

	return true;

ERROR :
	if (hce_server_src_id > 0) {
		g_source_remove(hce_server_src_id);
		hce_server_src_id = 0;
	}

	if (hce_server_channel != NULL) {
		g_io_channel_unref(hce_server_channel);
		hce_server_channel = NULL;
	}

	if (hce_server_socket != -1) {
		shutdown(hce_server_socket, SHUT_RDWR);
		close(hce_server_socket);

		hce_server_socket = -1;
	}

	return false;
}

void net_nfc_server_hce_ipc_deinit()
{
	__hce_client_clear();

	if (hce_server_src_id > 0) {
		g_source_remove(hce_server_src_id);
		hce_server_src_id = 0;
	}

	if (hce_server_channel != NULL) {
		g_io_channel_unref(hce_server_channel);
		hce_server_channel = NULL;
	}

	if (hce_server_socket != -1) {
		shutdown(hce_server_socket, SHUT_RDWR);
		close(hce_server_socket);
		hce_server_socket = -1;
	}
}

static bool __send_data_to_client(int socket, data_s *data)
{
	ssize_t ret;

	ret = send(socket, data->buffer, data->length, 0);
	if (ret == -1) {
		DEBUG_ERR_MSG("send failed, socket [%d]", socket);

		return false;
	}

	return true;
}

bool net_nfc_server_hce_send_to_client(const char *id, int type,
	net_nfc_target_handle_s *handle, data_s *data)
{
	bool ret;
	net_nfc_hce_client_t *client;
	data_s temp;
	uint32_t len = sizeof(net_nfc_hce_data_t);

	client = __hce_client_find_by_id(id);
	if (client == NULL) {
		DEBUG_ERR_MSG("__hce_client_find_by_id failed");

		return false;
	}

	if (data != NULL && data->length > 0) {
		len += data->length;
	}

	ret = net_nfc_util_init_data(&temp, len + sizeof(len)); /* length + data */
	if (ret == true) {
		net_nfc_hce_data_t *header;

		*(uint32_t *)(temp.buffer) = len;
		header = (net_nfc_hce_data_t *)(temp.buffer + sizeof(len));

		header->type = type;
		header->handle = (int)handle;

		if (data != NULL && data->length > 0) {
			memcpy(header->data, data->buffer, data->length);
		}

		ret = __send_data_to_client(client->socket, &temp);

		net_nfc_util_clear_data(&temp);
	} else {
		DEBUG_ERR_MSG("net_nfc_util_init_data failed");
	}

	return ret;
}

bool net_nfc_server_hce_send_to_all_client(int type,
	net_nfc_target_handle_s *handle, data_s *data)
{
	bool ret;
	data_s temp;
	uint32_t len = sizeof(net_nfc_hce_data_t);

	if (data != NULL && data->length > 0) {
		len += data->length;
	}

	ret = net_nfc_util_init_data(&temp, len + sizeof(len)); /* length + data */
	if (ret == true) {
		GHashTableIter iter;
		gpointer key;
		net_nfc_hce_client_t *client;
		net_nfc_hce_data_t *header;

		*(uint32_t *)(temp.buffer) = len;
		header = (net_nfc_hce_data_t *)(temp.buffer + sizeof(len));

		header->type = type;
		header->handle = (int)handle;

		if (data != NULL && data->length > 0) {
			memcpy(header->data, data->buffer, data->length);
		}

		g_hash_table_iter_init(&iter, hce_clients);

		while (g_hash_table_iter_next(&iter, (gpointer *)&key,
			(gpointer *)&client) == true) {
			ret = __send_data_to_client(client->socket, &temp);
		}

		net_nfc_util_clear_data(&temp);
	} else {
		DEBUG_ERR_MSG("net_nfc_util_init_data failed");
	}

	return ret;
}
