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

#include <stdlib.h>
#include <string.h>

#include "vconf.h"
#include "feedback.h"
#include "wav_player.h"
#include <mm_sound_private.h>

#include "net_nfc_debug_internal.h"
#include "net_nfc_util_internal.h"
#include "net_nfc_manager_util_internal.h"

static void __focus_changed_cb(sound_stream_info_h stream_info,
	sound_stream_focus_change_reason_e reason_for_change, const char *additional_info, void *user_data)
{

}

static void __wav_start_completed_cb(int id, void *user_data)
{
	sound_stream_info_h stream_info = (sound_stream_info_h) user_data;

	sound_manager_release_focus(stream_info, SOUND_STREAM_FOCUS_FOR_PLAYBACK, NULL);
	sound_manager_destroy_stream_information(stream_info);
}


void net_nfc_manager_util_play_sound(net_nfc_sound_type_e sound_type)
{
	int bSoundOn = 0;
	int bVibrationOn = 0;

	if (vconf_get_bool(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, &bSoundOn) != 0)
	{
		DEBUG_SERVER_MSG("vconf_get_bool failed for Sound");
		return;
	}

	if (vconf_get_bool(VCONFKEY_SETAPPL_VIBRATION_STATUS_BOOL, &bVibrationOn) != 0)
	{
		DEBUG_SERVER_MSG("vconf_get_bool failed for Vibration");
		return;
	}

	if ((sound_type > NET_NFC_TASK_ERROR) || (sound_type < NET_NFC_TASK_START))
	{
		DEBUG_SERVER_MSG("Invalid Sound Type");
		return;
	}

	if (bVibrationOn)
	{
		DEBUG_SERVER_MSG("Play Vibration");

		if (FEEDBACK_ERROR_NONE == feedback_initialize())
		{
			if (FEEDBACK_ERROR_NONE ==  feedback_play_type(FEEDBACK_TYPE_VIBRATION, FEEDBACK_PATTERN_SIP))
			{
				DEBUG_SERVER_MSG("feedback_play_type success");
			}

			feedback_deinitialize();
		}
	}

	if (bSoundOn)
	{
		char *sound_path = NULL;

		DEBUG_SERVER_MSG("Play Sound");

		switch (sound_type)
		{
		case NET_NFC_TASK_START :
			sound_path = strdup(NET_NFC_MANAGER_SOUND_PATH_TASK_START);
			break;
		case NET_NFC_TASK_END :
			sound_path = strdup(NET_NFC_MANAGER_SOUND_PATH_TASK_END);
			break;
		case NET_NFC_TASK_ERROR :
			sound_path = strdup(NET_NFC_MANAGER_SOUND_PATH_TASK_ERROR);
			break;
		}

		if (sound_path != NULL)
		{
			int id;
			int ret;
			sound_stream_info_h stream_info;

			ret = sound_manager_create_stream_information(SOUND_STREAM_TYPE_NOTIFICATION, __focus_changed_cb, NULL, &stream_info);

			if (ret == SOUND_MANAGER_ERROR_NONE) {
				ret = sound_manager_acquire_focus(stream_info, SOUND_STREAM_FOCUS_FOR_PLAYBACK, NULL);
				if (ret == SOUND_MANAGER_ERROR_NONE) {
					ret = wav_player_start_with_stream_info(sound_path,
						stream_info, __wav_start_completed_cb, stream_info, &id);
					if (ret == WAV_PLAYER_ERROR_NONE) {
						DEBUG_SERVER_MSG("wav_player_start_with_stream_info success");
					} else {
						DEBUG_SERVER_MSG("wav_player_start_with_stream_info failed");
					}
				} else {
					DEBUG_SERVER_MSG("sound_manager_acquire_focus failed");
				}
			} else {
				DEBUG_SERVER_MSG("sound_manager_create_stream_information failed");
			}

			_net_nfc_util_free_mem(sound_path);
		}
		else
		{
			DEBUG_ERR_MSG("Invalid Sound Path");
		}
	}
}
