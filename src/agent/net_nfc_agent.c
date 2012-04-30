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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <glib-object.h>
#include <glib.h>
#include <sys/utsname.h>
#include "vconf.h"
#include "vconf-keys.h"

#include "net_nfc_debug_private.h"
#include "net_nfc_util_defines.h"
#include "net_nfc_dbus_agent_obj_private.h"
#include "net_nfc_agent_private.h"

const char *net_nfc_daemon = "/usr/bin/nfc-manager-daemon";

#define VCONF_FALSE 0
#define VCONF_TRUE 1

#ifndef NET_NFC_SETUP_ENABLE
#define NET_NFC_SETUP_DEFAULT VCONF_FALSE
#endif

static pthread_mutex_t g_queue_lock = PTHREAD_MUTEX_INITIALIZER;
static pid_t manager_pid = 0;
static int child_status = 0;


#define device_file_path_plugin "/usr/lib/libnfc-plugin.so"
#define device_file_path_plugin_65 "/usr/lib/libnfc-plugin-65nxp.so"

int __net_nfc_intialize_dbus_connection();
int __net_nfc_register_child_sig_handler();
void __net_nfc_sig_child_cb(int signo);
void *__net_nfc_fork_thread(void *data);

int main(int argc, char **argv)
{
	int progress = 0;
	GMainLoop *event_loop;
	FILE *fp = NULL;

	if (!g_thread_supported())
	{
		g_thread_init(NULL);
	}
	dbus_g_thread_init();
	g_type_init();

	/* reset progress key */
	if (vconf_get_int(NET_NFC_VCONF_KEY_PROGRESS, &progress) == 0)
	{
		DEBUG_MSG("==========================================");
		DEBUG_MSG("Old NET_NFC_VCONF_KEY_PROGRESS value is %d", progress);
		DEBUG_MSG("==========================================");
	}

	if (vconf_set_int(NET_NFC_VCONF_KEY_PROGRESS, 0) != 0)
	{
		DEBUG_ERR_MSG("vconf_set_int failed");
	}

	/* run manager first */
	__net_nfc_fork_thread(NULL);

	if (__net_nfc_intialize_dbus_connection() != VCONF_TRUE)
	{
		DEBUG_ERR_MSG("DBUS: intialize dbus is failed");
	}
	if (__net_nfc_register_child_sig_handler() != VCONF_TRUE)
	{
		DEBUG_ERR_MSG("SIGCHLD: registering sig child is failed");
	}

	/* to check H/W which dose not support NFC */

	fp = fopen(device_file_path_plugin, "r");
	if(fp == NULL)
	{
		fp = fopen(device_file_path_plugin_65, "r");
		if(fp == NULL)
		{
			vconf_set_bool(VCONFKEY_NFC_FEATURE, VCONFKEY_NFC_FEATURE_OFF);
			vconf_set_bool(VCONFKEY_NFC_STATE, FALSE);
			DEBUG_ERR_MSG("There is NO plugin");
		}
		else
		{
			vconf_set_bool(VCONFKEY_NFC_FEATURE, VCONFKEY_NFC_FEATURE_ON);
			DEBUG_ERR_MSG("There is device_file_path_plugin_65");
			fclose(fp);
		}
	}
	else
	{
		vconf_set_bool(VCONFKEY_NFC_FEATURE, VCONFKEY_NFC_FEATURE_ON);
		DEBUG_ERR_MSG("There is  device_file_path_plugin ");

		fclose(fp);
	}


	event_loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(event_loop);

	return 0;
}

int __net_nfc_register_child_sig_handler()
{
	struct sigaction act;

	act.sa_handler = __net_nfc_sig_child_cb;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_NOCLDSTOP;

	if (sigaction(SIGCHLD, &act, NULL) < 0)
	{
		return VCONF_FALSE;
	}
	return VCONF_TRUE;
}

