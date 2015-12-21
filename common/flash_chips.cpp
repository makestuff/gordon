/* 
 * Copyright (C) 2013 Chris McClelland
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */
#include <cstdio>
#include <cstring>
#include "transport.h"
#include "exception.h"
#include "flash_chips.h"

#define MAX_PAGESIZE 528
#define BM_WIP 0x01
#define BM_POWER2 0x01
#define BM_READY 0x80

// Block erasers
static void spiBlockEraseNull(
	const FlashChip *flashChip, const Transport *transport, uint32 address)
{
	(void)flashChip;
	(void)transport;
	(void)address;
}
static void spiBlockEraseD8(
	const FlashChip *flashChip, const Transport *transport, uint32 address)
{
	const uint32 pageNum = (uint32)(address / flashChip->pageSize);
	const uint32 flashAddress = pageNum << flashChip->bitShift; // pageOffset guaranteed to be zero
	const uint8 writeEnable = 0x06; // write enable
	const uint8 readStatus = 0x05; // read status
	uint8 status;
	const uint8 eraseCommand[] = {
		0xD8,  // erase block
		(uint8)(flashAddress >> 16),
		(uint8)(flashAddress >> 8),
		(uint8)flashAddress
	};
	transport->sendMessage(&writeEnable, 1);
	transport->sendMessage(eraseCommand, 4);
	do {
		transport->sendMessage(&readStatus, 1, &status, 1);
	} while ( status & BM_WIP );
}

// Page programmers
static void spiPageProgram02(
	const FlashChip *flashChip, const Transport *transport,
	uint32 address, uint32 length, const uint8 *data)
{
	const uint32 pageNum = (uint32)(address / flashChip->pageSize);
	const uint32 flashAddress = pageNum << flashChip->bitShift; // pageOffset guaranteed to be zero
	const uint8 writeEnable = 0x06; // write enable
	const uint8 readStatus = 0x05; // read status
	uint8 writeCommand[MAX_PAGESIZE + 4];
	uint8 status;
	uint32 i;
	writeCommand[0] = 0x02;  // page program
	writeCommand[1] = (uint8)(flashAddress >> 16);
	writeCommand[2] = (uint8)(flashAddress >> 8);
	writeCommand[3] = (uint8)flashAddress;
	memcpy(writeCommand + 4, data, length);
	for ( i = length; i < flashChip->pageSize; i++ ) {
		*(writeCommand + 4 + i) = 0xFF;
	}
	transport->sendMessage(&writeEnable, 1);
	transport->sendMessage(writeCommand, flashChip->pageSize + 4);
	do {
		transport->sendMessage(&readStatus, 1, &status, 1);
	} while ( status & BM_WIP );
}

static void spiPageProgram82(
	const FlashChip *flashChip, const Transport *transport,
	uint32 address, uint32 length, const uint8 *data)
{
	const uint32 pageNum = (uint32)(address / flashChip->pageSize);
	const uint32 flashAddress = pageNum << flashChip->bitShift; // pageOffset guaranteed to be zero
	const uint8 readStatus = 0xD7; // read status
	uint8 writeCommand[MAX_PAGESIZE + 4];
	uint8 status;
	uint32 i;
	writeCommand[0] = 0x82;  // page program
	writeCommand[1] = (uint8)(flashAddress >> 16);
	writeCommand[2] = (uint8)(flashAddress >> 8);
	writeCommand[3] = (uint8)flashAddress;
	memcpy(writeCommand + 4, data, length);
	for ( i = length; i < flashChip->pageSize; i++ ) {
		*(writeCommand + 4 + i) = 0xFF;
	}
	transport->sendMessage(writeCommand, flashChip->pageSize + 4);
	do {
		transport->sendMessage(&readStatus, 1, &status, 1);
	} while ( !(status & BM_READY) );
}

// Readers
static void spiRead03(
	const FlashChip *flashChip, const Transport *transport,
	uint32 address, uint32 length, uint8 *buffer)
{
	const uint32 pageNum = (uint32)(address / flashChip->pageSize);
	const uint32 pageOffset = (uint32)(address % flashChip->pageSize);
	const uint32 flashAddress = (pageNum << flashChip->bitShift) | pageOffset;
	const uint8 readCommand[] = {
		0x03,  // read flash
		(uint8)(flashAddress >> 16),
		(uint8)(flashAddress >> 8),
		(uint8)flashAddress
	};
	transport->sendMessage(readCommand, 4, buffer, length);
}

// Selectors
static uint32 nullSelector(const Transport *transport) {
	(void)transport;
	return 0;
}
static uint32 powerTwoSelector(const Transport *transport) {
	const uint8 readStatus = 0xD7; // "read status" opcode
	uint8 status;
	transport->sendMessage(&readStatus, 1, &status, 1);
	return (status & BM_POWER2) ? 1 : 0;
}

#define ST_ID 0x20
#define AMIC_ID 0x7F37
#define ATMEL_ID 0x1F
#define WINBOND_NEX_ID 0xEF  /* Winbond (ex Nexcom) serial flashes */

#define ST_M25P10 0x2011
#define ST_M25P40 0x2013
#define ST_N25Q128 0xBA18
#define AMIC_A25L05PT 0x2020
#define AMIC_A25L40PT 0x2013
#define ATMEL_AT45DB041D 0x2400
#define ATMEL_AT45DB161D 0x2600
#define WINBOND_NEX_W25Q64_V 0x4017

