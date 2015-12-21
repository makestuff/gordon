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
#include <libfpgalink.h>
#include "transport_direct.h"

TransportDirect::TransportDirect(FLContext *handle, const char *portConfig) :
	TransportUSB(handle)
{
	const char *error = 0;
	FLStatus fStatus = flSelectConduit(m_handle, 0, &error);
	checkThrow(fStatus, error);
	fStatus = progOpen(m_handle, portConfig, &error);
	checkThrow(fStatus, error);
	m_ssPort = progGetPort(m_handle, LP_SS);
	m_ssBit = progGetBit(m_handle, LP_SS);
	fStatus = flSingleBitPortAccess(m_handle, m_ssPort, m_ssBit, PIN_HIGH, NULL, &error);
	checkThrow(fStatus, error);
}

TransportDirect::~TransportDirect() {
	FLStatus fStatus = progClose(m_handle, NULL);
	(void)fStatus;
}

void TransportDirect::sendMessage(
	const uint8 *cmdData, uint32 cmdLength,
	uint8 *recvBuf, uint32 recvLength) const
{
	const char *error = 0;
	FLStatus fStatus = flSingleBitPortAccess(m_handle, m_ssPort, m_ssBit, PIN_LOW, NULL, &error);
	checkThrow(fStatus, error);
	fStatus = spiSend(m_handle, cmdLength, cmdData, SPI_MSBFIRST, &error);
	checkThrow(fStatus, error);
	if ( recvLength ) {
		fStatus = spiRecv(m_handle, recvLength, recvBuf, SPI_MSBFIRST, &error);
		checkThrow(fStatus, error);
	}
	fStatus = flSingleBitPortAccess(m_handle, m_ssPort, m_ssBit, PIN_HIGH, NULL, &error);
	checkThrow(fStatus, error);
}
