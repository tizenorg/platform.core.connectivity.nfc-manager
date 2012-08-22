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


#include "net_nfc_debug_private.h"
#include "net_nfc_util_defines.h"
#include "net_nfc_util_private.h"
#include "net_nfc_util_ndef_message.h"
#include "net_nfc_util_ndef_record.h"
#include "net_nfc_util_ndef_parser.h"

static net_nfc_error_e __net_nfc_repair_record_flags(ndef_message_s *ndef_message);

net_nfc_error_e net_nfc_util_convert_rawdata_to_ndef_message(data_s *rawdata, ndef_message_s *ndef)
{
	ndef_record_s *newRec = NULL;
	ndef_record_s *prevRec = NULL;
	int dataRead = 0;
	uint8_t *current = NULL;
	int totalLength = 0;
	net_nfc_error_e rst = NET_NFC_OK;

	if (rawdata == NULL || ndef == NULL)
		return NET_NFC_NULL_PARAMETER;

	current = rawdata->buffer;
	ndef->recordCount = 0;

	do
	{
		if (rawdata->length < totalLength)
			return NET_NFC_NDEF_BUF_END_WITHOUT_ME;

		_net_nfc_util_alloc_mem(newRec, sizeof(ndef_record_s));
		if (newRec == NULL)
			return NET_NFC_ALLOC_FAIL;

		rst = __phFriNfc_NdefRecord_Parse(newRec, current, &dataRead);
		if (rst != NET_NFC_OK)
		{
			_net_nfc_util_free_mem(newRec);
			return rst;
		}

		if (ndef->recordCount == 0)
			ndef->records = newRec;
		else
			prevRec->next = newRec;

		prevRec = newRec;
		totalLength += dataRead;
		current += dataRead;
		(ndef->recordCount)++;
	}
	while (!prevRec->ME);

	return rst;
}

net_nfc_error_e net_nfc_util_convert_ndef_message_to_rawdata(ndef_message_s *ndef, data_s *rawdata)
{
	ndef_record_s *current = NULL;
	ndef_record_s *completed = NULL;
	uint8_t *current_buf = NULL;
	int remain = 0;
	uint32_t written = 0;
	net_nfc_error_e rst = NET_NFC_OK;

	if (rawdata == NULL || ndef == NULL)
		return NET_NFC_NULL_PARAMETER;

	current = ndef->records;
	completed = ndef->records;
	current_buf = rawdata->buffer;
	remain = rawdata->length;

	do
	{
		if (remain < 0)
			return NET_NFC_NDEF_BUF_END_WITHOUT_ME;

		rst = __phFriNfc_NdefRecord_Generate(current, current_buf, remain, &written);
		if (rst != NET_NFC_OK)
			return rst;

		completed = current;
		current = current->next;
		remain -= written;
		current_buf += written;
	}
	while (!completed->ME);

	return rst;
}

net_nfc_error_e net_nfc_util_append_record(ndef_message_s *msg, ndef_record_s *record)
{
	if (msg == NULL || record == NULL)
	{
		return NET_NFC_NULL_PARAMETER;
	}

	if (msg->recordCount == 0)
	{
		// set short message and append
		record->MB = 1;
		record->ME = 1;
		record->next = NULL;

		msg->records = record;

		msg->recordCount++;

		DEBUG_MSG("record is added to NDEF message :: count [%d]", msg->recordCount);
	}
	else
	{
		ndef_record_s *current = NULL;
		ndef_record_s *prev = NULL;

		// set flag :: this record is FIRST
		current = msg->records;

		if (current != NULL)
		{
			// first node
			current->MB = 1;
			current->ME = 0;

			prev = current;

			// second node
			current = current->next;

			while (current != NULL)
			{
				current->MB = 0;
				current->ME = 0;
				prev = current;
				current = current->next;
			}

			// set flag :: this record is END
			record->MB = 0;
			record->ME = 1;

			prev->next = record;
			msg->recordCount++;
		}

	}

	return NET_NFC_OK;
}

uint32_t net_nfc_util_get_ndef_message_length(ndef_message_s *message)
{
	ndef_record_s *current;
	int total = 0;

	if (message == NULL)
	{
		return NET_NFC_NULL_PARAMETER;
	}

	current = message->records;

	while (current != NULL)
	{
		total += net_nfc_util_get_record_length(current);
		current = current->next;
	}

	DEBUG_MSG("total byte length = [%d]", total);

	return total;
}

