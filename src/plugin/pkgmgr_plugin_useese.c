/*
 * Copyright (c) 2012, 2013 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Flora License, Version 1.1 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	 http://floralicense.org/license/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <pkgmgr-info.h>
#include <glib.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dlog.h>
#include <sys/stat.h>
#include <unistd.h>
//#include <sys/types.h>
//#include <syspopup_caller.h>

//#include "net_nfc.h"

#define CSA_USEESE_PATH "/csa/useese"
#define CSA_USEESE_FILE_PATH "/csa/useese/aid.dat"
#define DBG(fmt, args ...) \
	do { \
		LOGD(fmt, ## args); \
	} while (0)

#ifndef PKGMGR_MODULE_API
#define PKGMGR_MODULE_API __attribute__ ((visibility("default")))
#endif

typedef struct metadata {
	const char *key;
	const char *value;
} metadata;

static GList *list;
static GList *list_metadata;

static int __add_aid_to_csa(const char *aid_string)
{
	int res = 0;
	const int filePerm = 0777;
	int fd = -1;
	int writeLen = 0;
	char buf[1024 + 1];
	int count = 0;
	struct stat stat_buf;
	const char tk = ';';
	const char newline = '\n';
	char *token = NULL;
	char *token_metadata = NULL;
	int token_counter = 0;
	int metadata_token_counter = 0;
	int i = 0;

	memset(&stat_buf, 0, sizeof(struct stat));

	if (stat(CSA_USEESE_PATH, &stat_buf) < 0) {
		if (mkdir(CSA_USEESE_PATH, filePerm) == 0) {
			DBG("mkdir sucess!!!!!");
			res = chmod("/csa/useese", 0777);
			if(res < 0) {
				DBG("fail chmod /csa/useese");
				return -1;
			}
		}
		else {
			DBG("mkdir error");
			return -1;
		}
	}

	//get aid list from csa
	fd = open(CSA_USEESE_FILE_PATH, 00000002 | 00000100, filePerm);
	if (fd >= 0) {
		count = read(fd, buf, 1024);
		///////////
		if (count > 0 && count <= 1024) {
			while (count > 0 && buf[count - 1] == '\n')
				count--;
			buf[count] = '\0';
		} else {
			buf[0] = '\0';
		}
		close(fd);
	}
	///////////
	DBG("count : %d", count);
	if (count > 0) {
		token = strtok(buf, ";");
		while(token) {
			DBG("Token[%d] : %s", token_counter, token);
			list = g_list_append(list, strdup(token));
			token_counter++;
			token = strtok(NULL, ";");
		}
	}

	//get aid list from metadata
	char *aids = strdup(aid_string);
	token_metadata = strtok(aids, ";");
	while(token_metadata) {
		DBG("Metadata Token[%d] : %s", metadata_token_counter, token);
		list_metadata = g_list_append(list_metadata, strdup(token_metadata));
		metadata_token_counter++;
		token_metadata = strtok(NULL, ";");
	}
	//csa vs metadata value
	for (i = 0; i < g_list_length(list_metadata); i++) {
		GList* node;
		node = g_list_nth(list_metadata, i);
		DBG("[%d]th metadata value is %s", i, (char*)node->data);

		if (g_list_find_custom(list, (char*)node->data, (GCompareFunc)strcmp)) {
			DBG("AID [%s] is already exist!", (char*)node->data);
		}
		else {
			DBG("AID [%s] will be added to csa", (char*)node->data);
			list = g_list_append(list, strdup((char*)node->data));
		}
	}

	//re-write the final aid list
	fd = open(CSA_USEESE_FILE_PATH, 00000002 | 00000100, filePerm);
	if (fd >= 0) {
		for (i = 0; i < g_list_length(list); i++) {
			GList* node;
			node = g_list_nth(list, i);
			DBG("[%d]th list value is %s", i, (char*)node->data);

			writeLen = write(fd, (char*)node->data, strlen((char*)node->data));
			DBG("write to csa file : %d", writeLen);
			writeLen = write(fd, &tk, 1);	//temp
			DBG("write to csa file : %d", writeLen);
		}
		writeLen = write(fd, &newline, 1);	//temp
		DBG("write to csa file : %d", writeLen);

		close(fd);
	}

	free(aids);

	return res;
}

static int __install(const char *pkgid, const char *appid, GList *md_list)
{
	GList *list;
	pkgmgrinfo_pkginfo_h handle = NULL;
	int ret;
	char *path = NULL;
	metadata *detail;

	ret = pkgmgrinfo_pkginfo_get_pkginfo(pkgid, &handle);
	if (ret != 0) {
		DBG("pkgmgrinfo_pkginfo_get_pkginfo() [%d]", ret);
		goto END;
	}

	ret = pkgmgrinfo_pkginfo_get_root_path(handle, &path);
	if (ret != 0) {
		DBG("pkgmgrinfo_pkginfo_get_root_path() [%d]", ret);
		goto END;
	}

	DBG("pkgid [%s]", pkgid);
	DBG("root path [%s]", path);

	//get xml path from metadata
	list = g_list_first(md_list);
	detail = (metadata*)list->data;
	DBG("key = %s, value = %s", detail->key, detail->value);

	/* add tail slash */
	ret = __add_aid_to_csa(detail->value);
	DBG("__add_aid_to_csa() result : %d", ret);

