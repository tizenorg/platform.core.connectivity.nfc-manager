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

#include "net_nfc_typedef_private.h"
#include "net_nfc_debug_private.h"
#include "net_nfc_util_defines.h"
#include "net_nfc_util_private.h"

#include "net_nfc_server_ipc_private.h"
#include "net_nfc_server_dispatcher_private.h"
#include "net_nfc_controller_private.h"
#include "net_nfc_manager_util_private.h"

#include "vconf.h"

#ifdef SECURITY_SERVER
#include <security-server.h>
#endif


/* define */

typedef struct _net_nfc_client_info_t{
	int socket;
	GIOChannel* channel;
	uint32_t src_id;
	client_state_e state;
	int client_type;
	//client_type_e client_type;
	bool is_set_launch_popup;
	net_nfc_target_handle_s* target_handle;
}net_nfc_client_info_t;

#define NET_NFC_MANAGER_OBJECT "nfc-manager"
#define NET_NFC_CLIENT_MAX 5



/////////////////////////////

/* static variable */

static net_nfc_server_info_t g_server_info = {0,};
static net_nfc_client_info_t g_client_info[NET_NFC_CLIENT_MAX] = {{0, NULL, 0, NET_NFC_CLIENT_INACTIVE_STATE, 0, FALSE, NULL},};

#ifdef SECURITY_SERVER
static char* cookies = NULL;
static int cookies_size = 0;
static gid_t gid = 0;
#endif

static pthread_mutex_t g_server_socket_lock = PTHREAD_MUTEX_INITIALIZER;

/////////////////


/*define static function*/

static gboolean net_nfc_server_ipc_callback_func(GIOChannel* channel, GIOCondition condition, gpointer data);
static bool net_nfc_server_cleanup_client_context(GIOChannel* channel);
static bool net_nfc_server_read_client_request(int client_sock_fd, net_nfc_error_e* result);
static bool net_nfc_server_process_client_connect_request();
static bool net_nfc_server_add_client_context(int socket, GIOChannel* channel, uint32_t src_id, client_state_e state);
static bool net_nfc_server_change_client_state(int socket, client_state_e state);
static bool net_nfc_server_set_current_client_context(int socket_fd);
static int net_nfc_server_get_client_sock_fd(GIOChannel* channel);
static client_state_e net_nfc_server_get_client_state(int socket_fd);
static void net_nfc_server_set_non_block_socket(int socket);

/////////////////////////


static void net_nfc_server_set_non_block_socket(int socket)
{
	DEBUG_SERVER_MSG("set non block socket");

	int flags = fcntl (socket, F_GETFL);
	flags |= O_NONBLOCK;

	if (fcntl (socket, F_SETFL, flags) < 0){
		DEBUG_ERR_MSG("fcntl, executing nonblock error");

	}
}

bool net_nfc_server_ipc_initialize()
{

	/* initialize server context */

	g_server_info.server_src_id = 0;
	g_server_info.client_src_id = 0;

	g_server_info.server_channel = (GIOChannel *)NULL;
	g_server_info.client_channel = (GIOChannel *)NULL;

	g_server_info.server_sock_fd = -1;
	g_server_info.client_sock_fd = -1;

	g_server_info.state = NET_NFC_SERVER_IDLE;

#ifdef BROADCAST_MESSAGE
	g_server_info.received_message = NULL;
#endif
	///////////////////////////////
	g_server_info.target_info = NULL;

#ifdef USE_UNIX_DOMAIN

	remove(NET_NFC_SERVER_DOMAIN);

	struct sockaddr_un saddrun_rv;

	memset(&saddrun_rv, 0, sizeof(struct sockaddr_un));

	g_server_info.server_sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);

	if(g_server_info.server_sock_fd == -1)
	{
		DEBUG_SERVER_MSG("get socket is failed \n");
		return false;
	}

	net_nfc_server_set_non_block_socket(g_server_info.server_sock_fd);

	saddrun_rv.sun_family = AF_UNIX;
	strncpy ( saddrun_rv.sun_path, NET_NFC_SERVER_DOMAIN, sizeof(saddrun_rv.sun_path) - 1 );

	if(bind(g_server_info.server_sock_fd, (struct sockaddr *)&saddrun_rv, sizeof(saddrun_rv)) < 0)
	{
		DEBUG_ERR_MSG("bind is failed \n");
		goto ERROR;
	}

	if(chmod(NET_NFC_SERVER_DOMAIN, 0777) < 0)
	{
		DEBUG_ERR_MSG("can not change permission of UNIX DOMAIN file");
		goto ERROR;
	}

