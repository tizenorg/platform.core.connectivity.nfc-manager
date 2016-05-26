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

#include <sqlite3.h>

#include "vconf.h"

#include "net_nfc_debug_internal.h"
#include "net_nfc_util_internal.h"
#include "net_nfc_controller_internal.h"
#include "net_nfc_server.h"
#include "net_nfc_server_manager.h"
#include "net_nfc_server_se.h"
#include "net_nfc_server_vconf.h"
#include "net_nfc_server_route_table.h"
#include "net_nfc_app_util_internal.h"

#define PRINT_TABLE
#define AID_MAX		50

/* route table database */
#define NFC_ROUTE_TABLE_DB_FILE "/opt/usr/data/nfc-manager-daemon/.route_table.db"
#define NFC_ROUTE_TABLE_DB_TABLE "route_table"

typedef void (*_iterate_db_cb)(const char *package, net_nfc_se_type_e se_type,
	net_nfc_card_emulation_category_t category, const char *aid,
	bool unlock, int power, bool manifest, void *user_data);

static sqlite3 *db;
static sqlite3_stmt *current_stmt;
static int count = 0;

static net_nfc_error_e __route_table_add_aid(const char *id,
	const char *package, net_nfc_se_type_e se_type,
	net_nfc_card_emulation_category_t category,
	const char *aid, bool unlock, int power, bool manifest, bool routing);

static net_nfc_error_e __route_table_del_aid(const char *id,
	const char *package, const char *aid, bool force);

static net_nfc_error_e __insert_into_db(const char *package, net_nfc_se_type_e se_type,
	net_nfc_card_emulation_category_t category, const char *aid,
	bool unlock, int power, bool manifest);

static void __set_payment_handler(const char *package);

static bool __is_table_existing(const char *table)
{
	bool result;
	char *sql;
	int ret;

	sql = sqlite3_mprintf("SELECT count(*) FROM sqlite_master WHERE type='table' AND name ='%s';", table);
	if (sql != NULL) {
		sqlite3_stmt *stmt = NULL;

		ret = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
		if (ret == SQLITE_OK) {
			ret = sqlite3_step(stmt);
			if (ret == SQLITE_ROW) {
				int count;

				count = sqlite3_column_int(stmt, 0);
				if (count > 0) {
					result = true;
				} else {
					result = false;
				}
			} else {
				DEBUG_ERR_MSG("sqlite3_step failed, [%d:%s]", ret, sqlite3_errmsg(db));

				result = false;
			}

			sqlite3_finalize(stmt);
		} else {
			DEBUG_ERR_MSG("sqlite3_prepare_v2 failed, [%d:%s]", ret, sqlite3_errmsg(db));

			result = false;
		}

		sqlite3_free(sql);
	} else {
		DEBUG_ERR_MSG("sqlite3_mprintf failed");

		result = false;
	}

	return result;
}

static void __create_table()
{
	int ret;
	char *sql;

	sql = sqlite3_mprintf("CREATE TABLE %s(idx INTEGER PRIMARY KEY, package TEXT NOT NULL, se_type INTEGER, category INTEGER, aid TEXT NOT NULL COLLATE NOCASE, unlock INTEGER, power INTEGER, manifest INTEGER);", NFC_ROUTE_TABLE_DB_TABLE);
	if (sql != NULL) {
		char *error = NULL;

		ret = sqlite3_exec(db, sql, NULL, NULL, &error);
		if (ret != SQLITE_OK) {
			DEBUG_ERR_MSG("sqlite3_exec() failed, [%d:%s]", ret, error);

			sqlite3_free(error);
		}

		sqlite3_free(sql);
	} else {
		DEBUG_ERR_MSG("sqlite3_mprintf failed");
	}
}

static void __prepare_table()
{
	if (__is_table_existing(NFC_ROUTE_TABLE_DB_TABLE) == false) {
		__create_table();
	}
}

static void __initialize_db()
{
	int result;
	char *error = NULL;

	if (db == NULL) {
		result = sqlite3_open_v2(NFC_ROUTE_TABLE_DB_FILE, &db,
			SQLITE_OPEN_NOMUTEX | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
			NULL);
		if (result != SQLITE_OK) {
			DEBUG_ERR_MSG("sqlite3_open_v2 failed, [%d]", result);

			goto ERR;
		}

		/* Enable persist journal mode */
		result = sqlite3_exec(db, "PRAGMA journal_mode = PERSIST",
			NULL, NULL, &error);
		if (result != SQLITE_OK) {
			DEBUG_ERR_MSG("Fail to change journal mode: %s", error);
			sqlite3_free(error);

			goto ERR;
		}

		__prepare_table();
	}

	return;

ERR :
	if (db != NULL) {
		result = sqlite3_close(db);
		if (result == SQLITE_OK) {
			db = NULL;
		} else {
			DEBUG_ERR_MSG("sqlite3_close failed, [%d]", result);
		}
	}
}

static void __finalize_db()
{
	int result;

	if (db != NULL) {
		if (current_stmt != NULL) {
			result = sqlite3_finalize(current_stmt);
			if (result != SQLITE_OK) {
				DEBUG_ERR_MSG("sqlite3_finalize failed, [%d]", result);
			}
		}

		result = sqlite3_close(db);
		if (result == SQLITE_OK) {
			db = NULL;
		} else {
			DEBUG_ERR_MSG("sqlite3_close failed, [%d]", result);
		}
	}
}

static void __iterate_db(_iterate_db_cb cb, void *user_data)
{
	char *sql;

	if (cb == NULL) {
		return;
	}

	sql = sqlite3_mprintf("SELECT * FROM %s;", NFC_ROUTE_TABLE_DB_TABLE);
	if (sql != NULL) {
		sqlite3_stmt *stmt = NULL;
		int ret;

		ret = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
		if (ret == SQLITE_OK) {
			const char *package;
			net_nfc_se_type_e se_type;
			net_nfc_card_emulation_category_t category;
			const char *aid;
			bool unlock;
			int power;
			bool manifest;

			while ((ret = sqlite3_step(stmt)) == SQLITE_ROW) {
				package = (const char *)sqlite3_column_text(stmt, 1);
				se_type = (net_nfc_se_type_e)sqlite3_column_int(stmt, 2);
				category = (net_nfc_card_emulation_category_t)sqlite3_column_int(stmt, 3);
				aid = (const char *)sqlite3_column_text(stmt, 4);
				unlock = (bool)sqlite3_column_int(stmt, 5);
				power = sqlite3_column_int(stmt, 6);
				manifest = (bool)sqlite3_column_int(stmt, 7);

				cb(package, se_type, category, aid, unlock, power, manifest, user_data);
			}

			sqlite3_finalize(stmt);
		} else {
			DEBUG_ERR_MSG("sqlite3_prepare_v2 failed, [%d:%s]", ret, sqlite3_errmsg(db));
		}

		sqlite3_free(sql);
	} else {
		DEBUG_ERR_MSG("sqlite3_mprintf failed");
	}
}

