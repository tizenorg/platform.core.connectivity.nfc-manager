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

#include "net_nfc_typedef_private.h"
#include "net_nfc_client_ipc_private.h"
#include "net_nfc_client_dispatcher_private.h"
#include "net_nfc_util_private.h"
#include "net_nfc_debug_private.h"
#include "net_nfc_client_util_private.h"

/* static variable */
static int g_client_sock_fd = -1;

static pthread_mutex_t g_client_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t cb_lock = PTHREAD_MUTEX_INITIALIZER;


#ifndef NET_NFC_SERVER_ADDRESS
#define NET_NFC_SERVER_ADDRESS "127.0.0.1"
#endif

#ifndef NET_NFC_SERVER_PORT
#define NET_NFC_SERVER_PORT 3000
#endif

#ifndef NET_NFC_SERVER_DOMAIN
#define NET_NFC_SERVER_DOMAIN "/tmp/nfc-manager-server-domain"
#endif

#ifdef G_MAIN_LOOP
static GIOChannel* g_client_channel = NULL;
static uint32_t g_client_src_id = 0;
#else

#ifdef USE_IPC_EPOLL
#define EPOLL_SIZE 128
static int g_client_poll_fd = -1;
static struct epoll_event *g_client_poll_events = NULL;
#else
static fd_set fdset_read;
#endif

static pthread_t g_poll_thread = (pthread_t)0;

#endif

/////////////////////

static net_nfc_response_cb client_cb = NULL;
bool being_cb_called = false;



/* static function */
static net_nfc_response_msg_t* net_nfc_client_read_response_msg(net_nfc_error_e* result);
static void net_nfc_client_set_non_block_socket(int socket);

#ifdef G_MAIN_LOOP
gboolean net_nfc_client_ipc_callback_func(GIOChannel* channel, GIOCondition condition, gpointer data);
#else
static bool net_nfc_client_ipc_polling(net_nfc_error_e* result);
static void* net_nfc_client_ipc_thread(void *data);
#endif


static void net_nfc_client_set_non_block_socket(int socket)
{
	DEBUG_CLIENT_MSG("set non block socket");

	int flags = fcntl (socket, F_GETFL);
	flags |= O_NONBLOCK;

	if (fcntl (socket, F_SETFL, flags) < 0){
		DEBUG_ERR_MSG("fcntl, executing nonblock error");

	}
}

/////////////////////

bool net_nfc_client_is_connected()
{
#ifdef G_MAIN_LOOP
	if(g_client_sock_fd != -1 && g_client_channel != NULL && g_client_src_id != 0)
#else
#ifdef USE_IPC_EPOLL
	if(g_client_sock_fd != -1 && g_client_poll_fd != -1 && g_poll_thread != (pthread_t)NULL)
#else
	if(g_client_sock_fd != -1 && g_poll_thread != (pthread_t)NULL)
#endif
#endif
	{
		return true;
	}
	else
	{
		return false;
	}
}

net_nfc_error_e net_nfc_client_socket_initialize()
{
	net_nfc_error_e result = NET_NFC_OK;

	if(net_nfc_client_is_connected() == true)
	{
		DEBUG_CLIENT_MSG("client is already initialized");
		return NET_NFC_ALREADY_INITIALIZED;
	}

	pthread_mutexattr_t attr;

	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
	pthread_mutex_init(&cb_lock, &attr);


#ifdef USE_UNIX_DOMAIN
	struct sockaddr_un saddrun_rv;
	socklen_t len_saddr = 0;

	memset(&saddrun_rv, 0, sizeof(struct sockaddr_un));

	pthread_mutex_lock(&g_client_lock);

	g_client_sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);

	if(g_client_sock_fd == -1)
	{
		DEBUG_ERR_MSG("get socket is failed \n");
		pthread_mutex_unlock(&g_client_lock);
		return NET_NFC_IPC_FAIL;
	}

	DEBUG_CLIENT_MSG("socket is created");

	net_nfc_client_set_non_block_socket(g_client_sock_fd);

	saddrun_rv.sun_family = AF_UNIX;
	strncpy ( saddrun_rv.sun_path, NET_NFC_SERVER_DOMAIN, sizeof(saddrun_rv.sun_path) - 1 );

	len_saddr = sizeof(saddrun_rv.sun_family) + strlen(NET_NFC_SERVER_DOMAIN);

	if ((connect(g_client_sock_fd, (struct sockaddr *)&saddrun_rv, len_saddr)) < 0)
	{
		DEBUG_ERR_MSG("error is occured");
		result = NET_NFC_IPC_FAIL;
		goto ERROR;
	}
