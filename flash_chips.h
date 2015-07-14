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
#ifndef FLASH_CHIPS_H
#define FLASH_CHIPS_H

#include <makestuff.h>

// Each flash chip can have a maximum of four regions. This is not the number of
// distinct eraseable address ranges, but the number of range types. So if a
// chip has N ranges of identical length, that uses only one slot.
//
#define NUM_ERASEREGIONS 5

// Forward declarations
//
class Transport;
struct FlashChip;

// Erase function type: erase the region containing the flash address "address".
//
typedef void (*BlockEraseFunc)(
	const FlashChip *flashChip, const Transport *transport, uint32 address
);

// Page-program function type: write "length" bytes (guaranteed fewer than the
// page-length) from the data pointed to by "data" to the flash address
// "address" (which is guaranteed to be page-aligned).
//
typedef void (*PageProgramFunc)(
	const FlashChip *flashChip, const Transport *transport,
	uint32 address, uint32 length, const uint8 *data
);

// Data readback function type: read "length" bytes from address "address" and
// write it to the supplied buffer "buffer".
//
typedef void (*ReadFunc)(
	const FlashChip *flashChip, const Transport *transport,
	uint32 address, uint32 length, uint8 *buffer
);

// Family selector function type. This is used where the configuration of a chip
// affects its parameters, e.g chips whose page length can be configured to be a
// power of two or slightly bigger - it affects the overall capacity. So the
// idea is to have several configs for such chips, and the return value of this
// function provides an index to select between them.
//
typedef uint32 (*SelectorFunc)(
	const Transport *transport
);

// Each region has a size and a count, so a chip split into eight 64KiB chunks
// has just one {64KiB, 8} region.
//
struct EraseRegions {
	uint32 size;
	uint32 count;
};

// Each flash chip gets a descriptor described by this struct, allowing many
// disparate flash chips to be handled by the same API.
//
// This was heavily influenced by the similar struct (and overall architecture)
// of the flashrom project (http://www.flashrom.org).
//
struct FlashChip {
	const char *vendorName;
	const char *deviceName;
	uint32 vendorID;
	uint32 deviceID;
	uint32 kbCapacity;
	uint32 pageSize;
	uint32 bitShift;
	EraseRegions eraseRegions[NUM_ERASEREGIONS];
	BlockEraseFunc blockEraseFunc;
	PageProgramFunc pageProgramFunc;
	ReadFunc readFunc;
	SelectorFunc selectorFunc;
};

// The public API: given a Transport, it queries the attached chip for its JEDEC
// IDs, and finds the coresponding entry in the FlashChip table.
//
const FlashChip *findChip(const Transport *transport);

#endif