void net_nfc_util_print_ndef_message(ndef_message_s *msg)
{
	int idx = 0, idx2 = 0;
	ndef_record_s *current = NULL;
	char buffer[1024];

	if (msg == NULL)
	{
		return;
	}

	//                123456789012345678901234567890123456789012345678901234567890
	printf("========== NDEF Message ====================================\n");
	printf("Total NDEF Records count: %d\n", msg->recordCount);
	current = msg->records;
	for (idx = 0; idx < msg->recordCount; idx++)
	{
		if (current == NULL)
		{
			DEBUG_ERR_MSG("Message Record is NULL!! unexpected error");
			printf("============================================================\n");
			return;
		}
		printf("---------- Record -----------------------------------------\n");
		printf("MB:%d ME:%d CF:%d SR:%d IL:%d TNF:0x%02X\n",
			current->MB, current->ME, current->CF, current->SR, current->IL, current->TNF);
		printf("TypeLength:%d  PayloadLength:%d  IDLength:%d\n",
			current->type_s.length, current->payload_s.length, current->id_s.length);
		if (current->type_s.buffer != NULL)
		{
			memcpy(buffer, current->type_s.buffer, current->type_s.length);
			buffer[current->type_s.length] = '\0';
			printf("Type: %s\n", buffer);
		}
		if (current->id_s.buffer != NULL)
		{
			memcpy(buffer, current->id_s.buffer, current->id_s.length);
			buffer[current->id_s.length] = '\0';
			printf("ID: %s\n", buffer);
		}
		if (current->payload_s.buffer != NULL)
		{
			printf("Payload: ");
			for (idx2 = 0; idx2 < current->payload_s.length; idx2++)
			{
				if (idx2 % 16 == 0)
					printf("\n\t");
				printf("%02X ", current->payload_s.buffer[idx2]);
			}
			printf("\n");
		}
		current = current->next;
	}
	//                123456789012345678901234567890123456789012345678901234567890
	printf("============================================================\n");

}

net_nfc_error_e net_nfc_util_free_ndef_message(ndef_message_s *msg)
{
	int idx = 0;
	ndef_record_s *prev, *current;

	if (msg == NULL)
		return NET_NFC_NULL_PARAMETER;

	current = msg->records;

	for (idx = 0; idx < msg->recordCount; idx++)
	{
		if (current == NULL)
			break;

		prev = current;
		current = current->next;

		net_nfc_util_free_record(prev);
	}

	_net_nfc_util_free_mem(msg);

	return NET_NFC_OK;
}

net_nfc_error_e net_nfc_util_create_ndef_message(ndef_message_s **ndef_message)
{
	if (ndef_message == NULL)
	{
		return NET_NFC_NULL_PARAMETER;
	}

	_net_nfc_util_alloc_mem(*ndef_message, sizeof(ndef_message_s));

	return NET_NFC_OK;
}

net_nfc_error_e net_nfc_util_remove_record_by_index(ndef_message_s *ndef_message, int index)
{
	int current_idx = 0;
	ndef_record_s *prev;
	ndef_record_s *next;
	ndef_record_s *current;

	if (ndef_message == NULL)
	{
		return NET_NFC_NULL_PARAMETER;
	}

	if (index < 0 || index >= ndef_message->recordCount)
	{
		return NET_NFC_OUT_OF_BOUND;
	}

	if (index == 0)
	{
		current = ndef_message->records;
		next = ndef_message->records->next;
		ndef_message->records = next;
	}
	else
	{
		prev = ndef_message->records;
		for (; current_idx < index - 1; current_idx++)
		{
			prev = prev->next;
			if (prev == NULL)
			{
				return NET_NFC_INVALID_FORMAT;
			}
		}
		current = prev->next;
		if (current == NULL)
		{
			return NET_NFC_INVALID_FORMAT;
		}
		next = current->next;
		prev->next = next;
	}

	net_nfc_util_free_record(current);
	(ndef_message->recordCount)--;

	return __net_nfc_repair_record_flags(ndef_message);
}

net_nfc_error_e net_nfc_util_get_record_by_index(ndef_message_s *ndef_message, int index, ndef_record_s **record)
{
	ndef_record_s *current;
	int idx = 0;

	if (ndef_message == NULL || record == NULL)
	{
		return NET_NFC_NULL_PARAMETER;
	}

	if (index < 0 || index >= ndef_message->recordCount)
	{
		return NET_NFC_OUT_OF_BOUND;
	}

	current = ndef_message->records;

	for (; current != NULL && idx < index; idx++)
	{
		current = current->next;
	}

	*record = current;

	return NET_NFC_OK;
}