#else

	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0x00, sizeof(struct sockaddr_in));

	g_client_sock_fd = socket(PF_INET, SOCK_STREAM, 0);

	if(g_client_sock_fd == -1)
	{
		DEBUG_ERR_MSG("get socket is failed \n");
		pthread_mutex_unlock(&g_client_lock);
		return NET_NFC_IPC_FAIL;
	}

	DEBUG_CLIENT_MSG("socket is created");

	net_nfc_client_set_non_block_socket(g_client_sock_fd);

	serv_addr.sin_family= AF_INET;
	serv_addr.sin_addr.s_addr= htonl(INADDR_ANY);
	serv_addr.sin_port= htons(NET_NFC_SERVER_PORT);

	if ((connect(g_client_sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0)
	{
		DEBUG_ERR_MSG("error is occured");
		result = NET_NFC_IPC_FAIL;
		goto ERROR;
	}

#endif

#ifdef G_MAIN_LOOP

	GIOCondition condition = (GIOCondition) (G_IO_ERR | G_IO_HUP | G_IO_IN);

	if((g_client_channel = g_io_channel_unix_new (g_client_sock_fd)) != NULL)
	{
		if ((g_client_src_id = g_io_add_watch(g_client_channel, condition, net_nfc_client_ipc_callback_func, NULL)) < 1)
		{
			DEBUG_ERR_MSG(" g_io_add_watch is failed \n");
			result = NET_NFC_IPC_FAIL;
			goto ERROR;
		}
	}
	else
	{
		DEBUG_ERR_MSG(" g_io_channel_unix_new is failed \n");
		result = NET_NFC_IPC_FAIL;
		goto ERROR;
	}

	DEBUG_CLIENT_MSG("socket and g io channel is binded");

#else

#ifdef USE_IPC_EPOLL
	if((g_client_poll_fd = epoll_create1(EPOLL_CLOEXEC)) == -1)
	{
		DEBUG_ERR_MSG("error is occured");
		result = NET_NFC_IPC_FAIL;
		goto ERROR;
	}

	if((g_client_poll_events = (struct epoll_event *)calloc(1, sizeof(struct epoll_event) * EPOLL_SIZE)) == NULL)
	{
		DEBUG_ERR_MSG("error is occured");
		result = NET_NFC_ALLOC_FAIL;
		goto ERROR;
	}

	struct epoll_event ev;

	ev.events = EPOLLIN | EPOLLET | EPOLLHUP | EPOLLERR;
	ev.data.fd = g_client_sock_fd;

	epoll_ctl(g_client_poll_fd, EPOLL_CTL_ADD, g_client_sock_fd, &ev);
#else

	FD_ZERO(&fdset_read) ;
	FD_SET(g_client_sock_fd, &fdset_read);

#endif

	/* create polling thread and go */

	pthread_attr_t thread_attr;
	pthread_attr_init(&thread_attr);
	pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);

	pthread_cond_t pcond = PTHREAD_COND_INITIALIZER;

	if(pthread_create(&g_poll_thread, NULL, net_nfc_client_ipc_thread, &pcond) != 0)

	{
		DEBUG_ERR_MSG("error is occured");
		pthread_attr_destroy(&thread_attr);
		result = NET_NFC_THREAD_CREATE_FAIL;
		goto ERROR;
	}

#ifdef NET_NFC_USE_SIGTERM
	pthread_cond_wait (&pcond, &g_client_lock);
#endif

	DEBUG_CLIENT_MSG("start ipc thread = [%x]", (uint32_t )g_poll_thread);

	//////////////////////////////////

	pthread_attr_destroy(&thread_attr);

#endif // #ifdef G_MAIN_LOOP

	pthread_mutex_unlock(&g_client_lock);

	return NET_NFC_OK;

ERROR:


	DEBUG_ERR_MSG("error while initializing client ipc");

	if(g_client_sock_fd != -1)
	{
		shutdown(g_client_sock_fd, SHUT_RDWR);
		close(g_client_sock_fd);
	}

#ifdef G_MAIN_LOOP

	if(g_client_channel != NULL)
		g_io_channel_unref(g_client_channel);

	if(g_client_src_id != 0)
		g_source_remove(g_client_src_id);

#else

#ifdef USE_IPC_EPOLL
	if(g_client_poll_fd != -1)
	{
		close(g_client_poll_fd);
	}

	ev.events = EPOLLIN | EPOLLET | EPOLLHUP | EPOLLERR;
	ev.data.fd = g_client_sock_fd;
	epoll_ctl(g_client_poll_fd, EPOLL_CTL_DEL, g_client_sock_fd, &ev);
	g_client_poll_fd = -1;

	_net_nfc_client_util_free_mem (g_client_poll_events);

#else
	FD_CLR(g_client_sock_fd, &fdset_read);
#endif


#endif // #ifdef G_MAIN_LOOP

	g_client_sock_fd = -1;

	pthread_mutex_unlock(&g_client_lock);

	return result;
}


#ifdef NET_NFC_USE_SIGTERM
static void thread_sig_handler(int signo)
{
	/* do nothing */
}
#endif

#ifndef G_MAIN_LOOP
static void* net_nfc_client_ipc_thread(void *data)
{
	DEBUG_CLIENT_MSG("net_nfc_client_ipc_thread is started = [0x%x]", pthread_self());


#ifdef NET_NFC_USE_SIGTERM

	struct sigaction act;
	act.sa_handler = thread_sig_handler;
	sigaction(SIGTERM, &act, NULL);

	sigset_t newmask;
	sigemptyset(&newmask);
	sigaddset(&newmask, SIGTERM);
	pthread_sigmask(SIG_UNBLOCK, &newmask, NULL);
	DEBUG_CLIENT_MSG("sighandler is registered");

#endif

#ifdef NET_NFC_USE_SIGTERM
	pthread_mutex_lock(&g_client_lock);
	pthread_cond_signal ((pthread_cond_t *) data);
	pthread_mutex_unlock(&g_client_lock);

#endif

	bool condition = true;

	while(condition == true)
	{
		net_nfc_error_e result = 0;
		if(net_nfc_client_ipc_polling(&result) != true)
		{
			switch(result)
			{
				case NET_NFC_IPC_FAIL:
					DEBUG_CLIENT_MSG("stop ipc polling thread");
					condition = false;
					break;
				default:
					break;
			}
		}
		else
		{
			net_nfc_error_e result = NET_NFC_OK;

			DEBUG_CLIENT_MSG("message is coming from server to client");

			net_nfc_response_msg_t* msg = net_nfc_client_read_response_msg(&result);

			pthread_mutex_lock(&cb_lock);

			if(msg != NULL)
			{

#ifdef USE_GLIB_MAIN_LOOP
				net_nfc_client_call_dispatcher_in_g_main_loop(client_cb, msg);
#elif USE_ECORE_MAIN_LOOP
				net_nfc_client_call_dispatcher_in_ecore_main_loop(client_cb, msg);
#else
				net_nfc_client_call_dispatcher_in_current_context(client_cb, msg);
#endif
			}
			else
			{
				/* if client is wating for unlock signal, then unlock it.
				 * and send error message
				 * or make error reponse message and pass to client cb
				 */
				if (net_nfc_client_is_connected () == false){
					condition = false;
				}
				DEBUG_CLIENT_MSG("can not read response msg or callback is NULL");
			}

			pthread_mutex_unlock(&cb_lock);
		}

	}


	DEBUG_CLIENT_MSG("net_nfc_client_ipc_thread is terminated");

	return (void *)NULL;
}
#endif