#else

	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0x00, sizeof(struct sockaddr_in));

	g_server_info.server_sock_fd = socket(PF_INET, SOCK_STREAM, 0);

	if(g_server_info.server_sock_fd == -1)
	{
		DEBUG_SERVER_MSG("get socket is failed \n");
		return false;
	}

	net_nfc_server_set_non_block_socket(g_server_info.server_sock_fd);

	serv_addr.sin_family= AF_INET;
	serv_addr.sin_addr.s_addr= htonl(INADDR_ANY);
	serv_addr.sin_port= htons(NET_NFC_SERVER_PORT);

	int val = 1;

	if(setsockopt(g_server_info.server_sock_fd, SOL_SOCKET, SO_REUSEADDR, (char *) &val, sizeof(val)) == 0)
	{
		DEBUG_SERVER_MSG("reuse address");
	}


	if(bind(g_server_info.server_sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		DEBUG_ERR_MSG("bind is failed \n");
		goto ERROR;
	}


#endif

	if(listen(g_server_info.server_sock_fd, MAX_CLIENTS) < 0)
	{
		DEBUG_ERR_MSG("listen is failed \n");
		goto ERROR;
	}

	GIOCondition condition = (GIOCondition) (G_IO_ERR | G_IO_HUP | G_IO_IN);

	if((g_server_info.server_channel = g_io_channel_unix_new (g_server_info.server_sock_fd)) != NULL)
	{
		if ((g_server_info.server_src_id = g_io_add_watch(g_server_info.server_channel, condition, net_nfc_server_ipc_callback_func, NULL)) < 1)
		{
			DEBUG_ERR_MSG(" g_io_add_watch is failed \n");
			goto ERROR;
		}
	}
	else
	{
		DEBUG_ERR_MSG(" g_io_channel_unix_new is failed \n");
		goto ERROR;
	}

#ifdef SECURITY_SERVER

	gid = security_server_get_gid(NET_NFC_MANAGER_OBJECT);
	if(gid == 0)
	{
		DEBUG_SERVER_MSG("get gid from security server is failed. this object is not allowed by security server");
		goto ERROR;
	}


	if((cookies_size = security_server_get_cookie_size()) != 0)
	{
		if((cookies = (char *)calloc(1, cookies_size)) == NULL)
		{
			goto ERROR;
		}
	}

#endif

	net_nfc_dispatcher_start_thread();

	DEBUG_SERVER_MSG("server ipc is initialized");

	if(vconf_set_bool(NET_NFC_DISABLE_LAUNCH_POPUP_KEY, TRUE) != 0)
		DEBUG_ERR_MSG("SERVER : launch state set vconf fail");

	return true;
ERROR:
	if(g_server_info.server_channel != NULL)
	{
		g_io_channel_unref(g_server_info.server_channel);
		g_server_info.server_channel = NULL;
	}

	if(g_server_info.server_sock_fd != -1)
	{
		shutdown(g_server_info.server_sock_fd, SHUT_RDWR);
		close(g_server_info.server_sock_fd);

		g_server_info.server_sock_fd = -1;
	}

	return false;
}

bool net_nfc_server_ipc_finalize()
{
	if(g_server_info.server_channel != NULL)
	{
		g_io_channel_unref(g_server_info.server_channel);
		g_server_info.server_channel = NULL;
	}

	if(g_server_info.client_channel != NULL)
	{
		g_io_channel_unref(g_server_info.client_channel);
		g_server_info.server_channel = NULL;
	}

	if(g_server_info.server_sock_fd != -1)
	{
		shutdown(g_server_info.server_sock_fd, SHUT_RDWR);
		close(g_server_info.server_sock_fd);
		g_server_info.server_sock_fd = -1;
	}


	if(g_server_info.client_sock_fd != -1)
	{
		shutdown(g_server_info.client_sock_fd, SHUT_RDWR);
		close(g_server_info.client_sock_fd);
		g_server_info.client_sock_fd = -1;
	}

	return true;
}