net_nfc_error_e net_nfc_util_append_record_by_index(ndef_message_s *ndef_message, int index, ndef_record_s *record)
{
	int idx = 0;
	ndef_record_s *prev;
	ndef_record_s *next;

	if (ndef_message == NULL || record == NULL)
	{
		return NET_NFC_NULL_PARAMETER;
	}

	if (index < 0 || index > ndef_message->recordCount)
	{
		return NET_NFC_OUT_OF_BOUND;
	}

	prev = ndef_message->records;

	if (index == 0)
	{
		ndef_message->records = record;
		record->next = prev;
	}
	else
	{
		for (; idx < index - 1; idx++)
		{
			prev = prev->next;
			if (prev == NULL)
			{
				return NET_NFC_INVALID_FORMAT;
			}
		}
		next = prev->next;
		prev->next = record;
		record->next = next;
	}
	(ndef_message->recordCount)++;

	return __net_nfc_repair_record_flags(ndef_message);
}

net_nfc_error_e net_nfc_util_search_record_by_type(ndef_message_s *ndef_message, net_nfc_record_tnf_e tnf, data_s *type, ndef_record_s **record)
{
	int idx = 0;
	ndef_record_s *record_private;
	uint32_t type_length;
	uint8_t *buf;

	if (ndef_message == NULL || type == NULL || record == NULL)
	{
		return NET_NFC_NULL_PARAMETER;
	}

	type_length = type->length;
	buf = type->buffer;

	/* remove prefix of nfc specific urn */
	if (type_length > 12)
	{
		if (memcmp(buf, "urn:nfc:ext:", 12) == 0 ||
			memcmp(buf, "urn:nfc:wkt:", 12) == 0)
		{
			buf += 12;
			type_length -= 12;
		}
	}

	record_private = ndef_message->records;

	for (; idx < ndef_message->recordCount; idx++)
	{
		if (record_private == NULL)
		{
			*record = NULL;

			return NET_NFC_INVALID_FORMAT;
		}

		if (record_private->TNF == tnf &&
			type_length == record_private->type_s.length &&
			memcmp(buf, record_private->type_s.buffer, type_length) == 0)
		{
			*record = record_private;

			return NET_NFC_OK;
		}

		record_private = record_private->next;
	}

	return NET_NFC_NO_DATA_FOUND;
}

net_nfc_error_e net_nfc_util_search_record_by_id(ndef_message_s *ndef_message, data_s *id, ndef_record_s **record)
{
	int idx = 0;
	ndef_record_s *record_in_msg;
	uint32_t id_length;
	uint8_t *buf;

	if (ndef_message == NULL || id == NULL || record == NULL)
	{
		return NET_NFC_NULL_PARAMETER;
	}

	id_length = id->length;
	buf = id->buffer;

	record_in_msg = ndef_message->records;

	for (; idx < ndef_message->recordCount; idx++)
	{
		if (record_in_msg == NULL)
		{
			*record = NULL;

			return NET_NFC_INVALID_FORMAT;
		}
		if (id_length == record_in_msg->id_s.length &&
			memcmp(buf, record_in_msg->id_s.buffer, id_length) == 0)
		{
			*record = record_in_msg;

			return NET_NFC_OK;
		}

		record_in_msg = record_in_msg->next;
	}

	return NET_NFC_NO_DATA_FOUND;
}

static net_nfc_error_e __net_nfc_repair_record_flags(ndef_message_s *ndef_message)
{
	int idx = 0;
	ndef_record_s *record;

	if (ndef_message == NULL)
	{
		return NET_NFC_NULL_PARAMETER;
	}

	record = ndef_message->records;

	if (ndef_message->recordCount == 1)
	{
		if (record == NULL)
		{
			return NET_NFC_INVALID_FORMAT;
		}

		record->MB = 1;
		record->ME = 1;

		return NET_NFC_OK;
	}

	for (idx = 0; idx < ndef_message->recordCount; idx++)
	{
		if (record == NULL)
		{
			return NET_NFC_INVALID_FORMAT;
		}

		if (idx == 0)
		{
			record->MB = 1;
			record->ME = 0;
		}
		else if (idx == ndef_message->recordCount - 1)
		{
			record->MB = 0;
			record->ME = 1;
		}
		else
		{
			record->MB = 0;
			record->ME = 0;
		}
		record = record->next;
	}

	return NET_NFC_OK;
}