#ifdef G_MAIN_LOOP
gboolean net_nfc_client_ipc_callback_func(GIOChannel* channel, GIOCondition condition, gpointer data)
{
	DEBUG_CLIENT_MSG("event is detected on client socket");

	if(G_IO_IN & condition){

		int client_sock_fd = 0;

		if(channel == g_client_channel){

			DEBUG_CLIENT_MSG("message from server to client socket");

			net_nfc_error_e result = NET_NFC_OK;
			net_nfc_response_msg_t* msg = net_nfc_client_read_response_msg(&result);

			if(msg != NULL)
			{

				net_nfc_client_call_dispatcher_in_current_context(client_cb, msg);
			}
			else
			{
				DEBUG_CLIENT_MSG("can not read response msg or callback is NULL");
			}

		}
		else{
			DEBUG_CLIENT_MSG("unknown channel \n");
			return FALSE;
		}
	}
	else{

		DEBUG_CLIENT_MSG("IO ERROR. socket is closed \n");

		/* clean up client context */
		net_nfc_client_socket_finalize();

		return FALSE;
	}

	return TRUE;
}
#else

bool net_nfc_client_ipc_polling(net_nfc_error_e* result)
{
	int num_of_sockets = 0;
	int index = 0;

	if(result == NULL)
	{
		return false;
	}

	*result = NET_NFC_OK;

#ifdef USE_IPC_EPOLL

	DEBUG_CLIENT_MSG("wait event from epoll");

	if(g_client_poll_fd == -1 || g_client_sock_fd == -1)
	{
		DEBUG_CLIENT_MSG("client is deinitialized. ");
		*result = NET_NFC_IPC_FAIL;
		return false;
	}

	/* 0.5sec */
#ifdef USE_EPOLL_TIMEOUT

	while((num_of_sockets = epoll_wait(g_client_poll_fd, g_client_poll_events, EPOLL_SIZE, 300)) == 0){

		if(g_client_poll_fd == -1){
			DEBUG_CLIENT_MSG("client ipc thread is terminated");
			return false;
		}
	}
#else

	num_of_sockets = epoll_wait(g_client_poll_fd, g_client_poll_events, EPOLL_SIZE, -1);

#endif

	for(index = 0; index < num_of_sockets; index++)
	{
		if( (g_client_poll_events[index].events & (EPOLLHUP)) || (g_client_poll_events[index].events & (EPOLLERR)))
		{
			DEBUG_ERR_MSG("connection is closed");
			*result = NET_NFC_IPC_FAIL;
			return false;
		}
		else if(g_client_poll_events[index].events & EPOLLIN)
		{
			if(g_client_poll_events[index].data.fd == g_client_sock_fd)
			{
				return true;
			}
			else
			{
				DEBUG_ERR_MSG("not expected socket connection");
				return false;
			}
		}
		else
		{
			if(num_of_sockets == index)
			{
				DEBUG_ERR_MSG("unknown event");
				return false;
			}
		}


	}

#else

	DEBUG_CLIENT_MSG("wait event from select");


	int temp = select(g_client_sock_fd + 1, &fdset_read, NULL, NULL, NULL);

	if(temp > 0){

		if(FD_ISSET(g_client_sock_fd, &fdset_read) == true){

			int val = 0;
			int size = 0;

			if(getsockopt(g_client_sock_fd, SOL_SOCKET, SO_ERROR, &val, &size) == 0){
				if(val != 0){
					DEBUG_CLIENT_MSG("socket is on error");
					return false;
				}
				else{
					DEBUG_CLIENT_MSG("socket is readable");
					return true;
				}
			}
		}
		else{
			DEBUG_ERR_MSG("unknown error");
			*result = NET_NFC_IPC_FAIL;
			return false;
		}
	}

#endif

	DEBUG_CLIENT_MSG("polling error");
	*result = NET_NFC_IPC_FAIL;
	return false;
}
#endif
net_nfc_error_e net_nfc_client_socket_finalize()
{
	DEBUG_CLIENT_MSG("finalize client socket");

	net_nfc_error_e result = NET_NFC_OK;


	pthread_mutex_lock(&g_client_lock);

	if(g_client_sock_fd != -1)
	{
		shutdown(g_client_sock_fd, SHUT_RDWR);
		DEBUG_CLIENT_MSG("close client socket");
		close(g_client_sock_fd);
	}

#ifdef G_MAIN_LOOP

	g_io_channel_unref(g_client_channel);
	g_client_channel = NULL;

	g_source_remove(g_client_src_id);
	g_client_src_id = 0;

	pthread_mutex_unlock(&g_client_lock);

#else

#ifdef USE_IPC_EPOLL
	if(g_client_poll_fd != -1)
	{
		int ret = close(g_client_poll_fd);
		DEBUG_CLIENT_MSG("close returned %d", ret);
	}
#endif

	pthread_mutex_unlock(&g_client_lock);


#ifdef NET_NFC_USE_SIGTERM
/* This is dangerous because of the lock inside of the thread it creates dead lock!
     it may damage the application data or lockup, sigterm can be recieved while the application codes is executed in callback function
*/
	if(g_poll_thread != (pthread_t)NULL)
		pthread_kill(g_poll_thread, SIGTERM);
#endif

	if(g_poll_thread != (pthread_t)NULL){
		DEBUG_CLIENT_MSG("join epoll_thread start");
		pthread_join (g_poll_thread, NULL);
		DEBUG_CLIENT_MSG("join epoll_thread end");
	}


#ifdef USE_IPC_EPOLL

	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLET | EPOLLHUP | EPOLLERR;
	ev.data.fd = g_client_sock_fd;

	epoll_ctl(g_client_poll_fd, EPOLL_CTL_DEL, g_client_sock_fd, &ev);
	g_client_poll_fd = -1;

#else

	FD_CLR(g_client_sock_fd, &fdset_read);

#endif

	g_poll_thread = (pthread_t)NULL;

#endif // #ifdef G_MAIN_LOOP

	g_client_sock_fd = -1;

	return result;
}



