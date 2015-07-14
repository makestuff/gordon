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
#include <makestuff.h>
#include "exception.h"
#include "flash_chips.h"
#include "region_walker.h"

void RegionWalker::walkRegions(uint32 dataAddress, uint32 dataLength) {
	const EraseRegions *eraseRegions = &m_flashChip->eraseRegions[0];
	uint32 cumulativeAddr = 0;
	uint32 regionGroupsRemaining = NUM_ERASEREGIONS;
	uint32 regionsRemaining = eraseRegions->count;
	uint32 regionSize = eraseRegions->size;
	if ( dataAddress + dataLength > 1024 * m_flashChip->kbCapacity ) {
		char msg[256];
		sprintf(
			msg,
			"RegionWalker::walkRegions(): Address range error! This device's capacity is limited to %u KiB.",
			m_flashChip->kbCapacity
		);
		throw GordonException(msg);
	}
	while ( regionGroupsRemaining && cumulativeAddr < dataAddress ) {
		if ( !regionsRemaining ) {
			regionGroupsRemaining--;
			eraseRegions++;
			regionsRemaining = eraseRegions->count;
			regionSize = eraseRegions->size;
		}
		cumulativeAddr += regionSize;
		regionsRemaining--;
	}
	if ( cumulativeAddr > dataAddress ) {
		char msg[256];
		sprintf(
			msg,
			"RegionWalker::walkRegions(): Address alignment error! The nearest aligned addresses are 0x%08X and 0x%08X.",
			cumulativeAddr - regionSize, cumulativeAddr
		);
		throw GordonException(msg);
	}
	dataAddress += dataLength;
	while ( regionGroupsRemaining && cumulativeAddr < dataAddress ) {
		if ( !regionsRemaining ) {
			regionGroupsRemaining--;
			eraseRegions++;
			regionsRemaining = eraseRegions->count;
			regionSize = eraseRegions->size;
		}
		callback(
			cumulativeAddr,
			(dataLength > regionSize) ? regionSize : dataLength
		);
		cumulativeAddr +=regionSize;
		dataLength -= regionSize;
		regionsRemaining--;
	}
}
