/*
  * Copyright (C) 2010 NXP Semiconductors
  * Copyright (C) 2012 Samsung Electronics Co., Ltd
  *
  * Licensed under the Apache License, Version 2.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *      http://www.apache.org/licenses/LICENSE-2.0
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

/* Calculate the Flags of the record */
void __phFriNfc_NdefRecord_RecordFlag(uint8_t *rawData, ndef_record_s* record)
{
	if (*rawData & SLP_FRINET_NFC_NDEFRECORD_FLAGS_MB)
	{
		record->MB = 1;
	}
	if (*rawData & SLP_FRINET_NFC_NDEFRECORD_FLAGS_ME)
	{
		record->ME = 1;
	}
	if (*rawData & SLP_FRINET_NFC_NDEFRECORD_FLAGS_CF)
	{
		record->CF = 1;
	}
	if (*rawData & SLP_FRINET_NFC_NDEFRECORD_FLAGS_SR)
	{
		record->SR = 1;
	}
	if (*rawData & SLP_FRINET_NFC_NDEFRECORD_FLAGS_IL)
	{
		record->IL = 1;
	}
	DEBUG_MSG("__phFriNfc_NdefRecord_RecordFlag MB[%d]ME[%d]CF[%d]SR[%d]IL[%d]", record->MB,record->ME,record->CF,record->SR,record->IL  );


}

/* Calculate the Type Name Format for the record */
uint8_t __phFriNfc_NdefRecord_TypeNameFormat(uint8_t *rawData, ndef_record_s* record)
{
	uint8_t tnf = *rawData & SLP_FRINET_NFC_NDEFRECORD_TNFBYTE_MASK;

	switch (tnf)
	{
	case NET_NFC_NDEF_TNF_EMPTY :
		case NET_NFC_NDEF_TNF_NFCWELLKNOWN :
		case NET_NFC_NDEF_TNF_MEDIATYPE :
		case NET_NFC_NDEF_TNF_ABSURI :
		case NET_NFC_NDEF_TNF_NFCEXT :
		case NET_NFC_NDEF_TNF_UNKNOWN :
		case NET_NFC_NDEF_TNF_UNCHANGED :
		case NET_NFC_NDEF_TNF_RESERVED :
		record->TNF = tnf;
		break;
	default :
		tnf = 0xFF;
		break;
	}
	DEBUG_MSG("__phFriNfc_NdefRecord_TypeNameFormat tnf[%d]", tnf  );

	return tnf;

}