gboolean net_nfc_server_ipc_callback_func(GIOChannel* channel, GIOCondition condition, gpointer data)
{
	if((G_IO_ERR & condition) || (G_IO_HUP & condition))
	{
		DEBUG_SERVER_MSG("IO ERROR \n");
		if(channel == g_server_info.server_channel)
		{
			DEBUG_SERVER_MSG("server socket is closed");
			net_nfc_server_ipc_finalize();
		}
		else
		{
			DEBUG_SERVER_MSG("client socket is closed");
			if(net_nfc_server_cleanup_client_context(channel) == false)
			{
				DEBUG_ERR_MSG("failed to cleanup");
			}

			pthread_mutex_lock(&g_server_socket_lock);

			g_server_info.client_channel = NULL;
			g_server_info.client_sock_fd = -1;
			g_server_info.client_src_id = 0;

			pthread_mutex_unlock(&g_server_socket_lock);

		}

		net_nfc_dispatcher_cleanup_queue();

		net_nfc_dispatcher_put_cleaner();

		return FALSE;
	}
	else if(G_IO_NVAL & condition)
	{
		DEBUG_SERVER_MSG("INVALID socket \n");

		return FALSE;
	}
	else if(G_IO_IN & condition)
	{
		int client_sock_fd = 0;

		if(channel == g_server_info.server_channel)
		{
			net_nfc_server_process_client_connect_request();
		}
		else if((client_sock_fd = net_nfc_server_get_client_sock_fd(channel)) > 0)
		{
			net_nfc_error_e result = NET_NFC_OK;

			if(net_nfc_server_read_client_request(client_sock_fd, &result) == false)
			{
				DEBUG_SERVER_MSG("read client request is failed = [0x%x]", result);

				DEBUG_SERVER_MSG("client socket is closed");

				if(net_nfc_server_cleanup_client_context(channel) == false)
				{
					DEBUG_ERR_MSG("failed to cleanup");
				}

				pthread_mutex_lock(&g_server_socket_lock);

				g_server_info.client_channel = NULL;
				g_server_info.client_sock_fd = -1;
				g_server_info.client_src_id = 0;

				pthread_mutex_unlock(&g_server_socket_lock);

				return FALSE;
			}
		}
		else
		{
			DEBUG_SERVER_MSG("unknown channel \n");
			return FALSE;
		}
	}

	return TRUE;
}

bool net_nfc_server_process_client_connect_request()
{
	socklen_t addrlen = 0;

	int client_sock_fd = 0;
	GIOChannel* client_channel = NULL;
	uint32_t client_src_id ;

	DEBUG_SERVER_MSG("client is trying to connect to server");

	if(g_server_info.connected_client_count == NET_NFC_CLIENT_MAX)
	{
		DEBUG_SERVER_MSG("client is fully servered. no more capa is remained.");

		int tmp_client_sock_fd = -1;

		if((tmp_client_sock_fd = accept(g_server_info.server_sock_fd, NULL, &addrlen)) < 0)
		{
			DEBUG_ERR_MSG("can not accept client");
			return false;
		}
		else
		{
			shutdown(tmp_client_sock_fd, SHUT_RDWR);
			close(tmp_client_sock_fd);
			return false;
		}
	}

	if((client_sock_fd = accept(g_server_info.server_sock_fd, NULL, &addrlen)) < 0)
	{
		DEBUG_ERR_MSG("can not accept client");
		return false;
	}

	DEBUG_SERVER_MSG("client is accepted by server");

	GIOCondition condition = (GIOCondition) (G_IO_ERR | G_IO_HUP | G_IO_IN);

	if((client_channel = g_io_channel_unix_new (client_sock_fd)) != NULL)
	{
		if ((client_src_id = g_io_add_watch(client_channel, condition, net_nfc_server_ipc_callback_func, NULL)) < 1)
		{
			DEBUG_ERR_MSG("add io callback is failed");
			goto ERROR;
		}
	}
	else
	{
		DEBUG_ERR_MSG("create new g io channel is failed");
		goto ERROR;
	}

	DEBUG_SERVER_MSG("client socket is bound with g_io_channel");

	if(net_nfc_server_add_client_context(client_sock_fd, client_channel, client_src_id, NET_NFC_CLIENT_INACTIVE_STATE) == false)
	{
		DEBUG_ERR_MSG("failed to add client");
	}
	return true;

ERROR:

	if(client_channel != NULL)
	{
		g_io_channel_unref(client_channel);
	}

	if(client_sock_fd != -1)
	{
		shutdown(client_sock_fd, SHUT_RDWR);
		close(client_sock_fd);
	}

	return false;
}

