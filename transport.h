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
#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <makestuff.h>
#include <libfpgalink.h>

// This describes the contract to be implemented by all connections to an SPI
// flash chip. It basically accepts an FPGALink context and provides a pure
// virtual sendMessage() function for talking to the chip. This is implemented
// by DirectTransport and IndirectTransport and their subclasses.
//
class Transport {
	// Disallow copy-ctor and assignment.
	Transport(const Transport &other);
	Transport &operator=(const Transport &other);
protected:
	FLContext *const m_handle;
public:
	explicit Transport(FLContext *handle) : m_handle(handle) { }
	virtual ~Transport() { }

	// Public API: send some bytes to the flash, and read some bytes back.
	virtual void sendMessage(
		const uint8 *cmdData, uint32 cmdLength = 1,
		uint8 *recvBuf = 0, uint32 recvLength = 0
	) const = 0;
	static void checkThrow(FLStatus status, const char *error);
};

#endif