net_nfc_error_e __phFriNfc_NdefRecord_RecordIDCheck(uint8_t *rawData,
	uint8_t *TypeLength,
	uint8_t *TypeLengthByte,
	uint8_t *PayloadLengthByte,
	uint32_t *PayloadLength,
	uint8_t *IDLengthByte,
	uint8_t *IDLength,
	ndef_record_s *record)
{
	net_nfc_error_e Status = NET_NFC_OK;

	/* Check for Type Name Format  depending on the TNF,  Type Length value is set*/
	if (record->TNF == NET_NFC_NDEF_TNF_EMPTY)
	{
		*TypeLength = *(rawData + SLP_FRINET_NFC_NDEFRECORD_BUF_INC1);

		if (*(rawData + SLP_FRINET_NFC_NDEFRECORD_BUF_INC1) != 0)
		{
			/* Type Length  Error */
			Status = NET_NFC_NDEF_TYPE_LENGTH_IS_NOT_OK;
			return Status;
		}

		*TypeLengthByte = 1;

		/* Check for Short Record */
		if (record->SR)
		{
			/* For Short Record, Payload Length Byte is 1 */
			*PayloadLengthByte = 1;
			/*  1 for Header byte */
			*PayloadLength = *(rawData + *TypeLengthByte + 1);
			if (*PayloadLength != 0)
			{
				/* PayloadLength  Error */
				Status = NET_NFC_NDEF_PAYLOAD_LENGTH_IS_NOT_OK;
				return Status;
			}
		}
		else
		{
			/* For Normal Record, Payload Length Byte is 4 */
			*PayloadLengthByte = SLPFRINFCNDEFRECORD_NORMAL_RECORD_BYTE;
			*PayloadLength = ((((uint32_t)(*(rawData + SLP_FRINET_NFC_NDEFRECORD_BUF_INC2))) << SLPNFCSTSHL24) +
				(((uint32_t)(*(rawData + SLP_FRINET_NFC_NDEFRECORD_BUF_INC3))) << SLPNFCSTSHL16) +
				(((uint32_t)(*(rawData + SLP_FRINET_NFC_NDEFRECORD_BUF_INC4))) << SLPNFCSTSHL8) +
				*(rawData + SLP_FRINET_NFC_NDEFRECORD_BUF_INC5));
			if (*PayloadLength != 0)
			{
				/* PayloadLength  Error */
				Status = NET_NFC_NDEF_PAYLOAD_LENGTH_IS_NOT_OK;
				return Status;
			}
		}

		/* Check for ID Length existence */
		if (record->IL)
		{
			/* Length Byte exists and it is 1 byte */
			*IDLengthByte = 1;
			/*  1 for Header byte */
			*IDLength = (uint8_t)*(rawData + *PayloadLengthByte + *TypeLengthByte + SLP_FRINET_NFC_NDEFRECORD_BUF_INC1);
			if (*IDLength != 0)
			{
				/* IDLength  Error */
				Status = NET_NFC_NDEF_ID_LENGTH_IS_NOT_OK;
				return Status;
			}
		}
		else
		{
			*IDLengthByte = 0;
			*IDLength = 0;
		}
	}
	else if(record->TNF == NET_NFC_NDEF_TNF_ABSURI) /* temp_patch_for_absoluteURI */
	{
		*TypeLength = *(rawData + SLP_FRINET_NFC_NDEFRECORD_BUF_INC1);
		*TypeLengthByte = 1;
		DEBUG_MSG("__phFriNfc_NdefRecord_RecordIDCheck TypeLength = [%d]", *TypeLength);


		/* Check for Short Record */
		if (record->SR)
		{
			/* For Short Record, Payload Length Byte is 1 */
			*PayloadLengthByte = 1;
			/*  1 for Header byte */
			*PayloadLength = *(rawData + *TypeLengthByte + SLP_FRINET_NFC_NDEFRECORD_BUF_INC1);
		}
		else
		{
			/* For Normal Record, Payload Length Byte is 4 */
			*PayloadLengthByte = SLPFRINFCNDEFRECORD_NORMAL_RECORD_BYTE;
			*PayloadLength = ((((uint32_t)(*(rawData + SLP_FRINET_NFC_NDEFRECORD_BUF_INC2))) << SLPNFCSTSHL24) +
				(((uint32_t)(*(rawData + SLP_FRINET_NFC_NDEFRECORD_BUF_INC3))) << SLPNFCSTSHL16) +
				(((uint32_t)(*(rawData + SLP_FRINET_NFC_NDEFRECORD_BUF_INC4))) << SLPNFCSTSHL8) +
				*(rawData + SLP_FRINET_NFC_NDEFRECORD_BUF_INC5));
		}

		if (record->IL)
		{
			*IDLengthByte = 1;
			/*  1 for Header byte */
			*IDLength = (uint8_t)*(rawData + *PayloadLengthByte + *TypeLengthByte + SLP_FRINET_NFC_NDEFRECORD_BUF_INC1);
		}
		else
		{
			*IDLengthByte = 0;
			*IDLength = 0;
		}

	}
	else
	{
		if (record->TNF == NET_NFC_NDEF_TNF_UNKNOWN
			|| record->TNF == NET_NFC_NDEF_TNF_UNCHANGED)
		{
			if (*(rawData + SLP_FRINET_NFC_NDEFRECORD_BUF_INC1) != 0)
			{
				/* Type Length  Error */
				Status = NET_NFC_NDEF_TYPE_LENGTH_IS_NOT_OK;
				return Status;
			}
			*TypeLength = 0;
			*TypeLengthByte = 1;
		}
		else
		{
			/*  1 for Header byte */
			*TypeLength = *(rawData + SLP_FRINET_NFC_NDEFRECORD_BUF_INC1);
			*TypeLengthByte = 1;
		}

		/* Check for Short Record */
		if (record->SR)
		{
			/* For Short Record, Payload Length Byte is 1 */
			*PayloadLengthByte = 1;
			/*  1 for Header byte */
			*PayloadLength = *(rawData + *TypeLengthByte + SLP_FRINET_NFC_NDEFRECORD_BUF_INC1);
		}
		else
		{
			/* For Normal Record, Payload Length Byte is 4 */
			*PayloadLengthByte = SLPFRINFCNDEFRECORD_NORMAL_RECORD_BYTE;
			*PayloadLength = ((((uint32_t)(*(rawData + SLP_FRINET_NFC_NDEFRECORD_BUF_INC2))) << SLPNFCSTSHL24) +
				(((uint32_t)(*(rawData + SLP_FRINET_NFC_NDEFRECORD_BUF_INC3))) << SLPNFCSTSHL16) +
				(((uint32_t)(*(rawData + SLP_FRINET_NFC_NDEFRECORD_BUF_INC4))) << SLPNFCSTSHL8) +
				*(rawData + SLP_FRINET_NFC_NDEFRECORD_BUF_INC5));
		}

		/* Check for ID Length existence */
		if (record->IL)
		{
			*IDLengthByte = 1;
			/*  1 for Header byte */
			*IDLength = (uint8_t)*(rawData + *PayloadLengthByte + *TypeLengthByte + SLP_FRINET_NFC_NDEFRECORD_BUF_INC1);
		}
		else
		{
			*IDLengthByte = 0;
			*IDLength = 0;
		}
	}
	return Status;
}