int __net_nfc_server_read_util (int client_sock_fd, void ** detail, size_t size)
{
	static uint8_t flushing[128];

	*detail = NULL;
	_net_nfc_manager_util_alloc_mem (*detail,size);

	if (*detail == NULL)
	{
		size_t read_size;
		int readbyes = size;

		while (readbyes > 0)
		{
			read_size = readbyes > 128 ? 128 : readbyes;
			if(net_nfc_server_recv_message_from_client (client_sock_fd, flushing, read_size) == false)
			{
				return false;
			}
			readbyes -= read_size;
		}
		return false;
	}

	if(net_nfc_server_recv_message_from_client (client_sock_fd, *detail, size) == false)
	{
		_net_nfc_manager_util_free_mem (*detail);
		return false;
	}
	return true;
}

bool net_nfc_server_read_client_request(int client_sock_fd, net_nfc_error_e *result)
{
	uint32_t length = 0;
	net_nfc_request_msg_t *req_msg = NULL;

#ifdef SECURITY_SERVER
	if(net_nfc_server_recv_message_from_client(client_sock_fd, (void *)cookies, cookies_size) == false)
	{
		*result = NET_NFC_IPC_FAIL;
		return false;
	}
	else
	{
		char printf_buff[BUFFER_LENGTH_MAX] ={0,};
		int buff_count = BUFFER_LENGTH_MAX;
		int i = 0;

		for(; i < cookies_size; i++)
		{
			buff_count -= snprintf (printf_buff + BUFFER_LENGTH_MAX - buff_count, buff_count, " %02X", cookies[i]);
		}
		DEBUG_SERVER_MSG ("server got cookies >>>> %s", printf_buff);
	}
#endif

	if (net_nfc_server_recv_message_from_client(client_sock_fd, (void *)&length, sizeof(uint32_t)) == true && length != 0)
	{
		_net_nfc_util_alloc_mem(req_msg, length);
		if (req_msg != NULL)
		{
			if (net_nfc_server_recv_message_from_client(client_sock_fd, (void *)req_msg + sizeof(uint32_t), length - sizeof(uint32_t)) == true)
			{
				req_msg->length = length;
			}
			else
			{
				_net_nfc_util_free_mem(req_msg);
				*result = NET_NFC_IPC_FAIL;
				return false;
			}
		}
		else
		{
			*result = NET_NFC_ALLOC_FAIL;
			return false;
		}
	}
	else
	{
		*result = NET_NFC_IPC_FAIL;
		return false;
	}

	DEBUG_SERVER_MSG("message from client. request type = [%d]", req_msg->request_type);

#ifdef BROADCAST_MESSAGE
	if(req_msg->request_type != NET_NFC_MESSAGE_SERVICE_CHANGE_CLIENT_STATE &&
		req_msg->request_type != NET_NFC_MESSAGE_SERVICE_SET_LAUNCH_STATE)
	{
		net_nfc_server_received_message_s* p = (net_nfc_server_received_message_s*)malloc(sizeof(net_nfc_server_received_message_s));

		p->client_fd = client_sock_fd;
		p->mes_type = req_msg->request_type;
		p->next = g_server_info.received_message;
		g_server_info.received_message = p;
	}
#endif

	/* process exceptional case of request type */
	switch (req_msg->request_type)
	{
		case NET_NFC_MESSAGE_SERVICE_CHANGE_CLIENT_STATE :
		{
			net_nfc_request_change_client_state_t *detail = (net_nfc_request_change_client_state_t *)req_msg;

			if (net_nfc_server_change_client_state(client_sock_fd, detail->client_state) == true)
			{
				if (detail->client_state == NET_NFC_CLIENT_ACTIVE_STATE)
				{
					/* 1. check the client_sock_fd is current socket */
					net_nfc_server_set_client_type(client_sock_fd, detail->client_type);

					if (client_sock_fd == g_server_info.client_sock_fd)
					{
					}
					else
					{
						/* 1. deactivate before client socket */
						/* 2. set client_sock_fd as current client socket */
						if (g_server_info.client_sock_fd != -1)
						{
							/* if server context.client_sock_fd is 0, then there was no client before*/
							net_nfc_server_change_client_state(g_server_info.client_sock_fd, NET_NFC_CLIENT_INACTIVE_STATE);
						}

						net_nfc_server_set_current_client_context(client_sock_fd);
					}
				}
				else
				{
					/* 1. check the client_sock_fd is current socket */
					if (client_sock_fd == g_server_info.client_sock_fd)
					{
						/* 1. unset current client sock */
						pthread_mutex_lock(&g_server_socket_lock);
						g_server_info.client_channel = NULL;
						g_server_info.client_sock_fd = -1;
						g_server_info.client_src_id = 0;
						pthread_mutex_unlock(&g_server_socket_lock);

						/* 2. change client state */
						net_nfc_server_change_client_state(g_server_info.client_sock_fd, NET_NFC_CLIENT_INACTIVE_STATE);
					}
					else
					{
						/* 1. change client state */
						net_nfc_server_change_client_state(g_server_info.client_sock_fd, NET_NFC_CLIENT_INACTIVE_STATE);
					}
				}
			}

			DEBUG_SERVER_MSG("net_nfc_server_read_client_request is finished");

			_net_nfc_manager_util_free_mem(req_msg);

			return true;
		}
		break;

		case NET_NFC_MESSAGE_SERVICE_SET_LAUNCH_STATE :
		{
			net_nfc_request_set_launch_state_t *detail = (net_nfc_request_set_launch_state_t *)req_msg;

			net_nfc_server_set_launch_state(client_sock_fd, detail->set_launch_popup);
		}
		break;

		default :
			break;
	}

#ifdef SECURITY_SERVER
	int error = 0;
	if((error = security_server_check_privilege(cookies, gid)) < 0)
	{
		DEBUG_SERVER_MSG("failed to authentificate client [%d]", error);
		*result = NET_NFC_SECURITY_FAIL;

		_net_nfc_manager_util_free_mem(req_msg);

		return false;
	}
#endif

#ifdef BROADCAST_MESSAGE
	net_nfc_dispatcher_queue_push(req_msg);
	return true;

#else
	/* check current client context is activated. */
	if (net_nfc_server_get_client_state(client_sock_fd) == NET_NFC_CLIENT_ACTIVE_STATE)
	{
		DEBUG_SERVER_MSG("client is activated");
		net_nfc_dispatcher_queue_push(req_msg);
		DEBUG_SERVER_MSG("net_nfc_server_read_client_request is finished");
		return true;
	}
	else
	{
		DEBUG_SERVER_MSG("client is deactivated");

		/* free req_msg */
		_net_nfc_manager_util_free_mem(req_msg);

		DEBUG_SERVER_MSG("net_nfc_server_read_client_request is finished");
		return false;
	}
#endif
}

