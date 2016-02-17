/*
 * Copyright (c) 2012, 2013 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Flora License, Version 1.1 (the "License");
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

/* standard library header */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <glib.h>
#include <gio/gio.h>

/* SLP library header */
#include "cert-service.h"
#include "pkgmgr-info.h"
#include "bincfg.h"

/* local header */
#include "net_nfc_typedef_internal.h"
#include "net_nfc_debug_internal.h"
#include "net_nfc_util_internal.h"
#include "net_nfc_util_gdbus_internal.h"
#include "net_nfc_server_common.h"
#include "net_nfc_service_access_control.h"

static GDBusProxy *dbus_proxy;

static bool __check_platform_certificate(pid_t pid);
static bool __check_se_dependant_access_rule(
	uint8_t se_type, const char *package);
static bool __check_ese_access_rule(pid_t pid);


bool net_nfc_service_check_access_control(pid_t pid,
	net_nfc_access_control_type_e type)
{
	bool result = false;
	int ret;
	char package[1024];

	if (bincfg_get_binary_type() == BIN_TYPE_FACTORY) {
		result = true;
		goto END;
	}

	if (type & NET_NFC_ACCESS_CONTROL_PLATFORM) {
		result = __check_platform_certificate(pid);
		if (result == true)
			goto END;
	}

	/* get package id from pid */
	ret = net_nfc_util_get_pkgid_by_pid(pid, package, sizeof(package));
	if (ret < 0) {
		DEBUG_ERR_MSG("aul_app_get_pkgname_bypid failed [%d]", ret);
		goto END;
	}

	if (type & NET_NFC_ACCESS_CONTROL_UICC) {
		result = __check_se_dependant_access_rule(
			NET_NFC_SE_TYPE_UICC, package);
		if (result == true)
			goto END;
	}

	if (type & NET_NFC_ACCESS_CONTROL_ESE) {
		result = __check_ese_access_rule(pid);
		if (result == true)
			goto END;
	}

END :
	return result;
}

static gboolean __call_is_authorized_nfc_access_sync(GDBusProxy *proxy,
	guint arg_se_type,
	const gchar *arg_package,
	GVariant *arg_aid,
	gboolean *out_result)
{
	GVariant *var;
	GError *error = NULL;

	var = g_dbus_proxy_call_sync(proxy,
		"isAuthorizedNfcAccess",
		g_variant_new("(us@a(y))", arg_se_type,
			arg_package, arg_aid),
		G_DBUS_CALL_FLAGS_NONE, -1,
		NULL,
		&error);
	if (var != NULL) {
		g_variant_get(var, "(b)", (gboolean *)out_result);
		g_variant_unref(var);
	} else {
		DEBUG_ERR_MSG("g_dbus_proxy_call_sync failed, [%s]", error->message);
		g_error_free(error);
	}

	return (var != NULL);
}

bool net_nfc_service_access_control_is_authorized_nfc_access(uint8_t se_type,
	const char *package, const uint8_t *aid, const uint32_t len)
{
	gboolean result = false;
	GVariant *var_aid = NULL;
	int type = 0;
	GError *error = NULL;

	SECURE_MSG("check nfc access, [%s]", package);

#ifndef CHECK_NFC_ACCESS_FOR_ESE
	if (se_type == NET_NFC_SE_TYPE_ESE) {
		return true;
	}
#endif
	if (dbus_proxy == NULL) {

		dbus_proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
			G_DBUS_PROXY_FLAGS_NONE,
			NULL, /* GDBusInterfaceInfo */
			"org.tizen.SmartcardService",
			"/org/tizen/SmartcardService/AccessControl",
			"org.tizen.SmartcardService.AccessControl",
			NULL, /* GCancellable */
			&error);
		if (dbus_proxy == NULL) {
			DEBUG_ERR_MSG("g_dbus_proxy_new_for_bus_sync failed");
			return false;
		}
	}

	/* se type value
	 *
	 * 0x1? : UICC, low 4 bits indicates UICC number.
	 *        (ex, 0x10 = "SIM1", 0x11 = "SIM2", ...
	 * 0x2x : eSE. (low 4 bits doesn't be cared)
	 * 0x3x : SD Card..??
	 */
	switch (se_type) {
	case NET_NFC_SE_TYPE_UICC :
		type = 0x10;
		break;

#ifdef CHECK_NFC_ACCESS_FOR_ESE
	case NET_NFC_SE_TYPE_ESE :
		type = 0x20;
		break;
#endif
	case NET_NFC_SE_TYPE_SDCARD :
		type = 0x30;
		break;

	default :
		return false;
	}

	var_aid = net_nfc_util_gdbus_buffer_to_variant(aid, len);

	/* check access */
	if (__call_is_authorized_nfc_access_sync(
		dbus_proxy,
		type, package, var_aid, &result) == false) {
		DEBUG_ERR_MSG("_call_is_authorized_nfc_access_sync failed");

		return false;
	}

	SECURE_MSG("result : %s", result ? "true" : "false");

	return (result == true);
}