static net_nfc_error_e __insert_into_db(const char *package, net_nfc_se_type_e se_type,
	net_nfc_card_emulation_category_t category, const char *aid,
	bool unlock, int power, bool manifest)
{
	net_nfc_error_e result;
	char *sql;

	sql = sqlite3_mprintf("INSERT INTO %s (package, se_type, category, aid, unlock, power, manifest) values(?, %d, %d, ?, %d, %d, %d);",
		NFC_ROUTE_TABLE_DB_TABLE, se_type, category, (int)unlock, power, (int)manifest);
	if (sql != NULL) {
		sqlite3_stmt *stmt = NULL;
		int ret;

		ret = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
		if (ret == SQLITE_OK) {
			ret = sqlite3_bind_text(stmt, 1, package,
				strlen(package), SQLITE_STATIC);
			if (ret != SQLITE_OK) {
				DEBUG_ERR_MSG("sqlite3_bind_text failed, [%d]", ret);

				result = NET_NFC_OPERATION_FAIL;
				goto END;
			}

			ret = sqlite3_bind_text(stmt, 2, aid,
				strlen(aid), SQLITE_STATIC);
			if (ret != SQLITE_OK) {
				DEBUG_ERR_MSG("sqlite3_bind_text failed, [%d]", ret);

				result = NET_NFC_OPERATION_FAIL;
				goto END;
			}

			ret = sqlite3_step(stmt);
			if (ret != SQLITE_DONE) {
				DEBUG_ERR_MSG("sqlite3_step failed, [%d:%s]", ret, sqlite3_errmsg(db));

				result = NET_NFC_OPERATION_FAIL;
				goto END;
			}

			result = NET_NFC_OK;
END :
			sqlite3_finalize(stmt);
		} else {
			DEBUG_ERR_MSG("sqlite3_prepare_v2 failed, [%d:%s]", ret, sqlite3_errmsg(db));

			result = NET_NFC_OPERATION_FAIL;
		}

		sqlite3_free(sql);
	} else {
		DEBUG_ERR_MSG("sqlite3_mprintf failed");

		result = NET_NFC_ALLOC_FAIL;
	}

	return result;
}

static net_nfc_error_e __delete_from_db(const char *package, const char *aid)
{
	net_nfc_error_e result;
	char *sql;
	char *error = NULL;

	sql = sqlite3_mprintf("DELETE FROM %s WHERE package=%Q AND aid=%Q;",
		NFC_ROUTE_TABLE_DB_TABLE, package, aid);
	if (sql != NULL) {
		int ret;

		ret = sqlite3_exec(db, sql, NULL, NULL, &error);
		if (ret == SQLITE_OK) {
			result = NET_NFC_OK;
		} else {
			DEBUG_ERR_MSG("sqlite3_exec failed, [%d:%s]", ret, error);

			result = NET_NFC_OPERATION_FAIL;
			sqlite3_free(error);
		}

		sqlite3_free(sql);
	} else {
		DEBUG_ERR_MSG("sqlite3_mprintf failed");

		result = NET_NFC_ALLOC_FAIL;
	}

	return result;
}
#if 0
static net_nfc_error_e __delete_aids_from_db(const char *package)
{
	net_nfc_error_e result;
	char *sql;
	char *error = NULL;

	sql = sqlite3_mprintf("DELETE FROM %s WHERE package=%Q;",
		NFC_ROUTE_TABLE_DB_TABLE, package);
	if (sql != NULL) {
		int ret;

		ret = sqlite3_exec(db, sql, NULL, NULL, &error);
		if (ret == SQLITE_OK) {
			result = NET_NFC_OK;
		} else {
			DEBUG_ERR_MSG("sqlite3_exec failed, [%d:%s]", ret, error);

			result = NET_NFC_OPERATION_FAIL;
			sqlite3_free(error);
		}

		sqlite3_free(sql);
	} else {
		DEBUG_ERR_MSG("sqlite3_mprintf failed");

		result = NET_NFC_ALLOC_FAIL;
	}

	return result;
}
#endif
////////////////////////////////////////////////////////////////////////////////

static char *preferred_payment;
static char *activated_payment;

static char *preferred_other;
static GPtrArray *activated_other;
static int maximum_aids_count = AID_MAX;
static int used_aids_count[NET_NFC_CARD_EMULATION_CATEGORY_MAX + 1];

static void __update_payment_handler(const char *package)
{
	if (activated_payment != NULL) {
		g_free(activated_payment);
	}

	if (package != NULL) {
		activated_payment = g_strdup(package);
	}
}

static bool __is_preferred_payment_handler(const char *package)
{
	return (preferred_payment != NULL &&
		g_strcmp0(package, preferred_payment) == 0);
}

static bool __is_payment_handler(const char *package)
{
	return (activated_payment != NULL &&
		g_strcmp0(package, activated_payment) == 0);
}

static void __set_payment_handler(const char *package)
{
	int result;

	result = vconf_set_str(VCONFKEY_NFC_PAYMENT_HANDLERS,
		package != NULL ? package : "");
	if (result < 0) {
		DEBUG_ERR_MSG("vconf_set_str(VCONFKEY_NFC_PAYMENT_HANDLER) failed, [%d]", result);
	}
}

static void __on_destroy_other_cb(gpointer data)
{
	g_free(data);
}

static void __update_other_handler(const char *packages)
{
	if (activated_other != NULL) {
		g_ptr_array_free(activated_other, true);
	}

	if (packages != NULL) {
		gchar **tokens;
		gchar **ptr;

		activated_other = g_ptr_array_new_full(10,
			__on_destroy_other_cb);

		tokens = g_strsplit(packages, "|", -1);

		for (ptr = tokens; *ptr != NULL; ptr++) {
			g_ptr_array_add(activated_other, g_strdup(*ptr));
		}

		g_strfreev(tokens);
	}
}

void net_nfc_server_route_table_update_other_handler(const char *packages)
{
	__update_other_handler(packages);

	net_nfc_server_route_table_do_update(net_nfc_server_manager_get_active());
}

static bool __is_preferred_other_handler(const char *package)
{
	return (preferred_other != NULL &&
		g_strcmp0(package, preferred_other) == 0);
}

static bool __is_other_handler(const char *package)
{
	bool result = false;
	int i;

	for (i = 0; i < activated_other->len; i++) {
		if (g_strcmp0(package, activated_other->pdata[i]) == 0) {
			result = true;
			break;
		}
	}

	return result;
}

static void __remove_other_handler(const char *package)
{
	int i, found = -1, offset = 0;
	char built[4096] = { 0, };

	for (i = 0; i < activated_other->len; i++) {
		if (g_strcmp0(package, (char *)activated_other->pdata[i]) == 0) {
			found = i;
		} else {
			if (i != activated_other->len - 1) {
				offset += snprintf(built + offset,
					sizeof(built) - offset, "%s|",
					(char *)activated_other->pdata[i]);
			} else {
				offset += snprintf(built + offset,
					sizeof(built) - offset, "%s",
					(char *)activated_other->pdata[i]);
			}
		}
	}

	if (found >= 0) {
		/* vconf update */
		i = vconf_set_str(VCONFKEY_NFC_OTHER_HANDLERS, built);
		if (i < 0) {
			DEBUG_ERR_MSG("vconf_set_str(VCONFKEY_NFC_OTHERS_HANDLER) failed, [%d]", i);
		}
	}
}

static void __append_other_handler(const char *package)
{
	int result;
	char *others;
	char built[4096] = { 0, };

	if (__is_other_handler(package) == true) {
		return;
	}

	others = vconf_get_str(VCONFKEY_NFC_OTHER_HANDLERS);
	if (others != NULL) {
		if (strlen(others) > 0) {
			snprintf(built, sizeof(built), "%s|%s", others, package);
		} else {
			snprintf(built, sizeof(built), "%s", package);
		}

		g_free(others);
	} else {
		snprintf(built, sizeof(built), "%s", package);
	}

	/* vconf update */
	result = vconf_set_str(VCONFKEY_NFC_OTHER_HANDLERS, built);
	if (result < 0) {
		DEBUG_ERR_MSG("vconf_set_str(VCONFKEY_NFC_OTHERS_HANDLER) failed, [%d]", result);
	}
}

