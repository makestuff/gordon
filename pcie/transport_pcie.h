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
#ifndef TRANSPORT_PCIE_H
#define TRANSPORT_PCIE_H

#include <string>
#include "transport.h"

class TransportPCIE : public Transport {
	int m_dev;
	enum {
		bmTURBO    = (1<<0),
		bmSUPPRESS = (1<<1),
		bmFLASHCS  = (1<<2)
	};
	static const uint8 selectSuppress;
	static const uint8 selectNoSuppress;
	static const uint8 deSelect;
public:
	TransportPCIE(const char *devNode);
	virtual ~TransportPCIE();
	void sendMessage(
		const uint8 *cmdData, uint32 cmdLength = 1,
		uint8 *recvBuf = 0, uint32 recvLength = 0
	) const;
};

#endif