int __net_nfc_client_read_util (void ** detail, size_t size)
{
	static uint8_t flushing[128];

	*detail  = NULL;
	_net_nfc_client_util_alloc_mem (*detail,size);

	/* if the allocation is failed, this codes flush out all the buffered data */
	if (*detail == NULL){
		size_t read_size;
		int readbytes = size;
		while (readbytes > 0){
			read_size = readbytes > 128 ? 128 : readbytes;
			if (recv(g_client_sock_fd, flushing, read_size, 0) <= 0){
				return 0;
			}
			readbytes -= read_size;
		}
		return 0;
	}
	/* read */
	if (recv(g_client_sock_fd, *detail, size, 0) <= 0){
		_net_nfc_client_util_free_mem (*detail);
		return 0;
	}
	return 1;
}


net_nfc_response_msg_t* net_nfc_client_read_response_msg(net_nfc_error_e* result)
{
	net_nfc_response_msg_t* resp_msg = NULL;

	if( (resp_msg = calloc(1, sizeof(net_nfc_response_msg_t))) == NULL)
	{
		DEBUG_ERR_MSG("malloc fail");
		*result = NET_NFC_ALLOC_FAIL;
		return NULL;
	}

	int length = recv(g_client_sock_fd, (void *)&(resp_msg->response_type), sizeof(int), 0);

	DEBUG_CLIENT_MSG("message from server = [%d]. reading lengths = [%d]", resp_msg->response_type, length);

	if(length < 1)
	{
		DEBUG_ERR_MSG("reading message is failed");
		_net_nfc_client_util_free_mem(resp_msg);

		return NULL;
	}

	switch(resp_msg->response_type)
	{

		case NET_NFC_MESSAGE_SET_SE:
		{
			net_nfc_response_set_se_t* resp_detail = NULL;
			int res = 0;
			res = __net_nfc_client_read_util ((void **)&resp_detail, sizeof (net_nfc_response_set_se_t));
			if (res != 1){
				_net_nfc_client_util_free_mem(resp_msg);
				return NULL;
			}
			resp_msg->detail_message = resp_detail;
		}
		break;

		case NET_NFC_MESSAGE_GET_SE:
		{
			net_nfc_response_get_se_t* resp_detail = NULL;
			int res = 0;
			res = __net_nfc_client_read_util ((void **)&resp_detail, sizeof (net_nfc_response_get_se_t));
			if (res != 1){
				_net_nfc_client_util_free_mem(resp_msg);
				return NULL;
			}
			resp_msg->detail_message = resp_detail;
		}
		break;

		case NET_NFC_MESSAGE_OPEN_INTERNAL_SE:
		{
			net_nfc_response_open_internal_se_t* resp_detail = NULL;
			int res = 0;
			res = __net_nfc_client_read_util ((void **)&resp_detail, sizeof (net_nfc_response_open_internal_se_t));
			if (res != 1){
				_net_nfc_client_util_free_mem(resp_msg);
				return NULL;
			}
			DEBUG_CLIENT_MSG("handle = [0x%x]", resp_detail->handle);
			resp_msg->detail_message = resp_detail;
		}
		break;

		case NET_NFC_MESSAGE_CLOSE_INTERNAL_SE:
		{
			net_nfc_response_close_internal_se_t* resp_detail = NULL;
			int res = 0;
			res = __net_nfc_client_read_util ((void **)&resp_detail, sizeof (net_nfc_response_close_internal_se_t));
			if (res != 1){
				_net_nfc_client_util_free_mem(resp_msg);
				return NULL;
			}
			resp_msg->detail_message = resp_detail;
		}
		break;

		case NET_NFC_MESSAGE_SEND_APDU_SE:
		{

			net_nfc_response_send_apdu_t * resp_detail = NULL;
			int res = 0;
			res = __net_nfc_client_read_util ((void **)&resp_detail, sizeof (net_nfc_response_send_apdu_t));
			if (res == 1){
				if(resp_detail->data.length > 0){
					res += __net_nfc_client_read_util ((void **)&(resp_detail->data.buffer),resp_detail->data.length);
					if (res == 2){
						resp_msg->detail_message = resp_detail;
					}
					else{
						_net_nfc_client_util_free_mem (resp_detail);
						res --;
					}
				}
				else {
					resp_msg->detail_message = resp_detail;
					resp_detail->data.buffer = NULL;
				}
			}
			if (res == 0){
				_net_nfc_client_util_free_mem(resp_msg);
				return NULL;
			}
		}
		break;

		case NET_NFC_MESSAGE_TAG_DISCOVERED:
		{
			net_nfc_response_tag_discovered_t * resp_detail = NULL;
			int res = 0;
			res = __net_nfc_client_read_util ((void **)&resp_detail, sizeof (net_nfc_response_tag_discovered_t));
			if (res == 1){
				if(resp_detail->target_info_values.length > 0){
					res += __net_nfc_client_read_util ((void **)&(resp_detail->target_info_values.buffer),resp_detail->target_info_values.length);
					if (res == 2){
						if(resp_detail->raw_data.length > 0){
							res += __net_nfc_client_read_util ((void **)&(resp_detail->raw_data.buffer),resp_detail->raw_data.length);
							if(res == 3){
								resp_msg->detail_message = resp_detail;
							}
							else {
								_net_nfc_client_util_free_mem (resp_detail);
								res --;
							}
						}
						else {
							resp_msg->detail_message = resp_detail;
							resp_detail->raw_data.buffer = NULL;
						}
					}
					else{
						_net_nfc_client_util_free_mem (resp_detail);
						res --;
					}
				}
				else {
					resp_msg->detail_message = resp_detail;
					resp_detail->target_info_values.buffer = NULL;
				}
			}
			if (res == 0){
				_net_nfc_client_util_free_mem(resp_msg);
				return NULL;
			}
		}
		break;

		case NET_NFC_MESSAGE_GET_CURRENT_TAG_INFO:
		{
			net_nfc_response_get_current_tag_info_t * resp_detail = NULL;
			int res = 0;
			res = __net_nfc_client_read_util ((void **)&resp_detail, sizeof (net_nfc_response_get_current_tag_info_t));
			if (res == 1){
				if(resp_detail->target_info_values.length > 0){
					res += __net_nfc_client_read_util ((void **)&(resp_detail->target_info_values.buffer),resp_detail->target_info_values.length);
					if (res == 2){
						if(resp_detail->raw_data.length > 0){
							res += __net_nfc_client_read_util ((void **)&(resp_detail->raw_data.buffer),resp_detail->raw_data.length);
							if(res == 3){
								resp_msg->detail_message = resp_detail;
							}
							else {
								_net_nfc_client_util_free_mem (resp_detail);
								res --;
							}
						}
						else {
							resp_msg->detail_message = resp_detail;
							resp_detail->raw_data.buffer = NULL;
						}
					}
					else{
						_net_nfc_client_util_free_mem (resp_detail);
						res --;
					}
				}
				else {
					resp_msg->detail_message = resp_detail;
					resp_detail->target_info_values.buffer = NULL;
				}
			}
			if (res == 0){
				_net_nfc_client_util_free_mem(resp_msg);
				return NULL;
			}
		}
		break;

		case NET_NFC_MESSAGE_SE_START_TRANSACTION :
		case NET_NFC_MESSAGE_SE_END_TRANSACTION :
		case NET_NFC_MESSAGE_SE_TYPE_TRANSACTION :
		case NET_NFC_MESSAGE_SE_CONNECTIVITY :
		case NET_NFC_MESSAGE_SE_FIELD_ON :
		case NET_NFC_MESSAGE_SE_FIELD_OFF :
		{
			net_nfc_response_se_event_t* resp_detail = NULL;
			int res = 0;
			res = __net_nfc_client_read_util ((void **)&resp_detail, sizeof (net_nfc_response_se_event_t));
			if (res == 1){
				if(resp_detail->aid.length > 0){
					res += __net_nfc_client_read_util ((void **)&(resp_detail->aid.buffer),resp_detail->aid.length);
					if (res == 2){
						if(resp_detail->param.length > 0){
							res += __net_nfc_client_read_util ((void **)&(resp_detail->param.buffer),resp_detail->param.length);
							if(res == 3){
								resp_msg->detail_message = resp_detail;
							}
							else {
								_net_nfc_client_util_free_mem (resp_detail);
								res --;
							}
						}
						else {
							resp_msg->detail_message = resp_detail;
							resp_detail->param.buffer = NULL;
						}
					}
					else{
						_net_nfc_client_util_free_mem (resp_detail);
						res --;
					}
				}
				else {
					resp_msg->detail_message = resp_detail;
					resp_detail->aid.buffer = NULL;
				}
			}
			if (res == 0){
				_net_nfc_client_util_free_mem(resp_msg);
				return NULL;
			}
		}
		break;

		case NET_NFC_MESSAGE_TRANSCEIVE:
		{

			net_nfc_response_transceive_t * resp_detail = NULL;
			int res = 0;
			res = __net_nfc_client_read_util ((void **)&resp_detail, sizeof (net_nfc_response_transceive_t));
			if (res == 1){
				if(resp_detail->data.length > 0){
					res += __net_nfc_client_read_util ((void **)&(resp_detail->data.buffer),resp_detail->data.length);
					if (res == 2){
						resp_msg->detail_message = resp_detail;
					}
					else{
						_net_nfc_client_util_free_mem (resp_detail);
						res --;
					}
				}
				else {
					resp_msg->detail_message = resp_detail;
					resp_detail->data.buffer = NULL;
				}
			}
			if (res == 0){
				_net_nfc_client_util_free_mem(resp_msg);
				return NULL;
			}
		}
		break;

		case NET_NFC_MESSAGE_READ_NDEF:
		{

			net_nfc_response_read_ndef_t * resp_detail = NULL;
			int res = 0;
			res = __net_nfc_client_read_util ((void **)&resp_detail, sizeof (net_nfc_response_read_ndef_t));
			if (res == 1){
				if(resp_detail->data.length > 0){
					res += __net_nfc_client_read_util ((void **)&(resp_detail->data.buffer),resp_detail->data.length);
					if (res == 2){
						resp_msg->detail_message = resp_detail;
					}
					else{
						_net_nfc_client_util_free_mem (resp_detail);
						res --;
					}
				}
				else {
					resp_msg->detail_message = resp_detail;
					resp_detail->data.buffer = NULL;
				}
			}
			if (res == 0){
				_net_nfc_client_util_free_mem(resp_msg);
				return NULL;
			}
		}
		break;

		case NET_NFC_MESSAGE_WRITE_NDEF:
		{
			net_nfc_response_write_ndef_t * resp_detail = NULL;
			int res = 0;
			res = __net_nfc_client_read_util ((void **)&resp_detail, sizeof (net_nfc_response_write_ndef_t));
			if (res != 1){
				_net_nfc_client_util_free_mem(resp_msg);
				return NULL;
			}
			resp_msg->detail_message = resp_detail;
		}
		break;

		case NET_NFC_MESSAGE_SIM_TEST:
		{
			net_nfc_response_test_t * resp_detail = NULL;
			int res = 0;

			DEBUG_CLIENT_MSG("message from server NET_NFC_MESSAGE_SIM_TEST");


			res = __net_nfc_client_read_util ((void **)&resp_detail, sizeof (net_nfc_response_test_t));
			if (res != 1){
				_net_nfc_client_util_free_mem(resp_msg);
				return NULL;
			}
			resp_msg->detail_message = resp_detail;
		}
		break;

		case NET_NFC_MESSAGE_NOTIFY:
		{
			net_nfc_response_notify_t * resp_detail = NULL;
			int res = 0;
			res = __net_nfc_client_read_util ((void **)&resp_detail, sizeof (net_nfc_response_notify_t));
			if (res != 1){
				_net_nfc_client_util_free_mem(resp_msg);
				return NULL;
			}
			resp_msg->detail_message = resp_detail;
		}
		break;
		case NET_NFC_MESSAGE_TAG_DETACHED:
		{
			net_nfc_response_target_detached_t * resp_detail = NULL;
			int res = 0;
			res = __net_nfc_client_read_util ((void **)&resp_detail, sizeof (net_nfc_response_target_detached_t));
			if (res != 1){
				_net_nfc_client_util_free_mem(resp_msg);
				return NULL;
			}
			resp_msg->detail_message = resp_detail;
		}
		break;

		case NET_NFC_MESSAGE_FORMAT_NDEF:
		{

			net_nfc_response_format_ndef_t * resp_detail = NULL;
			int res = 0;
			res = __net_nfc_client_read_util ((void **)&resp_detail, sizeof (net_nfc_response_format_ndef_t));
			if (res != 1){
				_net_nfc_client_util_free_mem(resp_msg);
				return NULL;
			}
			resp_msg->detail_message = resp_detail;

		}
		break;

		case NET_NFC_MESSAGE_LLCP_DISCOVERED:
		{
			net_nfc_response_llcp_discovered_t * resp_detail = NULL;
			int res = 0;
			res = __net_nfc_client_read_util ((void **)&resp_detail, sizeof (net_nfc_response_llcp_discovered_t));
			if (res ==1){
				if(resp_detail->target_info_values.length > 0){
					res += __net_nfc_client_read_util ((void **)&(resp_detail->target_info_values.buffer),resp_detail->target_info_values.length);
					if (res == 2){
						resp_msg->detail_message = resp_detail;
					}
					else{
						_net_nfc_client_util_free_mem (resp_detail);
						res --;
					}
				}
				else {
					resp_msg->detail_message = resp_detail;
					resp_detail->target_info_values.buffer = NULL;
				}
			}
			if (res == 0){
				_net_nfc_client_util_free_mem(resp_msg);
				return NULL;
			}
		}
		break;

		case NET_NFC_MESSAGE_P2P_DETACHED:
		{
			net_nfc_response_llcp_detached_t * resp_detail = NULL;
			int res = 0;
			res = __net_nfc_client_read_util ((void **)&resp_detail, sizeof (net_nfc_response_llcp_detached_t));
			if (res != 1){
				_net_nfc_client_util_free_mem(resp_msg);
				return NULL;
			}
			resp_msg->detail_message = resp_detail;
		}
		break;

		case NET_NFC_MESSAGE_LLCP_LISTEN:
		{
			net_nfc_response_listen_socket_t * resp_detail = NULL;
			int res = 0;
			res = __net_nfc_client_read_util ((void **)&resp_detail, sizeof (net_nfc_response_listen_socket_t));
			if (res != 1){
				_net_nfc_client_util_free_mem(resp_msg);
				return NULL;
			}
			resp_msg->detail_message = resp_detail;

		}
		break;
		case NET_NFC_MESSAGE_LLCP_CONNECT:
		{
			net_nfc_response_connect_socket_t * resp_detail = NULL;
			int res = 0 ;
			res = __net_nfc_client_read_util ((void **)&resp_detail, sizeof (net_nfc_response_connect_socket_t));
			if (res != 1){
				_net_nfc_client_util_free_mem(resp_msg);
				return NULL;
			}
			resp_msg->detail_message = resp_detail;

		}
		break;
		case NET_NFC_MESSAGE_LLCP_CONNECT_SAP:
		{
			net_nfc_response_connect_sap_socket_t * resp_detail = NULL;
			int res = 0 ;
			res = __net_nfc_client_read_util ( (void **)&resp_detail, sizeof (net_nfc_response_connect_sap_socket_t));
			if (res != 1){
				_net_nfc_client_util_free_mem(resp_msg);
				return NULL;
			}
			resp_msg->detail_message = resp_detail;

		}
		break;
		case NET_NFC_MESSAGE_LLCP_SEND:
		{
			net_nfc_response_send_socket_t * resp_detail = NULL;
			int res = 0;
			res = __net_nfc_client_read_util ((void **)&resp_detail, sizeof (net_nfc_response_send_socket_t));
			if (res != 1){
				_net_nfc_client_util_free_mem(resp_msg);
				return NULL;
			}
			resp_msg->detail_message = resp_detail;
		}
		break;

		case NET_NFC_MESSAGE_LLCP_RECEIVE:
		{
			net_nfc_response_receive_socket_t * resp_detail = NULL;
			int res = 0;
			res = __net_nfc_client_read_util ((void **)&resp_detail, sizeof (net_nfc_response_receive_socket_t));
			if (res == 1){

				if(resp_detail->data.length > 0){
					res += __net_nfc_client_read_util ((void **)&(resp_detail->data.buffer),resp_detail->data.length);
				}
				if(res == 2){
					resp_msg->detail_message = resp_detail;
				}
				else {
					_net_nfc_client_util_free_mem (resp_detail);
					res--;
				}
			}
			else{
				resp_msg->detail_message = resp_detail;
				resp_detail->data.buffer = NULL;
			}
		}
		break;

		case NET_NFC_MESSAGE_P2P_RECEIVE:
		{
			net_nfc_response_p2p_receive_t * resp_detail = NULL;
			int res = 0;
			res = __net_nfc_client_read_util ((void **)&resp_detail, sizeof (net_nfc_response_p2p_receive_t));
			if (res == 1){

				if(resp_detail->data.length > 0){
					res += __net_nfc_client_read_util ((void **)&(resp_detail->data.buffer),resp_detail->data.length);
				}
				if(res == 2){
					resp_msg->detail_message = resp_detail;
				}
				else {
					_net_nfc_client_util_free_mem (resp_detail);
					res--;
				}
			}
			else{
				resp_msg->detail_message = resp_detail;
				resp_detail->data.buffer = NULL;
			}
		}
		break;

		case NET_NFC_MESSAGE_SERVICE_LLCP_CLOSE:
		{
			net_nfc_response_close_socket_t * resp_detail = NULL;
			int res = 0;
			res = __net_nfc_client_read_util ((void **)&resp_detail, sizeof (net_nfc_response_close_socket_t));
			if (res != 1){
				_net_nfc_client_util_free_mem(resp_msg);
				return NULL;
			}
			resp_msg->detail_message = resp_detail;
		}
		break;
		case NET_NFC_MESSAGE_LLCP_DISCONNECT:
		{
			net_nfc_response_disconnect_socket_t * resp_detail = NULL;
			int res = 0;
			res = __net_nfc_client_read_util ((void **)&resp_detail, sizeof (net_nfc_response_disconnect_socket_t));
			if (res != 1){
				_net_nfc_client_util_free_mem(resp_msg);
				return NULL;
			}
			resp_msg->detail_message = resp_detail;
		}
		break;

		case NET_NFC_MESSAGE_LLCP_CONFIG:
		{
			net_nfc_response_config_llcp_t * resp_detail = NULL;
			int res = 0;
			res = __net_nfc_client_read_util ((void **)&resp_detail, sizeof (net_nfc_response_config_llcp_t));
			if (res != 1){
				_net_nfc_client_util_free_mem(resp_msg);
				return NULL;
			}
			resp_msg->detail_message = resp_detail;
		}
		break;

		case NET_NFC_MESSAGE_LLCP_ERROR:
		{
			net_nfc_response_llcp_socket_error_t * resp_detail = NULL;
			int res = 0;
			res = __net_nfc_client_read_util ((void **)&resp_detail, sizeof (net_nfc_response_llcp_socket_error_t));
			if (res != 1){
				_net_nfc_client_util_free_mem(resp_msg);
				return NULL;
			}
			resp_msg->detail_message = resp_detail;
		}
		break;

		case NET_NFC_MESSAGE_LLCP_ACCEPTED:
		{
			net_nfc_response_incomming_llcp_t * resp_detail = NULL;
			int res = 0;
			res = __net_nfc_client_read_util ((void **)&resp_detail, sizeof (net_nfc_response_incomming_llcp_t));
			if (res != 1){
				_net_nfc_client_util_free_mem(resp_msg);
				return NULL;
			}
			resp_msg->detail_message = resp_detail;
		}
		break;

		case NET_NFC_MESSAGE_P2P_DISCOVERED :
		{
			net_nfc_response_p2p_discovered_t* resp_detail = NULL;
			int res = 0;

			res = __net_nfc_client_read_util ((void **)&resp_detail, sizeof (net_nfc_response_p2p_discovered_t));
			if (res != 1){
				_net_nfc_client_util_free_mem(resp_msg);
				return NULL;
			}
			resp_msg->detail_message = resp_detail;
		}
		break;

		case NET_NFC_MESSAGE_P2P_SEND :
		{
			net_nfc_response_p2p_send_t* resp_detail = NULL;
			int res = 0;

			res = __net_nfc_client_read_util ((void **)&resp_detail, sizeof (net_nfc_response_p2p_send_t));
			if (res != 1){
				_net_nfc_client_util_free_mem(resp_msg);
				return NULL;
			}
			resp_msg->detail_message = resp_detail;
		}
		break;

		case NET_NFC_MESSAGE_CONNECTION_HANDOVER :
		{
			net_nfc_response_connection_handover_t *resp_detail = NULL;
			int res = 0;

			DEBUG_CLIENT_MSG("NET_NFC_MESSAGE_CONNECTION_HANDOVER");
			res = __net_nfc_client_read_util((void **)&resp_detail, sizeof(net_nfc_response_connection_handover_t));

			if (res == 1)
			{
				if (resp_detail->data.length > 0)
				{
					res += __net_nfc_client_read_util((void **)&(resp_detail->data.buffer), resp_detail->data.length);
					if (res < 2)
					{
						DEBUG_ERR_MSG("__net_nfc_client_read_util failed. res [%d]", res);
						_net_nfc_client_util_free_mem(resp_detail);
						_net_nfc_client_util_free_mem(resp_msg);
						return NULL;
					}
				}
			}
			else
			{
				DEBUG_ERR_MSG("__net_nfc_client_read_util failed. res [%d]", res);
				_net_nfc_client_util_free_mem(resp_msg);
				return NULL;
			}

			resp_msg->detail_message = resp_detail;
		}
		break;

		case NET_NFC_MESSAGE_IS_TAG_CONNECTED:
		{
			net_nfc_response_is_tag_connected_t * resp_detail = NULL;
			int res = 0;
			res = __net_nfc_client_read_util ((void **)&resp_detail, sizeof (net_nfc_response_is_tag_connected_t));
			if (res != 1){
				_net_nfc_client_util_free_mem(resp_msg);
				return NULL;
			}
			resp_msg->detail_message = resp_detail;
		}
		break;

		case NET_NFC_MESSAGE_GET_CURRENT_TARGET_HANDLE:
		{
			net_nfc_response_get_current_target_handle_t * resp_detail = NULL;
			int res = 0;
			res = __net_nfc_client_read_util ((void **)&resp_detail, sizeof (net_nfc_response_get_current_target_handle_t));
			if (res != 1){
				_net_nfc_client_util_free_mem(resp_msg);
				return NULL;
			}
			resp_msg->detail_message = resp_detail;
			
		}
		break;
		
		default:
		{
			DEBUG_CLIENT_MSG("Currently NOT supported RESP TYPE = [%d]", resp_msg->response_type);
			_net_nfc_client_util_free_mem(resp_msg);
			return NULL;
		}

	}

	return resp_msg;
}