static bool __is_preferred_handler(net_nfc_card_emulation_category_t category,
	const char *package)
{
	switch (category) {
	case NET_NFC_CARD_EMULATION_CATEGORY_PAYMENT :
		return __is_preferred_payment_handler(package);
	case NET_NFC_CARD_EMULATION_CATEGORY_OTHER :
		return __is_preferred_other_handler(package);
	default :
		return false;
	}
}

static bool __is_activated_handler(net_nfc_card_emulation_category_t category,
	const char *package)
{
	/* set nfc-manager to default handler */
	if (g_strcmp0(package, "nfc-manager") == 0) {
		return true;
	}

	switch (category) {
	case NET_NFC_CARD_EMULATION_CATEGORY_PAYMENT :
		return __is_payment_handler(package);
	case NET_NFC_CARD_EMULATION_CATEGORY_OTHER :
		return __is_other_handler(package);
	default :
		return false;
	}
}

static void __activate_handler(const char *package,
	net_nfc_card_emulation_category_t category)
{
	switch (category) {
	case NET_NFC_CARD_EMULATION_CATEGORY_PAYMENT :
		if (__is_payment_handler(package) == false) {
			__set_payment_handler(package);
		}
		break;

	case NET_NFC_CARD_EMULATION_CATEGORY_OTHER :
		if (__is_other_handler(package) == false) {
			__append_other_handler(package);
		}
		break;

	default :
		break;
	}
}

static void __deactivate_handler(const char *package,
	net_nfc_card_emulation_category_t category)
{
	switch (category) {
	case NET_NFC_CARD_EMULATION_CATEGORY_PAYMENT :
		if (__is_payment_handler(package) == true) {
			__set_payment_handler(NULL);
		}
		break;

	case NET_NFC_CARD_EMULATION_CATEGORY_OTHER :
		if (__is_other_handler(package) == true) {
			__remove_other_handler(package);
		}
		break;

	default :
		break;
	}
}

void net_nfc_server_route_table_update_category_handler(const char *package,
	net_nfc_card_emulation_category_t category)
{
	switch (category) {
	case NET_NFC_CARD_EMULATION_CATEGORY_PAYMENT :
		__update_payment_handler(package);
		net_nfc_server_route_table_do_update(net_nfc_server_manager_get_active());
		break;

	case NET_NFC_CARD_EMULATION_CATEGORY_OTHER :
		__update_other_handler(package);
		net_nfc_server_route_table_do_update(net_nfc_server_manager_get_active());
		break;

	default :
		break;
	}
}


////////////////////////////////////////////////////////////////////////////////

/*Routing Table base on AID*/
route_table_handler_t *preferred_handler;
static GHashTable *routing_handlers;

static bool __get_package_name(const char *id, char *package, size_t len)
{
	pid_t pid;
	bool result;

	pid = net_nfc_server_gdbus_get_pid(id);
	if (pid > 0) {
		if (net_nfc_util_get_pkgid_by_pid(pid,
			package, len) == true) {
			result = true;
		} else {
			result = false;
		}
	} else {
		result = false;
	}

	return result;
}

static void __on_key_destroy(gpointer data)
{
	if (data != NULL) {
		g_free(data);
	}
}

static void __on_value_destroy(gpointer data)
{
	route_table_handler_t *listener = (route_table_handler_t *)data;

	if (data != NULL) {
		int i;

		for (i = NET_NFC_CARD_EMULATION_CATEGORY_PAYMENT;
			i < NET_NFC_CARD_EMULATION_CATEGORY_MAX;
			i++) {
			if (listener->aids[i] != NULL) {
				g_ptr_array_free(listener->aids[i], true);
			}
		}

		if (listener->id != NULL) {
			g_free(listener->id);
		}

		g_free(data);
	}
}

static void __on_aid_info_destroy(gpointer data)
{
	aid_info_t *info = (aid_info_t *)data;

	if (info->aid != NULL) {
		g_free(info->aid);
	}

	g_free(info);
}

static void __on_iterate_db_aid_cb(const char *package,
	net_nfc_se_type_e se_type, net_nfc_card_emulation_category_t category,
	const char *aid, bool unlock, int power, bool manifest, void *user_data)
{
	net_nfc_server_route_table_add_handler(NULL, package);

	SECURE_MSG("package [%s], se_type [%d], category [%d], manifest [%d], aid [%s]", package, se_type, category, manifest, aid);

	__route_table_add_aid(NULL, package, se_type, category, aid, unlock, power, manifest, false);
}

void net_nfc_server_route_table_init()
{
	if (routing_handlers == NULL) {
		char *package;

		__initialize_db();

		package = vconf_get_str(VCONFKEY_NFC_PAYMENT_HANDLERS);
		__update_payment_handler(package);
		if (package != NULL) {
			g_free(package);
		}

		package = vconf_get_str(VCONFKEY_NFC_OTHER_HANDLERS);
		__update_other_handler(package);
		if (package != NULL) {
			g_free(package);
		}

		routing_handlers = g_hash_table_new_full(g_str_hash,
			g_str_equal, __on_key_destroy, __on_value_destroy);

		/* load aids from database */
		net_nfc_server_route_table_load_db();

		/* update route table without routing */
		net_nfc_server_route_table_do_update(false);
	}
}

void net_nfc_server_route_table_load_db()
{
	if (routing_handlers != NULL) {
		__iterate_db(__on_iterate_db_aid_cb, NULL);
	}
}

void net_nfc_server_route_table_deinit()
{
	if (routing_handlers != NULL) {
		g_hash_table_destroy(routing_handlers);
		routing_handlers = NULL;

		__finalize_db();
	}

}

route_table_handler_t *net_nfc_server_route_table_find_handler(
	const char *package)
{
	return (route_table_handler_t *)g_hash_table_lookup(routing_handlers,
		(gconstpointer)package);
}

net_nfc_error_e net_nfc_server_route_table_add_handler(const char *id,
	const char *package)
{
	route_table_handler_t *data;
	net_nfc_error_e result;

	data = net_nfc_server_route_table_find_handler(package);
	if (data == NULL) {
		int i;

		SECURE_MSG("new package, [%s]", package);

		data = g_new0(route_table_handler_t, 1);

		data->package = g_strdup(package);
		if (id != NULL) {
			data->id = g_strdup(id);
		}

		/* full aid list in array index 0 */
		data->aids[NET_NFC_CARD_EMULATION_CATEGORY_UNKNOWN] =
			g_ptr_array_new_full(0, __on_aid_info_destroy);

		/* partial aid list for each category in array index > 1 */
		for (i = NET_NFC_CARD_EMULATION_CATEGORY_PAYMENT;
			i < NET_NFC_CARD_EMULATION_CATEGORY_MAX;
			i++) {
			data->aids[i] = g_ptr_array_new();
		}

		g_hash_table_insert(routing_handlers, (gpointer)g_strdup(package),
			(gpointer)data);

		result = NET_NFC_OK;
	} else {
		if (id != NULL && data->id == NULL) {
			SECURE_MSG("update client id, [%s]", id);
			data->id = g_strdup(id);
		}

		result = NET_NFC_OK;
	}

	return result;
}