static bool net_nfc_server_cleanup_client_context(GIOChannel* channel)
{
	DEBUG_SERVER_MSG("client up client context");
	int i = 0;

	bool ret = false;

	pthread_mutex_lock(&g_server_socket_lock);

	for(; i < NET_NFC_CLIENT_MAX; i++)
	{
		if(g_client_info[i].channel == channel)
		{

			if(g_client_info[i].channel != NULL)
			{
				g_io_channel_unref(g_client_info[i].channel);
			}

			/* need to check . is it neccesary to remove g_source_id */
			if(g_client_info[i].src_id > 0)
			{
				g_source_remove(g_client_info[i].src_id);
			}

			if(g_client_info[i].socket > 0)
			{
				shutdown(g_client_info[i].socket, SHUT_RDWR);
				close(g_client_info[i].socket);
			}

			g_client_info[i].socket = 0;
			g_client_info[i].channel = NULL;
			g_client_info[i].src_id = 0;
			g_client_info[i].state = NET_NFC_CLIENT_INACTIVE_STATE;
			g_client_info[i].target_handle = NULL;

			ret = true;

			g_server_info.connected_client_count--;

			if(vconf_set_bool(NET_NFC_DISABLE_LAUNCH_POPUP_KEY, net_nfc_server_is_set_launch_state()) != 0)
				DEBUG_ERR_MSG("SERVER :set launch vconf fail");

			break;
		}
	}

	pthread_mutex_unlock(&g_server_socket_lock);

	DEBUG_SERVER_MSG("current client count = [%d]", g_server_info.connected_client_count);

	return ret;
}

