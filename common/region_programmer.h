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
#ifndef REGION_PROGRAMMER_H
#define REGION_PROGRAMMER_H

#include "region_walker.h"

// Forward-declarations
//
class Transport;
struct FlashChip;

// The RegionProgrammer is the core of the flash programmer. It has a couple of
// public methods, one for reading and one for writing. The former just
// delegates to the read callback for the selected chip, but the latter uses the
// RegionWalker infrastructure to ensure that each region covered by the bytes
// to be written is properly erased prior to programming. Thus, the write()
// method calls RegionWalker::walkRegions(), which calls
// RegionProgrammer::callback() for each region requiring erasure. Some devices
// support page-level erasure, which just means the callback() is only expected
// to write one page on each call; other devices have more granular erasure
// regions (e.g 64KiB), so for them, the callback() is expected to erase its
// region and then write many pages.
// 
class RegionProgrammer : public RegionWalker {
	const Transport *m_transport;
	const uint8 *m_dataPtr;
	uint32 m_dotCount;
	void callback(uint32 blockAddress, uint32 bytesUsed);
public:
	explicit RegionProgrammer(const Transport *transport, const FlashChip *thisChip) :
		RegionWalker(thisChip), m_transport(transport)
	{ }

	// Public API: write "length" bytes of data from the supplied array to a
	// given flash byte-address, and read back "length" bytes from a given byte-
	// address into a supplied array.
	void write(uint32 address, uint32 length, const uint8 *data);
	void read(uint32 address, uint32 length, uint8 *buffer);
};

#endif