static net_nfc_error_e __do_routing_aid(aid_info_t *info, bool commit)
{
	net_nfc_error_e result;

	if (info->is_routed == false) {
		data_s temp = { 0, };

		SECURE_MSG("route aid, aid [%s]", info->aid);

		if (net_nfc_util_aid_is_prefix(info->aid) == true) {
			SECURE_MSG("prefix...");
		}

		if (net_nfc_util_hex_string_to_binary(info->aid, &temp) == true) {
			net_nfc_controller_secure_element_route_aid(&temp, info->se_type, info->power, &result);
			if (result == NET_NFC_OK) {
				info->is_routed = true;
				if (info->power == 0x21 || info->power == 0x31) {
					count++;
					net_nfc_server_vconf_set_screen_on_flag(true);
				}

				if (commit == true) {
					net_nfc_controller_secure_element_commit_routing(&result);
				}
			} else {
				DEBUG_ERR_MSG("net_nfc_controller_secure_element_route_aid failed, [%d]", result);
			}

			net_nfc_util_clear_data(&temp);
		} else {
			DEBUG_ERR_MSG("net_nfc_util_hex_string_to_binary failed");

			result = NET_NFC_ALLOC_FAIL;
		}
	} else {
		SECURE_MSG("already routed, aid [%s]", info->aid);

		result = NET_NFC_ALREADY_REGISTERED;
	}

	return result;
}

static net_nfc_error_e __do_unrouting_aid(aid_info_t *info, bool commit)
{
	net_nfc_error_e result;

	if (info->is_routed == true) {
		data_s temp = { 0, };

		SECURE_MSG("unroute aid, aid [%s]", info->aid);

		if (net_nfc_util_aid_is_prefix(info->aid) == true) {
			SECURE_MSG("prefix...");
		}

		if (net_nfc_util_hex_string_to_binary(info->aid, &temp) == true) {
			net_nfc_controller_secure_element_unroute_aid(&temp, &result);
			if (result == NET_NFC_OK) {
				info->is_routed = false;
				if (info->power == 0x21 || info->power == 0x31) {
					count--;
					if (count <= 0) {
						net_nfc_server_vconf_set_screen_on_flag(false);
						net_nfc_controller_set_screen_state(NET_NFC_SCREEN_OFF , &result);
					}
				}

				if (commit == true) {
					net_nfc_controller_secure_element_commit_routing(&result);
				}
			} else {
				DEBUG_ERR_MSG("net_nfc_controller_secure_element_unroute_aid failed, [%d]", result);
			}

			net_nfc_util_clear_data(&temp);
		} else {
			DEBUG_ERR_MSG("net_nfc_util_hex_string_to_binary failed");

			result = NET_NFC_ALLOC_FAIL;
		}
	} else {
		SECURE_MSG("didn't routed, aid [%s]", info->aid);

		result = NET_NFC_NOT_REGISTERED;
	}

	return result;
}

#if 0
net_nfc_error_e net_nfc_server_route_table_del_handler(const char *id,
	const char *package, bool force)
{
	route_table_handler_t *data;
	net_nfc_error_e result;
	bool need_commit = false;

	data = net_nfc_server_route_table_find_handler(package);
	if (data != NULL) {
		int i;
		aid_info_t *info;

		DEBUG_SERVER_MSG("deleting package, [%s]", package);

		for (i = NET_NFC_CARD_EMULATION_CATEGORY_PAYMENT;
			i < NET_NFC_CARD_EMULATION_CATEGORY_MAX;
			i++) {
			/* deactivate for each category */
			if (data->activated[i] == true) {
				/* TODO */
			}
		}

		for (i = (int)data->aids[0]->len - 1; i >= 0; i--) {
			info = data->aids[0]->pdata[i];

			if (force == true || info->manifest == false) {
				if (__do_unrouting_aid(info, false) == NET_NFC_OK) {
					need_commit = true;
				}

				g_ptr_array_remove(data->aids[info->category], info);
				g_ptr_array_remove_index(data->aids[0], i);
			} else {
				DEBUG_SERVER_MSG("manifest aid, skip deleting, [%s]", info->aid);
			}
		}

		if (need_commit == true) {
			net_nfc_controller_secure_element_commit_routing(&result);
		}

		if (data->aids[0]->len == 0) {
			g_hash_table_remove(routing_handlers, package);
		} else {
			DEBUG_SERVER_MSG("remain some aids, [%d]", data->aids[0]->len);
		}

		result = NET_NFC_OK;
	} else {
		DEBUG_ERR_MSG("package not found");

		result = NET_NFC_OPERATION_FAIL;
	}

	return result;
}
#else
net_nfc_error_e net_nfc_server_route_table_del_handler(const char *id,
	const char *package, bool force)
{
	route_table_handler_t *data;
	net_nfc_error_e result;

	data = net_nfc_server_route_table_find_handler(package);
	if (data != NULL) {
		net_nfc_card_emulation_category_t category;

		if (data->id != NULL) {
			g_free(data->id);
			data->id = NULL;
		}

		if (data->aids[0]->len > 0) {
			net_nfc_server_route_table_del_aids(id, package, force);
		}

		for (category = NET_NFC_CARD_EMULATION_CATEGORY_PAYMENT;
			category < NET_NFC_CARD_EMULATION_CATEGORY_MAX;
			category++) {
			if (data->aids[category]->len == 0 &&
				data->activated[category] == true) {
			/* deactivate for each category */
				SECURE_MSG("deactivate handler, [%d][%s]", category, package);

				/* TODO */
				__deactivate_handler(package, category);
			}
		}

		if (data->aids[0]->len == 0) {
			SECURE_MSG("deleting package, [%s]", package);

			g_hash_table_remove(routing_handlers, package);

			result = NET_NFC_OK;
		} else {
			SECURE_MSG("remain some aids, [%d]", data->aids[0]->len);

			result = NET_NFC_OPERATION_FAIL;
		}
	} else {
		DEBUG_ERR_MSG("package not found");

		result = NET_NFC_OPERATION_FAIL;
	}

	return result;
}
#endif
net_nfc_error_e net_nfc_server_route_table_update_handler_id(
	const char *package, const char *id)
{
	net_nfc_error_e result;

	if (id != NULL && strlen(id) > 0) {
		result = net_nfc_server_route_table_add_handler(id, package);
	} else {
		route_table_handler_t *data;

		data = net_nfc_server_route_table_find_handler(package);
		if (data != NULL) {
			if (data->id != NULL) {
				SECURE_MSG("remove client id, [%s]", id);
				g_free(data->id);
				data->id = NULL;
			}

		}

		result = NET_NFC_OK;
	}

	return result;
}

route_table_handler_t *net_nfc_server_route_table_get_preferred_handler()
{
	return preferred_handler;
}

void net_nfc_server_route_table_unset_preferred_handler_by_id(const char* id)
{
	route_table_handler_t *handler;

	handler = net_nfc_server_route_table_find_handler_by_id(id);
	if (handler != NULL) {
		if (__is_preferred_handler(NET_NFC_CARD_EMULATION_CATEGORY_PAYMENT, handler->package) ||
			 __is_preferred_handler(NET_NFC_CARD_EMULATION_CATEGORY_OTHER, handler->package)) {
			DEBUG_SERVER_MSG("[Preferred] Unset Preferred handler by id");
			net_nfc_server_route_table_set_preferred_handler(NULL);
			net_nfc_server_route_table_do_update(net_nfc_server_manager_get_active());
		}
	}
}

void net_nfc_server_route_table_set_preferred_handler(route_table_handler_t *handler)
{
	int payment_count, other_count;

	preferred_handler = handler;

	if (handler != NULL) {
		payment_count = handler->aids[NET_NFC_CARD_EMULATION_CATEGORY_PAYMENT]->len;
		if (payment_count > 0) {
			preferred_payment = handler->package;
			DEBUG_SERVER_MSG("[Preferred] Set Preferred payment handler");
		}

		other_count = handler->aids[NET_NFC_CARD_EMULATION_CATEGORY_OTHER]->len;
		if (other_count > 0) {
			preferred_other = handler->package;
			DEBUG_SERVER_MSG("[Preferred] Set Preferred other handler");
		}
	} else {
		preferred_payment = NULL;
		preferred_other = NULL;
	}
}