/*!
 *
 *  Extract a specific NDEF record from the data, provided by the caller. The data is a buffer holding
 *  at least the entire NDEF record (received via the NFC link, for example).
 *
 * \param[out] Record               The NDEF record structure. The storage for the structure has to be provided by the
 *                                  caller matching the requirements for \b Extraction, as described in the compound
 *                                  documentation.
 * \param[in]  RawRecord            The Pointer to the buffer, selected out of the array returned by
 *                                  the \ref phFriNfc_NdefRecord_GetRecords function.
 *
 * \retval NFCSTATUS_SUCCESS                Operation successful.
 * \retval NFCSTATUS_INVALID_PARAMETER      At least one parameter of the function is invalid.
 *
 * \note There are some caveats:
 *       - The "RawRecord" Data buffer must exist at least as long as the function execution time plus the time
 *         needed by the caller to evaluate the extracted information. No copying of the contained data is done.
 *       - Using the "RawRecord" and "RawRecordMaxSize" parameters the function internally checks whether the
 *         data to extract are within the bounds of the buffer.
 *
 *
 */
net_nfc_error_e __phFriNfc_NdefRecord_Parse(ndef_record_s*Record, uint8_t *RawRecord, int *readData)
{
	net_nfc_error_e Status = NET_NFC_OK;
	uint8_t PayloadLengthByte = 0,
		TypeLengthByte = 0,
		TypeLength = 0,
		IDLengthByte = 0,
		IDLength = 0,
		Tnf = 0;
	uint32_t PayloadLength = 0;
	*readData = 0;
	uint8_t *original = RawRecord;

	if (Record == NULL || RawRecord == NULL)
	{
		Status = NET_NFC_NULL_PARAMETER;
	}
	else
	{
		/* Calculate the Flag Value */
		__phFriNfc_NdefRecord_RecordFlag(RawRecord, Record);

		/* Calculate the Type Namr format of the record */
		Tnf = __phFriNfc_NdefRecord_TypeNameFormat(RawRecord, Record);
		if (Tnf != 0xFF)
		{
			/* To Calculate the IDLength and PayloadLength for short or normal record */
			Status = __phFriNfc_NdefRecord_RecordIDCheck(RawRecord,
				&TypeLength,
				&TypeLengthByte,
				&PayloadLengthByte,
				&PayloadLength,
				&IDLengthByte,
				&IDLength
				, Record);
			Record->type_s.length = TypeLength;
			Record->payload_s.length = PayloadLength;
			Record->id_s.length = IDLength;
			RawRecord = (RawRecord + PayloadLengthByte + IDLengthByte + TypeLengthByte + SLP_FRINET_NFC_NDEFRECORD_BUF_INC1);

			if (Record->type_s.length != 0)
			{
				_net_nfc_util_alloc_mem((Record->type_s.buffer), Record->type_s.length);

				if (Record->type_s.buffer != NULL)
					memcpy(Record->type_s.buffer, RawRecord, Record->type_s.length);
			}

			RawRecord = (RawRecord + Record->type_s.length);

			if (Record->id_s.length != 0)
			{
				_net_nfc_util_alloc_mem(Record->id_s.buffer, Record->id_s.length);
				if (Record->id_s.buffer != NULL)
					memcpy(Record->id_s.buffer, RawRecord, Record->id_s.length);
			}

			RawRecord = RawRecord + Record->id_s.length;

			if (Record->payload_s.length != 0)
			{
				_net_nfc_util_alloc_mem(Record->payload_s.buffer, Record->payload_s.length);
				if (Record->payload_s.buffer != NULL)
					memcpy(Record->payload_s.buffer, RawRecord, Record->payload_s.length);
			}
			DEBUG_MSG("__phFriNfc_NdefRecord_Parse Record->type_s.buffer= [%s]", Record->type_s.buffer);

			*readData = RawRecord + Record->payload_s.length - original;
		}
		else
		{
			Status = NET_NFC_ALLOC_FAIL;
		}
	}
	return Status;
}