static const FlashChip flashChips[] = {
	{
		"AMIC",
		"A25L05PT",
		AMIC_ID,
		AMIC_A25L05PT,
		64,  // device size in KiB
		256,  // page size in bytes
		8,
		{
			{32 * 1024, 1},
			{16 * 1024, 1},
			{8 * 1024, 1},
			{4 * 1024, 2}
		},
		spiBlockEraseD8,
		spiPageProgram02,
		spiRead03,
		nullSelector
	}, {
		"AMIC",
		"A25L40PT",
		AMIC_ID,
		AMIC_A25L40PT,
		512,  // device size in KiB
		256,  // page size in bytes
		8,
		{
			{64 * 1024, 7},
			{32 * 1024, 1},
			{16 * 1024, 1},
			{8 * 1024, 1},
			{4 * 1024, 2},
		},
		spiBlockEraseD8,
		spiPageProgram02,
		spiRead03,
		nullSelector
	}, {
		"Micron/Numonyx/ST",
		"M25P10",
		ST_ID,
		ST_M25P10,
		128,  // device size in KiB
		256,  // page size in bytes
		8,
		{
			{32 * 1024, 4}  // block size, num blocks
		},
		spiBlockEraseD8,
		spiPageProgram02,
		spiRead03,
		nullSelector
	}, {
		"Micron/Numonyx/ST",
		"M25P40",
		ST_ID,
		ST_M25P40,
		512,  // device size in KiB
		256,  // page size in bytes
		8,
		{
			{64 * 1024, 8}  // block size, num blocks
		},
		spiBlockEraseD8,
		spiPageProgram02,
		spiRead03,
		nullSelector
	}, {
		"Micron/Numonyx/ST",
		"N25Q128",
		ST_ID,
		ST_N25Q128,
		16384,  // device size in KiB
		256,    // page size in bytes
		8,
		{
			{64 * 1024, 256}  // block size, num blocks
		},
		spiBlockEraseD8,
		spiPageProgram02,
		spiRead03,
		nullSelector
	}, {
		"Atmel",
		"AT45DB041D",
		ATMEL_ID,
		ATMEL_AT45DB041D,
		528,  // device size in KiB
		264,  // page size in bytes
		9,
		{
			{264, 2048}  // block size, num blocks
		},
		spiBlockEraseNull,
		spiPageProgram82,
		spiRead03,
		powerTwoSelector
	}, {
		"Atmel",
		"AT45DB041D",
		ATMEL_ID,
		ATMEL_AT45DB041D,
		512, // device size in KiB
		256,  // page size in bytes
		9,
		{
			{264, 2048}  // block size, num blocks
		},
		spiBlockEraseNull,
		spiPageProgram82,
		spiRead03,
		NULL
	}, {
		"Atmel",
		"AT45DB161D",
		ATMEL_ID,
		ATMEL_AT45DB161D,
		2112, // device size in KiB
		528,  // page size in bytes
		10,
		{
			{528, 4096}  // block size, num blocks
		},
		spiBlockEraseNull,
		spiPageProgram82,
		spiRead03,
		powerTwoSelector
	}, {
		"Atmel",
		"AT45DB161D",
		ATMEL_ID,
		ATMEL_AT45DB161D,
		2048, // device size in KiB
		512,  // page size in bytes
		9,
		{
			{512, 4096}  // block size, num blocks
		},
		spiBlockEraseNull,
		spiPageProgram82,
		spiRead03,
		NULL
	}, {
		"Winbond",
		"W25Q64.V",
		WINBOND_NEX_ID,
		WINBOND_NEX_W25Q64_V,
		8192,  // device size in KiB
		256,  // page size in bytes
		8,
		{
			{64 * 1024, 128}  // block size, num blocks
		},
		spiBlockEraseD8,
		spiPageProgram02,
		spiRead03,
		nullSelector
	}, {
		NULL, NULL, 0, 0, 0, 0, 0, {{0, 0}}, NULL, NULL, NULL, NULL
	}
};

const FlashChip *findChip(const Transport *transport) {
	const uint8 readIdent = 0x9F;  // JEDEC ID command
	const FlashChip *thisChip = flashChips;
	uint8 buf[256];
	const uint8 *ptr = buf;
	uint32 vendorID = 0;
	uint32 deviceID;
	transport->sendMessage(&readIdent, 1, buf, 256);
	while ( *ptr == 0x7F ) {
		vendorID |= 0x7F;
		vendorID <<= 8;
		ptr++;
	}
	vendorID |= *ptr++;
	deviceID = (ptr[0] << 8) | ptr[1];
	while ( thisChip->deviceName && (thisChip->vendorID != vendorID || thisChip->deviceID != deviceID) ) {
		thisChip++;
	}
	if ( thisChip->vendorID == vendorID && thisChip->deviceID == deviceID ) {
		thisChip += thisChip->selectorFunc(transport);
		return thisChip;
	} else {
		char msg[256];
		sprintf(msg, "findChip(): Unknown device: vendorID = 0x%08X, deviceID = 0x%04X", vendorID, deviceID);
		throw GordonException(msg);
	}
}