net_nfc_error_e net_nfc_server_route_table_update_preferred_handler()
{
	char foreground[1024] = {0,};

	if (preferred_handler != NULL && preferred_handler->package) {
		pid_t pid;

		//check match prefered and foreground app
		pid = net_nfc_app_util_get_focus_app_pid();
		net_nfc_util_get_pkgid_by_pid(pid, foreground, sizeof(foreground));

		if (strcmp(foreground, preferred_handler->package) != 0) {
			DEBUG_SERVER_MSG("[Preferred] Not match!!! foreground : %s, preferred : %s",
				foreground, preferred_handler->package);

			net_nfc_server_route_table_set_preferred_handler(NULL);
			net_nfc_server_route_table_do_update(net_nfc_server_manager_get_active());
		}
	}

#ifdef PRINT_TABLE
	net_nfc_server_route_table_preferred_handler_dump();
#endif
	return NET_NFC_OK;
}

bool net_nfc_server_route_table_is_allowed_preferred_handler(route_table_handler_t *handler)
{
	int payment_count;

	//platform implentation
	//check if package has payment aid and check if handler is activated handler.
	payment_count = handler->aids[NET_NFC_CARD_EMULATION_CATEGORY_PAYMENT]->len;
	if (payment_count != 0) {
		if (__is_activated_handler(NET_NFC_CARD_EMULATION_CATEGORY_PAYMENT, handler->package) == true) {
			DEBUG_SERVER_MSG("[Preferred] %s is already payment activated handler", handler->package);
			return false;
		}
	}

	return true;
}

#ifdef PRINT_TABLE
void net_nfc_server_route_table_preferred_handler_dump()
{
	char foreground[1024] = {0,};
	pid_t pid;

	//check match prefered and foreground app
	pid = net_nfc_app_util_get_focus_app_pid();

	if (pid <= 0) {
		DEBUG_ERR_MSG("focus app is not exist. pid is %d", (int)pid);
		return;
	}

	net_nfc_util_get_pkgid_by_pid(pid, foreground, sizeof(foreground));

	DEBUG_SERVER_MSG("------------------------------");
	DEBUG_SERVER_MSG("Foreground package : %s", foreground);
	if (preferred_handler != NULL) {
		DEBUG_SERVER_MSG("Preferred default : %s", preferred_handler->package);
		DEBUG_SERVER_MSG("PAYMENT activate state : %d", preferred_handler->activated[1]);
		DEBUG_SERVER_MSG("OTHER activate state : %d", preferred_handler->activated[2]);
	}
	DEBUG_SERVER_MSG("------------------------------");
}
#endif

void net_nfc_server_route_table_iterate_handler(
	net_nfc_server_route_table_handler_iter_cb cb, void *user_data)
{
	GHashTableIter iter;
	gpointer key;
	route_table_handler_t *data;

	if (routing_handlers == NULL)
		return;

	g_hash_table_iter_init(&iter, routing_handlers);

	while (g_hash_table_iter_next(&iter, &key, (gpointer)&data)) {
		if (cb((const char *)key, data, user_data) == false) {
			break;
		}
	}
}

void net_nfc_server_route_table_iterate_handler_activated_last(
	net_nfc_server_route_table_handler_iter_cb cb, void *user_data)
{
	GHashTableIter iter;
	gpointer key;
	route_table_handler_t *data;

	if (routing_handlers == NULL)
		return;

	g_hash_table_iter_init(&iter, routing_handlers);

	while (g_hash_table_iter_next(&iter, &key, (gpointer)&data)) {
		if (!__is_preferred_payment_handler((const char *)key)
			&& !__is_preferred_other_handler((const char *)key)
			&& !__is_payment_handler((const char *)key)
			&& !__is_other_handler((const char *)key)) {
			if (cb((const char *)key, data, user_data) == false) {
				break;
			}
		}
	}

	g_hash_table_iter_init(&iter, routing_handlers);

	while (g_hash_table_iter_next(&iter, &key, (gpointer)&data)) {
		if (__is_preferred_payment_handler((const char *)key) == false
			&& __is_preferred_other_handler((const char *)key) == false
			&& (__is_payment_handler((const char *)key) || __is_other_handler((const char *)key))) {
			if (cb((const char *)key, data, user_data) == false) {
				break;
			}
		}
	}

	if (preferred_handler != NULL) {
		cb((const char *)preferred_handler->package, preferred_handler, user_data);
	}
}
#ifdef PRINT_TABLE
static const char *__get_se_name(net_nfc_se_type_e se_type)
{
	switch (se_type) {
	case NET_NFC_SE_TYPE_ESE :
		return "eSE";
	case NET_NFC_SE_TYPE_UICC :
		return "UICC";
	case NET_NFC_SE_TYPE_SDCARD :
		return "SD";
	case NET_NFC_SE_TYPE_HCE :
		return "HCE";
	default :
		return "Unknown";
	}
}

static bool _display_route_table_cb(const char *package,
	route_table_handler_t *handler, void *user_data)
{
	int i;
	if (preferred_handler == handler) {
		SECURE_LOGD(" + Preferred PACKAGE [%s|%s]", handler->package, handler->id);
	} else {
		SECURE_LOGD(" + PACKAGE [%s|%s]", handler->package, handler->id);
	}

	for (i = 0; i < handler->aids[0]->len; i++) {
		aid_info_t *info = (aid_info_t *)handler->aids[0]->pdata[i];

		SECURE_LOGD(" +-- AID [%s], SE [%s], CATEGORY [%s%s], MANIFEST [%s]%s",
			info->aid,
			__get_se_name(info->se_type),
			info->category == NET_NFC_CARD_EMULATION_CATEGORY_PAYMENT ? "payment" : "other",
			handler->activated[info->category] ? " (A)" : "",
			info->manifest ? "manifest" : "dynamic",
			info->is_routed ? ", (ROUTED)" : "");
	}

	return true;
}

static void __display_route_table()
{
	SECURE_LOGD(" +------------------------------------------------+");
	net_nfc_server_route_table_iterate_handler(_display_route_table_cb, NULL);
	SECURE_LOGD(" +------------------------------------------------+");
}
#endif
static bool __activation_iter_cb(const char *package,
	route_table_handler_t *handler, void *user_data)
{
	int i;
	bool ret = true;
	aid_info_t *info;
	gpointer *params = (gpointer *)user_data;
	net_nfc_se_type_e se_type;
	net_nfc_card_emulation_category_t category;

	category = (net_nfc_card_emulation_category_t)params[1];
	se_type = (net_nfc_se_type_e)params[2];

	if (params[0] != NULL &&
		g_ascii_strcasecmp(package, (const char *)params[0]) == 0) {
		handler->activated[category] = true;

		(*(int *)params[3])++;

		for (i = 0; i < handler->aids[category]->len; i++) {
			info = (aid_info_t *)handler->aids[category]->pdata[i];

			if (info->se_type == se_type) {
				if (info->is_routed == true) {
					__do_unrouting_aid(info, false);
				}
				continue;
			}

			if (info->is_routed == false) {
				__do_routing_aid(info, false);
			}

			/* FIXME : need commit?? check and return */
		}

		/* in others category, it is changed just exact handler state */
		/* FIXME : when 'others setting' is applied, conflict case should be handled */
		/* stop iterating */
		if (category == NET_NFC_CARD_EMULATION_CATEGORY_OTHER) {
			ret = false;
		}
	} else {
		if (category == NET_NFC_CARD_EMULATION_CATEGORY_PAYMENT &&
			handler->activated[category] == true) {
			/* deactivate another handlers in payment category */

			handler->activated[category] = false;

			for (i = 0; i < handler->aids[category]->len; i++) {
				info = (aid_info_t *)handler->aids[category]->pdata[i];

				if (info->is_routed == true) {
					__do_unrouting_aid(info, false);
				}

				/* FIXME : need commit?? check and return */
			}
		}
	}

	return ret;
}