#ifdef BROADCAST_MESSAGE
bool net_nfc_server_send_message_to_client(int mes_type, void* message, int length)
#else
bool net_nfc_server_send_message_to_client(void* message, int length)
#endif
{
#ifdef BROADCAST_MESSAGE
	net_nfc_server_received_message_s* p1;
	net_nfc_server_received_message_s* p2;

	int leng, i;

	p1 = g_server_info.received_message;
	p2 = NULL;

	while(p1 != NULL)
	{
		if(p1->mes_type == mes_type)
		{
			pthread_mutex_lock(&g_server_socket_lock);
			leng = send(p1->client_fd, (void *)message, length, 0);
			pthread_mutex_unlock(&g_server_socket_lock);



			if ((mes_type ==   NET_NFC_MESSAGE_SERVICE_INIT)||(mes_type ==   NET_NFC_MESSAGE_SERVICE_DEINIT))
			{

				for(i=0; i<NET_NFC_CLIENT_MAX; i++)
				{
					if((g_client_info[i].socket)&&(g_client_info[i].socket !=p1->client_fd))
					{
						pthread_mutex_lock(&g_server_socket_lock);
						leng = send(g_client_info[i].socket, (void *)message, length, 0);
						pthread_mutex_unlock(&g_server_socket_lock);
					}

					if(leng <= 0)
					{
						DEBUG_ERR_MSG("failed to send message, socket = [%d], msg_length = [%d]", g_server_info.client_sock_fd, length);
					}
				}

			}


			if(p2 != NULL)
			{
				p2->next = p1->next;
			}
			else
			{
				g_server_info.received_message = p1->next;
			}
			free(p1);

			if(leng > 0)
			{
				return true;
			}
			else
			{
				DEBUG_ERR_MSG("failed to send message, socket = [%d], msg_length = [%d]", p1->client_fd, length);
				return false;
			}
		}
		p2 = p1;
		p1 = p1->next;
	}

	for(i=0; i<NET_NFC_CLIENT_MAX; i++)
	{
		if(g_client_info[i].socket)
		{
			pthread_mutex_lock(&g_server_socket_lock);
			leng = send(g_client_info[i].socket, (void *)message, length, 0);
			pthread_mutex_unlock(&g_server_socket_lock);
		}

		if(leng <= 0)
		{
			DEBUG_ERR_MSG("failed to send message, socket = [%d], msg_length = [%d]", g_server_info.client_sock_fd, length);
		}
	}

	return true;
#else
	pthread_mutex_lock(&g_server_socket_lock);
	int leng = send(g_server_info.client_sock_fd, (void *)message, length, 0);
	pthread_mutex_unlock(&g_server_socket_lock);

	if(leng > 0)
	{
		return true;
	}
	else
	{
		DEBUG_ERR_MSG("failed to send message, socket = [%d], msg_length = [%d]", g_server_info.client_sock_fd, length);
		return false;
	}
#endif
}

bool net_nfc_server_recv_message_from_client(int client_sock_fd, void* message, int length)
{
	int leng = recv(client_sock_fd, message, length, 0);

	if(leng > 0)
	{
		return true;
	}
	else
	{
		DEBUG_ERR_MSG("failed to recv message, socket = [%d], msg_length = [%d]", g_server_info.client_sock_fd, length);
		return false;
	}
}

static int net_nfc_server_get_client_sock_fd(GIOChannel* channel)
{
	int i = 0;

	int client_sock_fd = 0;

	pthread_mutex_lock(&g_server_socket_lock);

	for(; i < NET_NFC_CLIENT_MAX; i++)
	{
		if(g_client_info[i].channel == channel)
		{
			client_sock_fd = g_client_info[i].socket ;
			break;
		}
	}

	pthread_mutex_unlock(&g_server_socket_lock);

	return client_sock_fd;
}

static bool net_nfc_server_set_current_client_context(int socket_fd)
{
	int i = 0;

	bool ret = false;

	pthread_mutex_lock(&g_server_socket_lock);

	for(; i < NET_NFC_CLIENT_MAX; i++)
	{
		if(g_client_info[i].socket == socket_fd)
		{
			g_server_info.client_sock_fd = g_client_info[i].socket ;
			g_server_info.client_channel = g_client_info[i].channel ;
			g_server_info.client_src_id = g_client_info[i].src_id ;

			ret = true;

			break;
		}
	}

	pthread_mutex_unlock(&g_server_socket_lock);

	return ret;
}

bool net_nfc_server_get_current_client_context(void* client_context)
{
#ifdef BROADCAST_MESSAGE
	return true;
#else
	*((int *)client_context) = g_server_info.client_sock_fd;

	return true;
#endif
}

static client_state_e net_nfc_server_get_client_state(int socket_fd)
{
	int i = 0;

	client_state_e client_state = NET_NFC_CLIENT_INACTIVE_STATE;

	for(; i < NET_NFC_CLIENT_MAX; i++)
	{
		if(g_client_info[i].socket == socket_fd)
		{
			client_state = g_client_info[i].state ;
			break;
		}
	}

	return client_state;
}