bool __net_nfc_client_send_msg(void* message, int length)
{
	DEBUG_CLIENT_MSG("sending message to server [%d]", g_client_sock_fd);

	int ret = send(g_client_sock_fd, (void *)message, length, 0);
	uint8_t  buf[1024] = {0x00,};

	if(ret > 0)
	{
		return true;
	}
	else
	{
		DEBUG_ERR_MSG("sending message is failed");
		DEBUG_CLIENT_MSG ("Returned : %s", strerror_r (ret , (char *)buf , sizeof(buf)));
		return false;
	}
}


static char* cookies = NULL;
static int cookies_size = 0;


void _net_nfc_client_set_cookies (const char * cookie , size_t size)
{
	cookies = (char*) cookie;
	cookies_size = size;

}

void _net_nfc_client_free_cookies (void)
{
	_net_nfc_client_util_free_mem (cookies);
	cookies_size = 0;
}

net_nfc_error_e _net_nfc_client_send_reqeust(net_nfc_request_msg_t *msg, ...)
{
	va_list args;
	int total_size = 0;
	int size = 0;
	int written_size = 0;
	void * data;
	uint8_t* send_buffer = NULL;

	pthread_mutex_lock(&g_client_lock);

	total_size += msg->length;

	va_start(args, msg);
	if (va_arg (args, void *) != NULL)
	{
		while ((size = va_arg(args, int)) != 0)
		{

			total_size += size;

			if (va_arg (args, void *) == NULL)
			{
				break;
			}
		}
	}
	va_end(args);

#ifdef SECURITY_SERVER
	total_size += cookies_size;
#endif

	_net_nfc_client_util_alloc_mem(send_buffer, total_size);
	if (send_buffer == NULL)
	{
		/*  prevent :: unlock mutex before returing */
		pthread_mutex_unlock(&g_client_lock);
		return NET_NFC_ALLOC_FAIL;
	}

#ifdef SECURITY_SERVER
	memcpy(send_buffer, cookies, cookies_size);
	written_size += cookies_size;
#endif

	memcpy(send_buffer + written_size, msg, msg->length);
	written_size += msg->length;

	va_start(args, msg);
	while ((data = va_arg(args, void *)) != NULL)
	{
		size = va_arg (args, int);
		if (size == 0)
			continue;
		memcpy(send_buffer + written_size, data, size);
		written_size += size;
	}
	va_end(args);

	bool msg_result = __net_nfc_client_send_msg((void *)send_buffer, total_size);
	pthread_mutex_unlock(&g_client_lock);

	if (!msg_result)
	{
		/* Sending is failed */
		return NET_NFC_IPC_FAIL;
	}

	DEBUG_CLIENT_MSG("sending request = [%d] is ok", msg->request_type);
	_net_nfc_client_util_free_mem(send_buffer);

	return NET_NFC_OK;
}

net_nfc_error_e _net_nfc_client_register_cb (net_nfc_response_cb cb) {
	if (cb == NULL) {
		return NET_NFC_NULL_PARAMETER;
	}

	pthread_mutex_lock(&cb_lock);
	client_cb = cb;
	pthread_mutex_unlock(&cb_lock);
	return NET_NFC_OK;
}

net_nfc_error_e _net_nfc_client_unregister_cb (void){
	if(client_cb == NULL) {
		return NET_NFC_NOT_REGISTERED;
	}
	pthread_mutex_lock(&cb_lock);
	client_cb = NULL;
	pthread_mutex_unlock(&cb_lock);
	return NET_NFC_OK;
}


