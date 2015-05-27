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
#ifndef TRANSPORT_DIRECT_H
#define TRANSPORT_DIRECT_H

#include "transport_usb.h"

// Programmer implementation using a microcontroller's SPI port. You can
// customise the actual ports used for MISO, MOSI, SCLK and SS, but it assumes
// the SPI is MSB-first though.
//
class TransportDirect : public TransportUSB {
	uint8 m_ssPort;
	uint8 m_ssBit;
public:
	TransportDirect(FLContext *handle, const char *portConfig);
	virtual ~TransportDirect();
	void sendMessage(
		const uint8 *cmdData, uint32 cmdLength = 1,
		uint8 *recvBuf = 0, uint32 recvLength = 0
	) const;
};

#endif
