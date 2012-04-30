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

#ifndef NET_NFC_CONTROLLER_H
#define NET_NFC_CONTROLLER_H

#include "net_nfc_typedef.h"
#include "net_nfc_typedef_private.h"

void* net_nfc_controller_onload(void);
bool net_nfc_controller_unload(void* handle);
bool net_nfc_controller_init (net_nfc_error_e* result);
bool net_nfc_controller_deinit (void);
bool net_nfc_controller_register_listener(target_detection_listener_cb target_detection_listener, se_transaction_listener_cb se_transaction_listener, llcp_event_listener_cb llcp_event_listener, net_nfc_error_e* result);
bool net_nfc_controller_unregister_listener(void);
bool net_nfc_controller_check_firmware_version(net_nfc_error_e* result);
bool net_nfc_controller_update_firmware(net_nfc_error_e* result);
bool net_nfc_controller_get_stack_information(net_nfc_stack_information_s* stack_info, net_nfc_error_e* result);
bool net_nfc_controller_confiure_discovery (net_nfc_discovery_mode_e mode, net_nfc_event_filter_e config, net_nfc_error_e* result);
bool net_nfc_controller_get_secure_element_list(net_nfc_secure_element_info_s* list, int* count, net_nfc_error_e* result);
bool net_nfc_controller_set_secure_element_mode(net_nfc_secure_element_type_e element_type, net_nfc_secure_element_mode_e mode, net_nfc_error_e* result);
bool net_nfc_controller_check_target_presence(net_nfc_target_handle_s* handle, net_nfc_error_e* result);
bool net_nfc_controller_connect(net_nfc_target_handle_s* handle, net_nfc_error_e* result);
bool net_nfc_controller_disconnect(net_nfc_target_handle_s* handle, net_nfc_error_e* result);
bool net_nfc_controller_check_ndef(net_nfc_target_handle_s* handle, uint8_t *ndef_card_state, int* max_data_size, int* real_data_size, net_nfc_error_e* result);
bool net_nfc_controller_read_ndef(net_nfc_target_handle_s* handle, data_s** data, net_nfc_error_e* result);
bool net_nfc_controller_write_ndef(net_nfc_target_handle_s* handle, data_s* data, net_nfc_error_e* result);
bool net_nfc_controller_make_read_only_ndef(net_nfc_target_handle_s* handle,  net_nfc_error_e* result);
bool net_nfc_controller_format_ndef(net_nfc_target_handle_s* handle, data_s* secure_key, net_nfc_error_e* result);
bool net_nfc_controller_transceive (net_nfc_target_handle_s* handle, net_nfc_transceive_info_s* info, data_s** data, net_nfc_error_e* result);
bool net_nfc_controller_exception_handler(void);
bool net_nfc_controller_is_ready(net_nfc_error_e* result);

// LLCP API DEFINE

bool net_nfc_controller_llcp_config(net_nfc_llcp_config_info_s * config, net_nfc_error_e * result);
bool net_nfc_controller_llcp_check_llcp(net_nfc_target_handle_s* handle, net_nfc_error_e* result);
bool net_nfc_controller_llcp_activate_llcp(net_nfc_target_handle_s * handle, net_nfc_error_e* result);
bool net_nfc_controller_llcp_create_socket(net_nfc_llcp_socket_t* socket, net_nfc_socket_type_e socketType, uint16_t miu, uint8_t rw,  net_nfc_error_e* result, void * user_param);
bool net_nfc_controller_llcp_bind(net_nfc_llcp_socket_t socket, uint8_t service_access_point, net_nfc_error_e* result);
bool net_nfc_controller_llcp_listen(net_nfc_target_handle_s* handle, uint8_t* service_access_name, net_nfc_llcp_socket_t socket, net_nfc_error_e* result, void * user_param);
bool net_nfc_controller_llcp_accept(net_nfc_llcp_socket_t socket, net_nfc_error_e* result);
bool net_nfc_controller_llcp_connect_by_url(net_nfc_target_handle_s * handle, net_nfc_llcp_socket_t socket, uint8_t* service_access_name, net_nfc_error_e* result, void * user_param);
bool net_nfc_controller_llcp_connect(net_nfc_target_handle_s * handle, net_nfc_llcp_socket_t socket, uint8_t service_access_point, net_nfc_error_e* result, void * user_param);
bool net_nfc_controller_llcp_disconnect(net_nfc_target_handle_s * handle, net_nfc_llcp_socket_t socket, net_nfc_error_e* result, void * user_param);
bool net_nfc_controller_llcp_socket_close(net_nfc_llcp_socket_t socket, net_nfc_error_e* result);
bool net_nfc_controller_llcp_recv(net_nfc_target_handle_s* handle, net_nfc_llcp_socket_t socket, data_s* data, net_nfc_error_e* result, void * user_param);
bool net_nfc_controller_llcp_send(net_nfc_target_handle_s* handle, net_nfc_llcp_socket_t socket, data_s* data, net_nfc_error_e* result, void * user_param);
bool net_nfc_controller_llcp_recv_from(net_nfc_target_handle_s* handle, net_nfc_llcp_socket_t	socket, data_s* data, net_nfc_error_e* result, void * user_param);
bool net_nfc_controller_llcp_send_to(net_nfc_target_handle_s* handle, net_nfc_llcp_socket_t	socket, data_s* data, uint8_t service_access_point, net_nfc_error_e* result, void * user_param);
bool net_nfc_controller_llcp_reject(net_nfc_target_handle_s* handle, net_nfc_llcp_socket_t	socket, net_nfc_error_e* result);
bool net_nfc_controller_llcp_get_remote_config (net_nfc_target_handle_s* handle, net_nfc_llcp_config_info_s *config, net_nfc_error_e* result);
bool net_nfc_controller_llcp_get_remote_socket_info (net_nfc_target_handle_s* handle, net_nfc_llcp_socket_t socket, net_nfc_llcp_socket_option_s * option, net_nfc_error_e* result);



// LLCP API DEFINE

bool net_nfc_controller_sim_test(net_nfc_error_e* result);
bool net_nfc_controller_test_mode_on(net_nfc_error_e* result);
bool net_nfc_controller_test_mode_off(net_nfc_error_e* result);
bool net_nfc_test_sim(void);
#endif

