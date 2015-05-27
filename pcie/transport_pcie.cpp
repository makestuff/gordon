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
#include <fcntl.h>
#include <unistd.h>
#include <makestuff.h>
#include "transport_pcie.h"
#include "exception.h"
#include "fpgacam.h"

#define SPIDATA     (1*16+0)
#define SPICTRL     (1*16+1)

const uint8 TransportPCIE::selectSuppress = (bmTURBO | bmSUPPRESS | bmFLASHCS);  // 0x07
const uint8 TransportPCIE::selectNoSuppress = (bmTURBO | bmFLASHCS);             // 0x05
const uint8 TransportPCIE::deSelect = bmTURBO;

TransportPCIE::TransportPCIE(const char *devNode) : m_dev(0) {
	const int dev = open(devNode, O_RDWR|O_SYNC);
	if ( dev < 0 ) {
		throw GordonException(
			"Failed to open device node. Did you forget to install the driver?",
			dev
		);
	}
	m_dev = dev;
}

TransportPCIE::~TransportPCIE() {
	if ( m_dev ) {
		close(m_dev);
	}
}

void TransportPCIE::sendMessage(
	const uint8 *cmdData, uint32 cmdLength,
	uint8 *recvBuf, uint32 recvLength) const
{
	struct Cmd wrCmd[] = {
		WR(SPICTRL, 0)
	};
	struct Cmd rdCmd[] = {
		RD(SPIDATA)
	};
	//struct Cmd datwCmd[] = {
	//	WR(SPIDATA, 0), WR(SPIDATA, 0), WR(SPIDATA, 0), WR(SPIDATA, 0),
	//	WR(SPIDATA, 0), WR(SPIDATA, 0), WR(SPIDATA, 0), WR(SPIDATA, 0),
	//	WR(SPIDATA, 0), WR(SPIDATA, 0), WR(SPIDATA, 0), WR(SPIDATA, 0),
	//	WR(SPIDATA, 0), WR(SPIDATA, 0), WR(SPIDATA, 0), WR(SPIDATA, 0)
	//};
	//struct Cmd datrCmd[] = {
	//	RD(SPIDATA), RD(SPIDATA), RD(SPIDATA), RD(SPIDATA),
	//	RD(SPIDATA), RD(SPIDATA), RD(SPIDATA), RD(SPIDATA),
	//	RD(SPIDATA), RD(SPIDATA), RD(SPIDATA), RD(SPIDATA),
	//	RD(SPIDATA), RD(SPIDATA), RD(SPIDATA), RD(SPIDATA)
	//};
	uint32 i;

	// Configure SPI
	wrCmd[0].reg = SPICTRL;
	wrCmd[0].val = deSelect;  // just in case it was left in a bad state
	fcCmdList(m_dev, wrCmd);
	wrCmd[0].val = selectSuppress;  // suppress responses whilst we're sending command bytes
	fcCmdList(m_dev, wrCmd);

	// Send command bytes
	wrCmd[0].reg = SPIDATA;
	for ( i = 0; i < cmdLength; i++ ) {
		wrCmd[0].val = (uint32)cmdData[i];
		fcCmdList(m_dev, wrCmd);
	}

	// Maybe get response
	if ( recvLength ) {
		wrCmd[0].reg = SPICTRL;
		wrCmd[0].val = selectNoSuppress;  // stop suppressing responses
		fcCmdList(m_dev, wrCmd);
		wrCmd[0].reg = SPIDATA;
		wrCmd[0].val = 0xFF;
		while ( recvLength-- ) {
			fcCmdList(m_dev, wrCmd);
			fcCmdList(m_dev, rdCmd);
			*recvBuf++ = (uint8)rdCmd[0].val;
		}
	}

	// Deassert CS
	wrCmd[0].reg = SPICTRL;
	wrCmd[0].val = deSelect;
	fcCmdList(m_dev, wrCmd);
}