net_nfc_error_e net_nfc_server_route_table_set_handler_activation(
	const char *package, net_nfc_card_emulation_category_t category)
{
	net_nfc_error_e result;
	gpointer params[4];
	int ret = 0;

	params[0] = (gpointer)package;
	params[1] = (gpointer)category;
	params[2] = (gpointer)net_nfc_server_se_get_se_type();
	params[3] = (gpointer)&ret;

	net_nfc_server_route_table_iterate_handler(__activation_iter_cb,
		(void *)params);

#ifdef PRINT_TABLE
	__display_route_table();
#endif

	if (ret == 1) {
		SECURE_MSG("activated category [%d], package [%s]", category, package);

		net_nfc_controller_secure_element_commit_routing(&result);

		result = NET_NFC_OK;
	} else if (ret == 0) {
		SECURE_MSG("package not found : [%s]", package);

		result = NET_NFC_NO_DATA_FOUND;
	} else {
		SECURE_MSG("wrong result : [%s][%d]", package, ret);

		result = NET_NFC_OPERATION_FAIL;
	}

	return result;
}


route_table_handler_t *net_nfc_server_route_table_find_handler_by_id(
	const char *id)
{
	route_table_handler_t *result;
	char package[1024];

	if (__get_package_name(id, package, sizeof(package)) == true) {
		DEBUG_SERVER_MSG("caller package_name is %s", package);
		result = net_nfc_server_route_table_find_handler(package);
	} else {
		result = NULL;
	}

	return result;
}

net_nfc_error_e net_nfc_server_route_table_add_handler_by_id(const char *id)
{
	net_nfc_error_e result;
	char package[1024];

	if (__get_package_name(id, package, sizeof(package)) == true) {
		result = net_nfc_server_route_table_add_handler(id,
			package);
	} else {
		result = NET_NFC_INVALID_PARAM;
	}

	return result;
}

net_nfc_error_e net_nfc_server_route_table_del_handler_by_id(const char *id)
{
	net_nfc_error_e result;
	char package[1024];

	if (__get_package_name(id, package, sizeof(package)) == true) {
		result = net_nfc_server_route_table_del_handler(id, package, false);
	} else {
		result = NET_NFC_INVALID_PARAM;
	}

	return result;
}


net_nfc_error_e net_nfc_server_route_table_set_handler_activation_by_id(
	const char *id, net_nfc_card_emulation_category_t category)
{
	net_nfc_error_e result;
	char package[1024];

	if (__get_package_name(id, package, sizeof(package)) == true) {
		result = net_nfc_server_route_table_set_handler_activation(
			package, category);
	} else {
		result = NET_NFC_INVALID_PARAM;
	}

	return result;
}



aid_info_t *net_nfc_server_route_table_find_aid(const char *package,
	const char *aid)
{
	route_table_handler_t *handler;

	handler = net_nfc_server_route_table_find_handler(package);
	if (handler != NULL) {
		aid_info_t *info;
		int i;

		for (i = 0; i < handler->aids[0]->len; i++) {
			info = handler->aids[0]->pdata[i];

			if (g_ascii_strcasecmp(aid, info->aid) == 0) {
				return info;
			}
		}
	}

	return NULL;
}

static bool __find_handler_iter_cb(const char *package,
	route_table_handler_t *handler, void *user_data)
{
	bool result = true;
	gpointer *params = (gpointer *)user_data;
	aid_info_t *aid;

	aid = net_nfc_server_route_table_find_aid(package,
		(const char *)params[0]);
	if (aid != NULL) {
		if (handler->activated[aid->category] == true) {
			params[1] = handler;
			result = false;
		} else {
			SECURE_MSG("not activated payment aid, [%s]", aid->aid);
		}
	}

	return result;
}

route_table_handler_t *net_nfc_server_route_table_find_handler_by_aid(
	const char *aid)
{
	gpointer params[2];

	params[0] = g_strdup(aid);
	params[1] = NULL;

	net_nfc_server_route_table_iterate_handler(__find_handler_iter_cb, params);

	g_free(params[0]);

	return (route_table_handler_t *)params[1];
}

static bool __matched_aid_cb(const char *package,
	route_table_handler_t *handler, void *user_data)
{
	bool result = true;
	gpointer *params = (gpointer *)user_data;

	if (net_nfc_server_route_table_find_aid(package,
		(const char *)params[0]) != NULL) {
		params[1] = handler;
		result = false;
	}

	return result;
}

aid_info_t *net_nfc_server_route_table_find_first_matched_aid(const char *aid)
{
	__matched_aid_cb(NULL, NULL, NULL);

	return NULL;
}

static bool __check_conflict_aid_cb(
	const char *package, route_table_handler_t *handler,
	aid_info_t *aid, void *user_data)
{
	bool result;
	gpointer *params = (gpointer *)user_data;

	if (g_strcmp0(package, (const char *)params[0]) == 0) {
		/* skip same package */
		return true;
	}

	if (handler->activated[aid->category] == true &&
		g_strcmp0(aid->aid, (const char *)params[1]) == 0) {
		net_nfc_error_e *res = params[3];

		SECURE_MSG("conflict!!! [%s|%s] vs [%s|%s]", handler->package, aid->aid, (const char *)params[0], (const char *)params[1]);

		*res = NET_NFC_DATA_CONFLICTED;
		result = false;
	} else {
		result = true;
	}

	return result;
}

static net_nfc_error_e __route_table_add_aid(const char *id,
	const char *package, net_nfc_se_type_e se_type,
	net_nfc_card_emulation_category_t category,
	const char *aid, bool unlock, int power, bool manifest, bool routing)
{
	net_nfc_error_e result = NET_NFC_OK;
	route_table_handler_t *data;

	/* check conflict */
	if (category == NET_NFC_CARD_EMULATION_CATEGORY_OTHER) {
		gpointer params[4];

		params[0] = (gpointer)package;
		params[1] = (gpointer)aid;
		params[2] = (gpointer)category;
		params[3] = &result;

		net_nfc_server_route_table_iterate_aids(__check_conflict_aid_cb, params);
	}

	data = net_nfc_server_route_table_find_handler(package);
	if (data != NULL) {
		if (net_nfc_server_route_table_find_aid(package, aid) == NULL) {
			aid_info_t *info;

			SECURE_MSG("new aid, package [%s], se_type [%d], category [%d], aid [%s], ", package, se_type, category, aid);

			info = g_new0(aid_info_t, 1);

			info->aid = g_strdup(aid);
			info->se_type = se_type;
			info->category = category;
			info->unlock = unlock;
			info->power = 0x39;/*Temp code for Screen off Transaction*/
			info->manifest = manifest;

			g_ptr_array_add(data->aids[0], info);
			g_ptr_array_add(data->aids[category], info);

			if (result == NET_NFC_OK) {
				if (routing == true) {
					if (se_type != net_nfc_server_se_get_se_type()) {
						if (data->activated[category] == true) {
							SECURE_MSG("routing... package [%s], aid [%s], ", package, aid);

							result = __do_routing_aid(info, true);
						} else {
							SECURE_MSG("not activated handler, aid [%s]", aid);

							result = NET_NFC_OK;
						}
					} else {
						SECURE_MSG("route to default SE... skip, aid [%s]", aid);

						result = NET_NFC_OK;
					}
				}

				if (data->aids[0]->len > AID_MAX) {
					SECURE_MSG("no space. cannot add aid, [%d]", result);

					result = NET_NFC_INSUFFICIENT_STORAGE;
				}
			}
		} else {
			SECURE_MSG("already exist, aid [%s]", aid);

			result = NET_NFC_ALREADY_REGISTERED;
		}
	} else {
		DEBUG_ERR_MSG("package not found");

		result = NET_NFC_OPERATION_FAIL;
	}

	return result;
}