/* Calculate the Flags of the record */
uint8_t __phFriNfc_NdefRecord_GenFlag(ndef_record_s* record)
{
	uint8_t flag = 0x00;
	if (record->MB)
	{
		flag = flag | SLP_FRINET_NFC_NDEFRECORD_FLAGS_MB;
	}
	if (record->ME)
	{
		flag = flag | SLP_FRINET_NFC_NDEFRECORD_FLAGS_ME;
	}
	if (record->CF)
	{
		flag = flag | SLP_FRINET_NFC_NDEFRECORD_FLAGS_CF;
	}
	if (record->SR)
	{
		flag = flag | SLP_FRINET_NFC_NDEFRECORD_FLAGS_SR;
	}
	if (record->IL)
	{
		flag = flag | SLP_FRINET_NFC_NDEFRECORD_FLAGS_IL;
	}
	return flag;
}

/*!
 *  The function writes one NDEF record to a specified memory location. Called within a loop, it is possible to
 *  write more records into a contiguous buffer, in each cycle advancing by the number of bytes written for
 *  each record.
 *
 * \param[in]     Record             The Array of NDEF record structures to append. The structures
 *                                   have to be filled by the caller matching the requirements for
 *                                   \b Composition, as described in the documentation of
 *                                   the \ref phFriNfc_NdefRecord_t "NDEF Record" structure.
 * \param[in]     Buffer             The pointer to the buffer.
 * \param[in]     MaxBufferSize      The data buffer's maximum size, provided by the caller.
 * \param[out]    BytesWritten       The actual number of bytes written to the buffer. This can be used by
 *                                   the caller to serialise more than one record into the same buffer before
 *                                   handing it over to another instance.
 *
 * \retval NFCSTATUS_SUCCESS                  Operation successful.
 * \retval NFCSTATUS_INVALID_PARAMETER        At least one parameter of the function is invalid.
 * \retval NFCSTATUS_BUFFER_TOO_SMALL         The data buffer, provided by the caller is to small to
 *                                            hold the composed NDEF record. The existing content is not changed.
 *
 */