void *__net_nfc_fork_thread(void *data)
{
	int state, progress;
	int result;

	if (data != NULL)
	{
		DEBUG_MSG("child process is terminated with: %d ", *((int *)data));
	}

	result = vconf_get_bool(VCONFKEY_NFC_STATE, &state);
	if (result != 0)
	{
		DEBUG_MSG("VCONFKEY_NFC_STATE is not exist: %d ", result);
		return NULL;
	}

	result = vconf_get_int(NET_NFC_VCONF_KEY_PROGRESS, &progress);
	if (result != 0)
	{
		DEBUG_MSG("NET_NFC_VCONF_KEY_PROGRESS is not exist: %d ", result);

		progress = 0;
		result = vconf_set_int(NET_NFC_VCONF_KEY_PROGRESS, progress);
		if (result != 0)
		{
			DEBUG_ERR_MSG("vconf_set_int failed");
			return NULL;
		}
	}

	if (progress != 0)
	{
		DEBUG_ERR_MSG("Error occur in progress [%d]", progress);

		/* off */
		result = vconf_set_bool(VCONFKEY_NFC_STATE, 0);
		if (result != 0)
		{
			DEBUG_MSG("vconf_set_int failed", result);
		}
		return NULL;
	}

	if (state == TRUE)
	{
		DEBUG_MSG("Vconf value is true");
		if (_net_nfc_launch_net_nfc_manager() == VCONF_FALSE)
		{
			DEBUG_ERR_MSG("sigchild is called");
		}
	}

	return NULL;
}

void __net_nfc_sig_child_cb(int signo)
{

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_t sig_thread;
	pid_t child_pid;
	int status;

	child_pid = waitpid(-1, &status, 0);

	child_status = status;

	// do not use lock here!
	manager_pid = 0;

	pthread_create(&sig_thread, &attr, __net_nfc_fork_thread, &child_status);
}

int _net_nfc_is_terminated()
{
	return (manager_pid == 0);
}

int _net_nfc_launch_net_nfc_manager()
{
	pthread_mutex_lock(&g_queue_lock);
	if (manager_pid == 0)
	{
		manager_pid = fork();
		if (manager_pid == 0)
		{
			if (-1 == execl(net_nfc_daemon, net_nfc_daemon, NULL))
			{
				exit(0);
			}
		}
	}
	else
	{
		DEBUG_MSG("NFC-manager is already running");
		pthread_mutex_unlock(&g_queue_lock);
		return VCONF_FALSE;
	}
	DEBUG_MSG("NFC-manager's PID %d", manager_pid);
	pthread_mutex_unlock(&g_queue_lock);
	return VCONF_TRUE;
}

int _net_nfc_terminate_net_nfc_manager()
{
	int status;

	pthread_mutex_lock(&g_queue_lock);
	if (manager_pid)
	{
		DEBUG_MSG("NFC-manager's PID %d", manager_pid);
		kill(manager_pid, SIGKILL);
		wait(&status);
		manager_pid = 0;
		pthread_mutex_unlock(&g_queue_lock);
		return VCONF_TRUE;
	}
	pthread_mutex_unlock(&g_queue_lock);
	return VCONF_FALSE;
}

int __net_nfc_intialize_dbus_connection()
{
	GError *error = NULL;
	DBusGProxy *proxy;
	GObject *object = NULL;
	guint32 name;
	DBusGConnection *connection;

	connection = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);
	if (connection == NULL)
	{
		DEBUG_ERR_MSG("DBUS: getting dbus connection is failed [%s]", error->message);
		g_error_free(error);

		return VCONF_FALSE;
	}

	object = g_object_new(DBUS_AGENT_TYPE, NULL);

	dbus_g_connection_register_g_object(connection, DBUS_AGENT_PATH, object);

	proxy = dbus_g_proxy_new_for_name(connection, DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS);

	// Make sure getting name is correct version or not
	if (!org_freedesktop_DBus_request_name(proxy, DBUS_AGENT_NAME, 0, &name, &error))
	{
		DEBUG_ERR_MSG("DBUS: getting dbus proxy is failed [%s]", error->message);
		g_error_free(error);

		return VCONF_FALSE;
	}

	if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != name)
	{
		DEBUG_MSG("Requested name is: %d", name);
	}

	return VCONF_TRUE;
}