net_nfc_target_handle_s* net_nfc_server_get_current_client_target_handle(int socket_fd)
{
	int i = 0;

	pthread_mutex_lock(&g_server_socket_lock);

	net_nfc_target_handle_s* handle = NULL;

	for(; i < NET_NFC_CLIENT_MAX; i++)
	{
		if(g_client_info[i].socket == socket_fd)
		{
			handle = g_client_info[i].target_handle ;
			break;
		}
	}

	pthread_mutex_unlock(&g_server_socket_lock);

	return handle;
}

bool net_nfc_server_set_current_client_target_handle(int socket_fd, net_nfc_target_handle_s* handle)
{
	int i = 0;

	pthread_mutex_lock(&g_server_socket_lock);

	for(; i < NET_NFC_CLIENT_MAX; i++)
	{
		if(g_client_info[i].socket == socket_fd)
		{
			g_client_info[i].target_handle  = handle;
			pthread_mutex_unlock(&g_server_socket_lock);
			return true;
		}
	}

	pthread_mutex_unlock(&g_server_socket_lock);
	return false;
}

bool net_nfc_server_check_client_is_running(void* client_context)
{
#ifdef BROADCAST_MESSAGE
	int i = 0;

	pthread_mutex_lock(&g_server_socket_lock);

	for(; i < NET_NFC_CLIENT_MAX; i++)
	{
		if(g_client_info[i].socket > 0)
		{
			pthread_mutex_unlock(&g_server_socket_lock);
			return true;
		}
	}

	pthread_mutex_unlock(&g_server_socket_lock);
	return false;
#else
	int client_fd = *((int *)client_context);

	if(client_fd > 0)
		return true;
	else
		return false;
#endif
}

static bool net_nfc_server_add_client_context(int socket, GIOChannel* channel, uint32_t src_id, client_state_e state)
{
	DEBUG_SERVER_MSG("add client context");
	int i = 0;

	bool ret = false;

	pthread_mutex_lock(&g_server_socket_lock);

	for(; i < NET_NFC_CLIENT_MAX; i++)
	{
		if(g_client_info[i].socket == 0 && g_client_info[i].channel == NULL)
		{
			g_client_info[i].socket = socket;
			g_client_info[i].channel = channel;
			g_client_info[i].src_id = src_id;
			g_client_info[i].state = state;
			g_client_info[i].is_set_launch_popup = TRUE;

			ret = true;

			g_server_info.connected_client_count++;

			break;
		}
	}

	pthread_mutex_unlock(&g_server_socket_lock);

	DEBUG_SERVER_MSG("current client count = [%d]", g_server_info.connected_client_count);

	return ret;
}

static bool net_nfc_server_change_client_state(int socket, client_state_e state)
{
	int i = 0;

	bool ret = false;

	pthread_mutex_lock(&g_server_socket_lock);

	for(; i < NET_NFC_CLIENT_MAX; i++)
	{
		if(g_client_info[i].socket == socket)
		{
			g_client_info[i].state = state;

			ret = true;

			break;
		}
	}

	pthread_mutex_unlock(&g_server_socket_lock);

	return ret;
}


bool net_nfc_server_set_client_type(int socket, int type)
{
	int i = 0;

	bool ret = false;

	pthread_mutex_lock(&g_server_socket_lock);

	for(; i < NET_NFC_CLIENT_MAX; i++)
	{
		if(g_client_info[i].socket == socket)
		{
			g_client_info[i].client_type = type;

			ret = true;

			break;
		}
	}

	pthread_mutex_unlock(&g_server_socket_lock);

	return ret;
}

bool net_nfc_server_set_server_state(uint32_t state)
{
	pthread_mutex_lock(&g_server_socket_lock);

	if (state == NET_NFC_SERVER_IDLE)
		g_server_info.state &= NET_NFC_SERVER_IDLE;
	else
		g_server_info.state |= state;

	pthread_mutex_unlock(&g_server_socket_lock);

	return true;
}

bool net_nfc_server_unset_server_state(uint32_t state)
{
	pthread_mutex_lock(&g_server_socket_lock);

	g_server_info.state &= ~state;

	pthread_mutex_unlock(&g_server_socket_lock);

	return true;
}

uint32_t net_nfc_server_get_server_state()
{
	return g_server_info.state;
}

bool net_nfc_server_get_client_type(int socket, int* client_type)
{
	int i = 0;

	bool ret = false;

	if(socket < 0 || client_type == NULL)
	{
		return ret;
	}

	pthread_mutex_lock(&g_server_socket_lock);

	for(; i < NET_NFC_CLIENT_MAX; i++)
	{
		if(g_client_info[i].socket == socket)
		{
			*client_type = g_client_info[i].client_type;

			ret = true;

			break;
		}
	}

	pthread_mutex_unlock(&g_server_socket_lock);

	return ret;
}