static net_nfc_error_e __route_table_del_aid(const char *id,
	const char *package, const char *aid, bool force)
{
	net_nfc_error_e result = NET_NFC_NO_DATA_FOUND;
	route_table_handler_t *data;

	data = net_nfc_server_route_table_find_handler(package);
	if (data != NULL &&
		(id == NULL || data->id == NULL ||
			g_ascii_strcasecmp(id, data->id) == 0)) {
		int i;

		for (i = 0; i < data->aids[0]->len; i++) {
			aid_info_t *info = (aid_info_t *)data->aids[0]->pdata[i];

			if (g_ascii_strcasecmp(info->aid, aid) == 0) {
				if (force == true || info->manifest == false) {
					SECURE_MSG("remove aid, package [%s], aid [%s]", package, aid);

					if (info->is_routed == true) {
						__do_unrouting_aid(info, true);
					}

					g_ptr_array_remove(data->aids[info->category], info);
					g_ptr_array_remove_index(data->aids[0], i);

					result = NET_NFC_OK;
				} else {
					SECURE_MSG("cannot remove aid because it stored in manifest, aid [%s]", info->aid);

					result = NET_NFC_OPERATION_FAIL;
				}

				break;
			}
		}
	}

	return result;
}

net_nfc_error_e net_nfc_server_route_table_add_aid(const char *id,
	const char *package, net_nfc_se_type_e se_type,
	net_nfc_card_emulation_category_t category,
	const char *aid)
{
	net_nfc_error_e result;

	result = net_nfc_server_route_table_add_handler(id, package);
	if (result == NET_NFC_OK) {
		result = __route_table_add_aid(id,
			package, se_type, category, aid, true, 1, false, true);
		if (result == NET_NFC_OK) {
			result = __insert_into_db(package, se_type, category, aid, true, 1, false);
			if (result != NET_NFC_OK) {
				DEBUG_ERR_MSG("__insert_into_db failed, [%d]", result);

				result = __route_table_del_aid(id, package, aid, false);
				if (result != NET_NFC_OK) {
					DEBUG_ERR_MSG("__route_table_del_aid failed, [%d]", result);
				}
			}
		} else if (result == NET_NFC_INSUFFICIENT_STORAGE) {
			DEBUG_ERR_MSG("no space. cannot add aid, [%d]", result);

			__route_table_del_aid(id, package, aid, false);
		} else {
			DEBUG_ERR_MSG("net_nfc_server_route_table_add_aid failed, [%d]", result);
		}
	} else {
		DEBUG_ERR_MSG("net_nfc_server_route_table_add_handler failed, [%d]", result);
	}

#ifdef PRINT_TABLE
	__display_route_table();
#endif
	return result;
}

net_nfc_error_e net_nfc_server_route_table_del_aid(const char *id,
	const char *package, const char *aid, bool force)
{
	net_nfc_error_e result = NET_NFC_OK;

	result = __route_table_del_aid(id, package, aid, false);
	if (result == NET_NFC_OK) {
	} else {
		DEBUG_ERR_MSG("__route_table_del_aid failed, [%d]", result);
	}

	result = __delete_from_db(package, aid);
	if (result != NET_NFC_OK) {
		DEBUG_ERR_MSG("__delete_from_db failed, [%d]", result);
	}

#ifdef PRINT_TABLE
	__display_route_table();
#endif
	return result;
}

net_nfc_error_e net_nfc_server_route_table_del_aids(const char *id,
	const char *package, bool force)
{
	net_nfc_error_e result = NET_NFC_NO_DATA_FOUND;
	route_table_handler_t *data;
	bool need_commit = false;

	data = net_nfc_server_route_table_find_handler(package);
	if (data != NULL &&
		(id == NULL || data->id == NULL ||
			g_ascii_strcasecmp(id, data->id) == 0)) {
		int i;

		for (i = (int)data->aids[0]->len - 1; i >= 0; i--) {
			aid_info_t *info = (aid_info_t *)data->aids[0]->pdata[i];

			if (force == true || info->manifest == false) {
				SECURE_MSG("remove aid, package [%s], aid [%s]", package, info->aid);

				if (info->is_routed == true &&
					__do_unrouting_aid(info, false) == NET_NFC_OK) {
					need_commit = true;
				}

				result = __delete_from_db(package, info->aid);
				if (result != NET_NFC_OK) {
					DEBUG_ERR_MSG("__delete_from_db failed, [%d]", result);
				}

				g_ptr_array_remove(data->aids[info->category], info);
				g_ptr_array_remove_index(data->aids[0], i);
			} else {
				SECURE_MSG("cannot remove aid because it stored in manifest, aid [%s]", info->aid);
			}
		}
	} else {
		SECURE_MSG("not found, package [%s]", package);
	}

	if (need_commit == true) {
		net_nfc_controller_secure_element_commit_routing(&result);
	}

#ifdef PRINT_TABLE
	__display_route_table();
#endif
	return result;
}

void net_nfc_server_route_table_iterate_aids(
	net_nfc_server_route_table_aid_iter_cb cb, void *user_data)
{
	GHashTableIter iter;
	gpointer key;
	route_table_handler_t *data;
	int i;
	aid_info_t *info;

	if (routing_handlers == NULL)
		return;

	g_hash_table_iter_init(&iter, routing_handlers);

	while (g_hash_table_iter_next(&iter, &key, (gpointer)&data)) {
		for (i = 0; i < data->aids[0]->len; i++) {
			info = (aid_info_t *)data->aids[0]->pdata[i];

			if (cb((const char *)key, data, info, user_data) == false) {
				break;
			}
		}
	}
}

void net_nfc_server_route_table_iterate_handler_aids(const char *package,
	net_nfc_server_route_table_aid_iter_cb cb, void *user_data)
{
	route_table_handler_t *data;
	int i;
	aid_info_t *info;

	if (routing_handlers == NULL)
		return;

	data = (route_table_handler_t *)g_hash_table_lookup(routing_handlers,
		package);
	if (data != NULL) {
		for (i = 0; i < data->aids[0]->len; i++) {
			info = (aid_info_t *)data->aids[0]->pdata[i];

			if (cb(package, data, info, user_data) == false) {
				break;
			}
		}
	}
}

aid_info_t *net_nfc_server_route_table_find_aid_by_id(const char *id,
	const char *aid)
{
	aid_info_t *result;
	char package[1024];

	if (__get_package_name(id, package, sizeof(package)) == true) {
		result = net_nfc_server_route_table_find_aid(package, aid);
	} else {
		result = NULL;
	}

	return result;

}

net_nfc_error_e net_nfc_server_route_table_add_aid_by_id(const char *id,
	net_nfc_se_type_e se_type,
	net_nfc_card_emulation_category_t category,
	const char *aid)
{
	net_nfc_error_e result;
	char package[1024];

	if (__get_package_name(id, package, sizeof(package)) == true) {
		result = net_nfc_server_route_table_add_aid(id,
			package, se_type, category, aid);
	} else {
		result = NET_NFC_INVALID_PARAM;
	}

	return result;
}

