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
#include "exception.h"
#include "flash_chips.h"
#include "region_programmer.h"

void RegionProgrammer::callback(uint32 blockAddress, uint32 bytesUsed) {
	const uint32 pageSize = m_flashChip->pageSize;
	const BlockEraseFunc eraseFunc = m_flashChip->blockEraseFunc;
	const PageProgramFunc progFunc = m_flashChip->pageProgramFunc;
	eraseFunc(m_flashChip, m_transport, blockAddress);
	while ( bytesUsed > pageSize ) {
		progFunc(m_flashChip, m_transport, blockAddress, pageSize, m_dataPtr);
		m_dotCount++;
		m_dotCount &= 0x3F;
		printf(m_dotCount ? "." : ".\n");
		fflush(stdout);
		m_dataPtr += pageSize;
		blockAddress += pageSize;
		bytesUsed -= pageSize;
	}
	progFunc(m_flashChip, m_transport, blockAddress, bytesUsed, m_dataPtr);
	m_dotCount++;
	m_dotCount &= 0x3F;
	printf(m_dotCount ? "." : ".\n");
	fflush(stdout);
	m_dataPtr += bytesUsed;
}

void RegionProgrammer::write(uint32 address, uint32 length, const uint8 *data) {
	printf("Writing 0x%08X bytes to address 0x%08X...\n", length, address);
	m_dataPtr = data;
	m_dotCount = 0;
	walkRegions(address, length);
	printf("\n");
}

void RegionProgrammer::read(uint32 address, uint32 length, uint8 *buffer) {
	printf("Reading 0x%08X bytes from address 0x%08X...\n", length, address);
	if ( address + length > 1024 * m_flashChip->kbCapacity ) {
		char msg[256];
		sprintf(
			msg,
			"RegionProgrammer::read(): Requested %u bytes at address %u from a device with a capacity of only %u bytes!",
			length, address, 1024 * m_flashChip->kbCapacity
		);
		throw GordonException(msg);
	}
	m_flashChip->readFunc(m_flashChip, m_transport, address, length, buffer);
}