bool _net_nfc_send_response_msg(int msg_type, ...)
{
	va_list args;
	int total_size = 0;
	int size = 0;
	int written_size = 0;
	void * data;
	uint8_t* send_buffer = NULL;

	va_start (args, msg_type);
	if (va_arg (args, void*) != NULL)
	{
		while ((size = va_arg(args, int)) != 0)
		{

			total_size+=size;

			if (va_arg (args, void*) == NULL)
			{
				break;
			}
		}
	}
	va_end (args);

	total_size += sizeof (int);
	_net_nfc_manager_util_alloc_mem (send_buffer, total_size);
	memcpy(send_buffer, &(msg_type), sizeof(int));
	written_size += sizeof (int);

	va_start (args, msg_type);
	while ((data = va_arg(args, void *)) != 0)
	{
		size = va_arg (args, int);
		if (size == 0) return false;
		memcpy (send_buffer + written_size, data, size);
		written_size += size;
	}
	va_end (args);
#ifdef BROADCAST_MESSAGE
	if(net_nfc_server_send_message_to_client(msg_type, (void *)send_buffer, total_size) == true)
#else
	if(net_nfc_server_send_message_to_client((void *)send_buffer, total_size) == true)
#endif
	{
		DEBUG_SERVER_MSG("SERVER : [MSG:%d] sending response is ok ", msg_type);
	}
	_net_nfc_manager_util_free_mem(send_buffer);
	return true;
}


bool _net_nfc_check_client_handle ()
{
#ifdef BROADCAST_MESSAGE
	return true;
#else
	int client_context = 0;
	if(net_nfc_server_get_current_client_context(&client_context) == true &&
		net_nfc_server_check_client_is_running(&client_context) == true &&
		net_nfc_server_get_current_client_target_handle(client_context) != NULL)
	{
		return true;
	}
	return false;
#endif
}


void net_nfc_server_set_tag_info(void * info)
{
	net_nfc_request_target_detected_t* detail = (net_nfc_request_target_detected_t *)info;

	net_nfc_current_target_info_s* target_info = NULL;

	_net_nfc_util_alloc_mem(target_info, sizeof(net_nfc_current_target_info_s) + detail->target_info_values.length);

	target_info->handle = detail->handle;
	target_info->devType = detail->devType;

	if(target_info->devType != NET_NFC_NFCIP1_INITIATOR && target_info->devType != NET_NFC_NFCIP1_TARGET)
	{
		target_info->number_of_keys =  detail->number_of_keys;
		target_info->target_info_values.length = detail->target_info_values.length;
		memcpy(&target_info->target_info_values, &detail->target_info_values, target_info->target_info_values.length);
	}

	pthread_mutex_lock(&g_server_socket_lock);

	g_server_info.target_info = target_info;

	pthread_mutex_unlock(&g_server_socket_lock);

}

net_nfc_current_target_info_s* net_nfc_server_get_tag_info()
{
	return g_server_info.target_info;
}

void net_nfc_server_free_current_tag_info()
{
	pthread_mutex_lock(&g_server_socket_lock);

	_net_nfc_util_free_mem(g_server_info.target_info);

	pthread_mutex_unlock(&g_server_socket_lock);
}

void net_nfc_server_set_launch_state(int socket, bool enable)
{
	int i = 0;

	if(vconf_set_bool(NET_NFC_DISABLE_LAUNCH_POPUP_KEY, enable) != 0)
		DEBUG_ERR_MSG("SERVER : launch state set vconf fail");

	pthread_mutex_lock(&g_server_socket_lock);

	if(enable == TRUE)
	{
		for(; i < NET_NFC_CLIENT_MAX; i++)
		{
			g_client_info[i].is_set_launch_popup = enable;
		}
	}
	else
	{
		for(; i < NET_NFC_CLIENT_MAX; i++)
		{
			if(g_client_info[i].socket == socket)
			{
				g_client_info[i].is_set_launch_popup = enable;

				break;
			}
		}
	}

	pthread_mutex_unlock(&g_server_socket_lock);

}

bool net_nfc_server_is_set_launch_state()
{
	int i = 0;

	for(; i < NET_NFC_CLIENT_MAX; i++)
	{
		if(g_client_info[i].socket > 0 && g_client_info[i].is_set_launch_popup == FALSE)
			return FALSE;
	}

	return TRUE;
}

