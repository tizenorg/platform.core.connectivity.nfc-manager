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


#ifndef __NET_NFC_UTIL_NDEF_PARSER__
#define __NET_NFC_UTIL_NDEF_PARSER__

#include "net_nfc_typedef_private.h"

/**
 * \brief These are the flags specifying the content, structure or purpose of a NDEF Record.
 * \name NDEF Record Header Flags
 *
 * Flags of the first record byte, as defined by the NDEF specification.
 *
 */
/*@{*/
#define SLP_FRINET_NFC_NDEFRECORD_FLAGS_MB       ((uint8_t)0x80)  /**< This marks the begin of a NDEF Message. */
#define SLP_FRINET_NFC_NDEFRECORD_FLAGS_ME       ((uint8_t)0x40)  /**< Set if the record is at the Message End. */
#define SLP_FRINET_NFC_NDEFRECORD_FLAGS_CF       ((uint8_t)0x20)  /**< Chunk Flag: The record is a record chunk only. */
#define SLP_FRINET_NFC_NDEFRECORD_FLAGS_SR       ((uint8_t)0x10)  /**< Short Record: Payload Length is encoded in ONE byte only. */
#define SLP_FRINET_NFC_NDEFRECORD_FLAGS_IL       ((uint8_t)0x08)  /**< The ID Length Field is present. */
/*@}*/

/* Internal:
 * NDEF Record #defines for constant value
 */
#define SLPFRINFCNDEFRECORD_CHUNKBIT_SET         1               /** \internal Chunk Bit is set. */
#define SLPFRINFCNDEFRECORD_CHUNKBIT_SET_ZERO    0  				/** \internal Chunk Bit is not set. */
#define SLPNFCSTSHL8                          	8
#define SLPNFCSTSHL16                            16              /** \internal Shift 16 bits(left or right). */
#define SLPNFCSTSHL24                            24              /** \internal Shift 24 bits(left or right). */
#define SLPFRINFCNDEFRECORD_NORMAL_RECORD_BYTE   4               /** \internal Normal record. */
#define SLP_FRINET_NFC_NDEFRECORD_TNFBYTE_MASK       ((uint8_t)0x07) /** \internal For masking */
#define SLP_FRINET_NFC_NDEFRECORD_BUF_INC1           1               /** \internal Increment Buffer Address by 1 */
#define SLP_FRINET_NFC_NDEFRECORD_BUF_INC2           2               /** \internal Increment Buffer Address by 2 */
#define SLP_FRINET_NFC_NDEFRECORD_BUF_INC3           3               /** \internal Increment Buffer Address by 3 */
#define SLP_FRINET_NFC_NDEFRECORD_BUF_INC4           4               /** \internal Increment Buffer Address by 4 */
#define SLP_FRINET_NFC_NDEFRECORD_BUF_INC5           5               /** \internal Increment Buffer Address by 5 */
#define SLP_FRINET_NFC_NDEFRECORD_BUF_TNF_VALUE      ((uint8_t)0x00) /** \internal If TNF = Empty, Unknown and Unchanged, the id, type and payload length is ZERO  */
#define SLP_FRINET_NFC_NDEFRECORD_FLAG_MASK          ((uint8_t)0xF8) /** \internal To Mask the Flag Byte */


#define NET_NFC_NDEF_TNF_EMPTY        ((uint8_t)0x00)  /**< Empty Record, no type, ID or payload present. */
#define NET_NFC_NDEF_TNF_NFCWELLKNOWN ((uint8_t)0x01)  /**< NFC well-known type (RTD). */
#define NET_NFC_NDEF_TNF_MEDIATYPE    ((uint8_t)0x02)  /**< Media Type. */
#define NET_NFC_NDEF_TNF_ABSURI       ((uint8_t)0x03)  /**< Absolute URI. */
#define NET_NFC_NDEF_TNF_NFCEXT       ((uint8_t)0x04)  /**< Nfc External Type (following the RTD format). */
#define NET_NFC_NDEF_TNF_UNKNOWN      ((uint8_t)0x05)  /**< Unknown type; Contains no Type information. */
#define NET_NFC_NDEF_TNF_UNCHANGED    ((uint8_t)0x06)  /**< Unchanged: Used for Chunked Records. */
#define NET_NFC_NDEF_TNF_RESERVED     ((uint8_t)0x07)  /**< RFU, must not be used. */

void __phFriNfc_NdefRecord_RecordFlag(uint8_t* rawData, ndef_record_s* record);
uint8_t __phFriNfc_NdefRecord_TypeNameFormat(uint8_t *rawData, ndef_record_s* record);
net_nfc_error_e __phFriNfc_NdefRecord_RecordIDCheck(uint8_t *rawData,
	uint8_t *TypeLength,
	uint8_t *TypeLengthByte,
	uint8_t *PayloadLengthByte,
	uint32_t *PayloadLength,
	uint8_t *IDLengthByte,
	uint8_t *IDLength,
	ndef_record_s *record);
net_nfc_error_e __phFriNfc_NdefRecord_Parse(ndef_record_s*Record, uint8_t *RawRecord, int *readData);
uint8_t __phFriNfc_NdefRecord_GenFlag(ndef_record_s* record);
net_nfc_error_e __phFriNfc_NdefRecord_Generate(ndef_record_s*Record,
	uint8_t *Buffer,
	uint32_t MaxBufferSize,
	uint32_t *BytesWritten);

#endif