bool net_nfc_service_access_control_is_authorized_nfc_access_by_pid(
	uint8_t se_type, pid_t pid, const uint8_t *aid, const uint32_t len)
{
	char package[1024];
	int ret;

	/* get package id from pid */
	ret = net_nfc_util_get_pkgid_by_pid(pid, package, sizeof(package));
	if (ret < 0) {
		DEBUG_ERR_MSG("net_nfc_util_get_pkgid_by_pid failed [%d]", ret);
		return false;
	}

	return net_nfc_service_access_control_is_authorized_nfc_access(se_type,
		package, aid, len);
}
#if 0
static gboolean __call_is_authorized_extra_access_sync(GDBusProxy *proxy,
	guint arg_se_type,
	const gchar *arg_package,
	gboolean *out_result)
{
	GVariant *var;
	GError *error = NULL;

	var = g_dbus_proxy_call_sync(proxy,
		"isAuthorizedExtraAccess",
		g_variant_new("(us)", arg_se_type,
			arg_package),
		G_DBUS_CALL_FLAGS_NONE, -1,
		NULL,
		&error);
	if (var != NULL) {
		g_variant_get(var, "(b)", (gboolean *)out_result);
		g_variant_unref(var);
	} else {
		DEBUG_ERR_MSG("g_dbus_proxy_call_sync failed, [%s]", error->message);
		g_error_free(error);
	}

	return (var != NULL);
}
#endif
static void __is_authorized_extra_access_cb(GObject *source_object,
	GAsyncResult *res,
	gpointer user_data)
{
	GError *error = NULL;
	GVariant *_ret;

	_ret = g_dbus_proxy_call_finish(G_DBUS_PROXY(source_object), res, &error);
	if (_ret != NULL) {
		g_variant_get(_ret, "(b)", (gboolean *)user_data);
		g_variant_unref(_ret);
	} else {
		DEBUG_ERR_MSG("g_dbus_proxy_call_sync failed, [%s]", error->message);
		g_error_free(error);
		*(gboolean *)user_data = false;
	}

	net_nfc_server_controller_quit_dispatch_loop();
}

static gboolean __call_is_authorized_extra_access(GDBusProxy *proxy,
	guint arg_se_type,
	const gchar *arg_package,
	GAsyncReadyCallback callback,
	gpointer user_param)
{
	g_dbus_proxy_call(proxy,
		"isAuthorizedExtraAccess",
		g_variant_new("(us)", arg_se_type,
			arg_package),
		G_DBUS_CALL_FLAGS_NONE, -1,
		NULL,
		callback,
		user_param);

	return true;
}