//	ret = _nfc_register_aid(buf, pkgid);
//	DBG("_nfc_register_aid() [%d]", ret);

END :
	if (!handle) {
		pkgmgrinfo_pkginfo_destroy_pkginfo(handle);
	}

	return ret;
}

static int __uninstall(const char *pkgid, const char *appid, GList *md_list)
{
	int ret = 0;

//	ret = net_nfc_client_se_remove_package_aids_sync(pkgid);
//	DBG("net_nfc_client_se_remove_route_aid_sync() [%d]", ret);

	return ret;
}

PKGMGR_MODULE_API
int PKGMGR_MDPARSER_PLUGIN_INSTALL(const char *pkgid, const char *appid, GList *md_list)
{
	int ret;

	DBG("PKGMGR_MDPARSER_PLUGIN_INSTALL called");

//	ret = net_nfc_client_initialize();
//	if (ret != 0) {
//		DBG("net_nfc_client_initialize() [%d]", ret);
//		return ret;
//	}

	ret = __install(pkgid, appid, md_list);

//	net_nfc_client_deinitialize();

	return ret;
}


PKGMGR_MODULE_API
int PKGMGR_MDPARSER_PLUGIN_UNINSTALL(const char *pkgid, const char *appid, GList *md_list)
{
	int ret;

	DBG("PKGMGR_MDPARSER_PLUGIN_UNINSTALL called");

//	ret = net_nfc_client_initialize();
//	if (ret != 0) {
//		DBG("net_nfc_client_initialize() [%d]", ret);
//		return ret;
//	}

	ret = __uninstall(pkgid, appid, md_list);

//	net_nfc_client_deinitialize();

	return ret;
}

PKGMGR_MODULE_API
int PKGMGR_MDPARSER_PLUGIN_UPGRADE(const char *pkgid, const char *appid, GList *list)
{
	int ret;

	DBG("PKGMGR_MDPARSER_PLUGIN_UPGRADE called");

//	ret = net_nfc_client_initialize();
//	if (ret != 0) {
//		DBG("net_nfc_client_initialize() [%d]", ret);
//		return ret;
//	}

	ret = __uninstall(pkgid, appid, list);
	if (ret != 0) {
		DBG("__uninstall() [%d]", ret);
	} else {
		DBG("__uninstall() success");
	}

	ret = __install(pkgid, appid, list);
	if (ret != 0) {
		DBG("__install() [%d]", ret);
	} else {
		DBG("__install() success");
	}

//	net_nfc_client_deinitialize();

	return ret;
}
