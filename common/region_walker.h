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
#ifndef REGION_WALKER_H
#define REGION_WALKER_H

#include <makestuff.h>

struct FlashChip;

/**
 * Given a flash chip, provide some basic infrastructure to give callbacks for
 * each region covered by a given range of addresses. For a programmer this is
 * important - many flash chips can only be erased at a fairly coarse level of
 * granularity like 64KiB. So the idea is you call walkRegions() with the range
 * of addresses you're interested in, and you'll get callbacks on callback()
 * for each region covered.
 */
class RegionWalker {
protected:
	const FlashChip *const m_flashChip;
private:
	// Don't allow copy-ctor or assignment
	RegionWalker(const RegionWalker &other);
	RegionWalker &operator=(const RegionWalker &other);
	
	// Pure virtual callback() function, to be implemented by derived classes.
	virtual void callback(uint32 blockAddress, uint32 bytesUsed) = 0;
public:
	// Public API: construct from a FlashChip, and walk its regions covering a
	// given address range.
	explicit RegionWalker(const FlashChip *flashChip) : m_flashChip(flashChip) { }
	void walkRegions(uint32 dataAddress, uint32 dataLength);
};

#endif