static bool __check_se_dependant_access_rule(uint8_t se_type,
	const char *package)
{
	gboolean result = false;
	int type = 0;
	GError *error = NULL;

	SECURE_MSG("check extra access, [%s]", package);

	if (dbus_proxy == NULL) {

		dbus_proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
			G_DBUS_PROXY_FLAGS_NONE,
			NULL, /* GDBusInterfaceInfo */
			"org.tizen.SmartcardService",
			"/org/tizen/SmartcardService/AccessControl",
			"org.tizen.SmartcardService.AccessControl",
			NULL, /* GCancellable */
			&error);
		if (dbus_proxy == NULL) {
			DEBUG_ERR_MSG("g_dbus_proxy_new_for_bus_sync failed");
			return false;
		}
	}

	/* se type value
	 *
	 * 0x1? : UICC, low 4 bits indicates UICC number.
	 *        (ex, 0x10 = "SIM1", 0x11 = "SIM2", ...
	 * 0x2x : eSE. (low 4 bits doesn't be cared)
	 * 0x3x : SD Card..??
	 */
	switch (se_type) {
	case NET_NFC_SE_TYPE_UICC :
		type = 0x10;
		break;

	case NET_NFC_SE_TYPE_ESE :
		type = 0x20;
		break;

	case NET_NFC_SE_TYPE_SDCARD :
		type = 0x30;
		break;

	default :
		return false;
	}

	/* check access */
	if (__call_is_authorized_extra_access(
		dbus_proxy,
		type, package,
		__is_authorized_extra_access_cb,
		&result) == false) {
		DEBUG_ERR_MSG("_call_is_authorized_extra_access_sync failed");

		return false;
	}

	net_nfc_server_controller_run_dispatch_loop();

	SECURE_MSG("result : %s", result ? "true" : "false");

	return (result == true);
}

static bool __check_certificate_level(pid_t pid, int mask)
{
	bool result = false;
	int ret = 0;
	pkgmgrinfo_certinfo_h certinfo = NULL;
	char pkgid[1024];
	int type;

	if (net_nfc_util_get_pkgid_by_pid(pid, pkgid, sizeof(pkgid)) == false) {
		DEBUG_ERR_MSG("net_nfc_util_get_pkgid_by_pid failed");

		goto END;
	}

	/* load certificate hashes */
	ret = pkgmgrinfo_pkginfo_create_certinfo(&certinfo);
	if (ret < 0) {
		DEBUG_ERR_MSG("pkgmgr_pkginfo_create_certinfo failed, [%d]", ret);

		goto END;
	}

	ret = pkgmgrinfo_pkginfo_load_certinfo(pkgid, certinfo);
	if (ret < 0) {
		DEBUG_ERR_MSG("pkgmgr_pkginfo_load_certinfo, failed [%d]", ret);

		goto END;
	}

	/* check permission */
	for (type = (int)PMINFO_AUTHOR_ROOT_CERT;
		type <= (int)PMINFO_DISTRIBUTOR2_SIGNER_CERT; type++)
	{
		const char *value = NULL;

		ret = pkgmgrinfo_pkginfo_get_cert_value(certinfo,
			(pkgmgrinfo_cert_type)type, &value);
		if (ret == PMINFO_R_OK &&
			value != NULL && strlen(value) > 0)
		{
			int visibility = 0;

			ret = cert_svc_get_visibility_by_root_certificate(value, strlen(value), &visibility);
			if (ret != CERT_SVC_ERR_NO_ERROR) {
				if (ret != CERT_SVC_ERR_NO_ROOT_CERT) {
					SECURE_MSG("cert_svc_get_visibility_by_root_certificate failed, type [%d], error [%d]", type, ret);
				}
				continue;
			}

			if (visibility & mask) {
				SECURE_MSG("found!! type [%d], visibility [%08X]", type, visibility);

				result = true;
				break;
			}
		}
	}

END :
	if (certinfo != NULL) {
		pkgmgrinfo_pkginfo_destroy_certinfo(certinfo);
	}

	return result;
}

static bool __check_platform_certificate(pid_t pid)
{
	bool result = false;

	SECURE_MSG("check platform level, [%d]", pid);

	result = __check_certificate_level(pid, CERT_SVC_VISIBILITY_PLATFORM);

	SECURE_MSG("result : %s", result ? "true" : "false");

	return result;
}

static bool __check_ese_access_rule(pid_t pid)
{
	bool result = false;
	int mask = CERT_SVC_VISIBILITY_PLATFORM | CERT_SVC_VISIBILITY_PARTNER_MANUFACTURER | CERT_SVC_VISIBILITY_PARTNER_OPERATOR | CERT_SVC_VISIBILITY_PARTNER;

	SECURE_MSG("check ese rule, [%d]", pid);

	result = __check_certificate_level(pid, mask);

	SECURE_MSG("result : %s", result ? "true" : "false");

	return result;
}
