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
#ifndef TRANSPORT_INDIRECT_H
#define TRANSPORT_INDIRECT_H

#include "transport.h"

// Transport implementation using an indirect (host->micro->FPGA->flash) link.
// This requires that the FPGA has been programmed with the "spi-talk" design,
// which provides an indirect interface to the SPI flash via the FPGA, using
// a regular FPGALink CommFPGA conduit. It assumes the flash to be accessed is
// on the first of potentially many CS lines from the FPGA.
//
class TransportIndirect : public Transport {
	enum {
		bmTURBO    = (1<<0),
		bmSUPPRESS = (1<<1),
		bmFLASHCS  = (1<<2),
		bmSDCARDCS = (1<<3)
	};
	static const uint8 selectSuppress;
	static const uint8 selectNoSuppress;
	static const uint8 deSelect[];
public:
	TransportIndirect(FLContext *handle, const char *conduit);
	void sendMessage(
		const uint8 *cmdData, size_t cmdLength = 1,
		uint8 *recvBuf = 0, size_t recvLength = 0
	) const;
};

#endif