net_nfc_error_e net_nfc_server_route_table_del_aid_by_id(const char *id,
	const char *aid, bool force)
{
	net_nfc_error_e result = NET_NFC_NO_DATA_FOUND;
	char package[1024];

	if (__get_package_name(id, package, sizeof(package)) == true) {
		result = net_nfc_server_route_table_del_aid(id,
			package, aid, force);
	}

	return result;
}


void net_nfc_server_route_table_iterate_aids_by_id(const char *id,
	net_nfc_server_route_table_aid_iter_cb cb, void *user_data)
{
	char package[1024];

	if (__get_package_name(id, package, sizeof(package)) == true) {
		net_nfc_server_route_table_iterate_handler_aids(package, cb, user_data);
	}
}

net_nfc_error_e net_nfc_server_route_table_insert_aid_into_db(
	const char *package, net_nfc_se_type_e se_type,
	net_nfc_card_emulation_category_t category,
	const char *aid, bool unlock, int power)
{
	net_nfc_error_e result;

	result = __insert_into_db(package, se_type, category,
		aid, unlock, power, true);
	if (result == NET_NFC_OK) {
		result = net_nfc_server_route_table_add_handler(NULL, package);
		if (result == NET_NFC_OK) {
			result = __route_table_add_aid(NULL,
				package, se_type, category, aid,
				unlock, power, true, net_nfc_server_manager_get_active());
			if (result == NET_NFC_OK) {
				if (category == NET_NFC_CARD_EMULATION_CATEGORY_PAYMENT) {
					char *handler;

					handler = vconf_get_str(VCONFKEY_NFC_PAYMENT_HANDLERS);
					if (handler == NULL || strlen(handler) == 0) {
						SECURE_MSG("set to default payment handler, [%s]", package);

						__activate_handler(package, category);
					}

					if (handler != NULL) {
						g_free(handler);
					}
				} else {
					if (__is_activated_handler(category, package) == false) {
						__activate_handler(package, category);
					}
				}
			} else if (result == NET_NFC_INSUFFICIENT_STORAGE) {
				net_nfc_card_emulation_category_t temp;

				DEBUG_ERR_MSG("no space. deactive handler, [%d]", result);

				for (temp = NET_NFC_CARD_EMULATION_CATEGORY_PAYMENT;
					temp < NET_NFC_CARD_EMULATION_CATEGORY_MAX;
					temp++) {
					__deactivate_handler(package, temp);
				}
			} else if (result == NET_NFC_DATA_CONFLICTED) {
				net_nfc_card_emulation_category_t temp;

				DEBUG_ERR_MSG("conflict occured. deactive handler, [%d]", result);

				for (temp = NET_NFC_CARD_EMULATION_CATEGORY_PAYMENT;
					temp < NET_NFC_CARD_EMULATION_CATEGORY_MAX;
					temp++) {
					__deactivate_handler(package, temp);
				}
			} else {
				DEBUG_ERR_MSG("net_nfc_server_route_table_add_aid failed, [%d]", result);
			}
		} else {
			DEBUG_ERR_MSG("net_nfc_server_route_table_add_handler failed, [%d]", result);
		}
	} else {
		DEBUG_ERR_MSG("__insert_into_db failed, [%d]", result);
	}

#ifdef PRINT_TABLE
	__display_route_table();
#endif
	return result;
}

net_nfc_error_e net_nfc_server_route_table_delete_aid_from_db(
	const char *package, const char *aid)
{
	net_nfc_error_e result;

	result = net_nfc_server_route_table_del_aid(NULL, package, aid, true);

	return result;
}

net_nfc_error_e net_nfc_server_route_table_delete_aids_from_db(
	const char *package)
{
	net_nfc_error_e result;

	result = net_nfc_server_route_table_del_aids(NULL, package, true);

	return result;
}

static bool __update_table_iter_cb(const char *package,
	route_table_handler_t *handler, void *user_data)
{
	gpointer *params = (gpointer *)user_data;
	int i;
	aid_info_t *info;
	net_nfc_se_type_e se_type;
	net_nfc_card_emulation_category_t category;
	int *ret;
	bool routing;

	se_type = (net_nfc_se_type_e)params[0];
	ret = (int *)params[1];
	routing = (bool)params[2];

	for (category = NET_NFC_CARD_EMULATION_CATEGORY_PAYMENT;
		category < NET_NFC_CARD_EMULATION_CATEGORY_MAX;
		category++) {
		if (__is_preferred_handler(category, package) == true ||
			(preferred_handler == NULL && __is_activated_handler(category, package) == true)) {
			/* update used aid count */
			used_aids_count[category] += handler->aids[category]->len;
			used_aids_count[NET_NFC_CARD_EMULATION_CATEGORY_MAX] += handler->aids[category]->len;

			if (routing == true) {
				for (i = 0; i < handler->aids[category]->len; i++) {
					info = (aid_info_t *)handler->aids[category]->pdata[i];

					if (info->se_type == se_type) {
						/* unroute aid when se type is same with default */
						if (info->is_routed == true) {
							__do_unrouting_aid(info, false);
						}

						continue;
					}

					if (info->is_routed == false) {
						__do_routing_aid(info, false);
					}
				}
			}

			handler->activated[category] = true;
		} else {
			if (routing == true) {
				for (i = 0; i < handler->aids[category]->len; i++) {
					info = (aid_info_t *)handler->aids[category]->pdata[i];

					if (info->is_routed == true) {
						__do_unrouting_aid(info, false);
					}
				}
			}

			handler->activated[category] = false;
		}
	}

	(*ret) = 0;

	return true;
}

net_nfc_error_e net_nfc_server_route_table_do_update(bool routing)
{
	net_nfc_error_e result = NET_NFC_OK;
	gpointer params[3];
	net_nfc_se_type_e se_type;
	int ret = 0;
	net_nfc_card_emulation_category_t category;

//	if (net_nfc_controller_secure_element_clear_aid_table(&result) == false) {
//		DEBUG_ERR_MSG("net_nfc_controller_secure_element_clear_aid_table failed, [%d]", result);
//	}

	se_type = net_nfc_server_se_get_se_type();

	params[0] = (gpointer)se_type;
	params[1] = (gpointer)&ret;
	params[2] = (gpointer)routing;

	for (category = NET_NFC_CARD_EMULATION_CATEGORY_PAYMENT;
		category <= NET_NFC_CARD_EMULATION_CATEGORY_MAX;
		category++) {
		used_aids_count[category] = 0;
	}

	net_nfc_server_route_table_iterate_handler_activated_last(__update_table_iter_cb,
		(void *)params);

	if (ret == 0) {
		INFO_MSG("routing table update complete, [%d/%d]", used_aids_count[NET_NFC_CARD_EMULATION_CATEGORY_MAX], maximum_aids_count);

		result = NET_NFC_OK;
	} else if (ret > 1) {
		DEBUG_ERR_MSG("some packages are not updated, [%d]", ret);

		result = NET_NFC_OPERATION_FAIL;
	} else {
		DEBUG_ERR_MSG("wrong result, [%d]", ret);

		result = NET_NFC_OPERATION_FAIL;
	}

	if (routing == true) {
		net_nfc_controller_secure_element_commit_routing(&result);
	}

#ifdef PRINT_TABLE
	__display_route_table();
#endif

	return result;
}

net_nfc_error_e net_nfc_server_route_table_get_storage_info(
	net_nfc_card_emulation_category_t category, int *used, int *max)
{
	if (used == NULL || max == NULL) {
		return NET_NFC_NULL_PARAMETER;
	}

	*used = used_aids_count[category];
	*max = maximum_aids_count;

	return NET_NFC_OK;
}