net_nfc_error_e __phFriNfc_NdefRecord_Generate(ndef_record_s *Record,
	uint8_t *Buffer,
	uint32_t MaxBufferSize,
	uint32_t *BytesWritten)
{
	uint8_t FlagCheck,
		TypeCheck = 0,
		*temp,
		i;
	uint32_t i_data = 0;

	if (Record == NULL || Buffer == NULL || BytesWritten == NULL || MaxBufferSize == 0)
	{
		DEBUG_MSG("Record = [%p]||Buffer = [%p]||BytesWritten = [%p]||MaxBufferSize = [%d]", Record, Buffer, BytesWritten, MaxBufferSize);
		return NET_NFC_NULL_PARAMETER;
	}

	/* calculate the length of the record and check with the buffersize if it exceeds return */
	i_data = net_nfc_util_get_record_length(Record);
	if (i_data > MaxBufferSize)
	{
		return NET_NFC_INVALID_FORMAT;
	}
	*BytesWritten = i_data;

	/*fill the first byte of the message(all the flags) */
	/*increment the buffer*/
	*Buffer = (__phFriNfc_NdefRecord_GenFlag(Record) | Record->TNF);
	Buffer++;

	/* check the TypeNameFlag for SLP_FRINET_NFC_NDEFRECORD_TNF_EMPTY */
	FlagCheck = Record->TNF;
	if (FlagCheck == NET_NFC_NDEF_TNF_EMPTY)
	{
		/* fill the typelength idlength and payloadlength with zero(empty message)*/
		for (i = 0; i < 3; i++)
		{
			*Buffer = SLP_FRINET_NFC_NDEFRECORD_BUF_TNF_VALUE;
			Buffer++;
		}
		return NET_NFC_OK;
	}

	/* check for TNF Unknown or Unchanged */
	FlagCheck = Record->TNF;
	if (FlagCheck == NET_NFC_NDEF_TNF_UNKNOWN ||
		FlagCheck == NET_NFC_NDEF_TNF_UNCHANGED)
	{
		*Buffer = SLP_FRINET_NFC_NDEFRECORD_BUF_TNF_VALUE;
		Buffer++;
	}
	else if (FlagCheck == NET_NFC_NDEF_TNF_ABSURI )  /* temp_patch_for_absoluteURI */
	{
		*Buffer = Record->type_s.length;
		Buffer++;
		TypeCheck = 1;
	}
	else
	{
		*Buffer = Record->type_s.length;
		Buffer++;
		TypeCheck = 1;
	}

	/* check for the short record bit if it is then payloadlength is only one byte */
	FlagCheck = Record->SR;
	if (FlagCheck != 0)
	{
		*Buffer = (uint8_t)(Record->payload_s.length & 0x000000ff);
		Buffer++;
	}
	else
	{
		/* if it is normal record payloadlength is 4 byte(32 bit)*/
		*Buffer = (uint8_t)((Record->payload_s.length & 0xff000000) >> SLPNFCSTSHL24);
		Buffer++;
		*Buffer = (uint8_t)((Record->payload_s.length & 0x00ff0000) >> SLPNFCSTSHL16);
		Buffer++;
		*Buffer = (uint8_t)((Record->payload_s.length & 0x0000ff00) >> SLPNFCSTSHL8);
		Buffer++;
		*Buffer = (uint8_t)((Record->payload_s.length & 0x000000ff));
		Buffer++;
	}

	/*check for IL bit set(Flag), if so then IDlength is present*/
	FlagCheck = Record->IL;

	if (FlagCheck != 0)
	{
		*Buffer = Record->id_s.length;
		Buffer++;
	}

	/*check for TNF and fill the Type*/
	temp = Record->type_s.buffer;
	if (TypeCheck != 0)
	{
		for (i = 0; i < (Record->type_s.length); i++)
		{
			*Buffer = *temp;
			Buffer++;
			temp++;
		}
	}

	/*check for IL bit set(Flag), if so then IDlength is present and fill the ID*/
	//FlagCheck=Record->IL;
	temp = Record->id_s.buffer;

	if (FlagCheck != 0)
	{
		for (i = 0; i < (Record->id_s.length); i++)
		{
			*Buffer = *temp;
			Buffer++;
			temp++;
		}
	}

	temp = Record->payload_s.buffer;
	/*check for SR bit and then correspondingly use the payload length*/

	for (i_data = 0; i_data < (Record->payload_s.length); i_data++)
	{
		*Buffer = *temp;
		Buffer++;
		temp++;
	}

	return NET_NFC_OK;
}

