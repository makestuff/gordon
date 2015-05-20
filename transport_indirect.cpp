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
#include <cstdlib>
#include <libfpgalink.h>
#include "exception.h"
#include "transport_indirect.h"

const uint8 TransportIndirect::selectSuppress = (bmTURBO | bmSUPPRESS | bmFLASHCS);  // 0x07
const uint8 TransportIndirect::selectNoSuppress = (bmTURBO | bmFLASHCS);             // 0x05
const uint8 TransportIndirect::deSelect = bmTURBO;

TransportIndirect::TransportIndirect(FLContext *handle, const char *conduitStr) :
	TransportUSB(handle)
{
	const uint8 conduitNum = (uint8)strtoul(conduitStr, NULL, 10);
	if ( !conduitNum ) {
		throw GordonException("TransportIndirect::TransportIndirect(): Illegal conduit");
	}
	const char *error = 0;
	FLStatus fStatus = flSelectConduit(m_handle, conduitNum, &error);
	checkThrow(fStatus, error);
}

void TransportIndirect::sendMessage(
	const uint8 *cmdData, uint32 cmdLength,
	uint8 *recvBuf, uint32 recvLength) const
{
	const char *error = 0;
	FLStatus fStatus = flWriteChannel(m_handle, 1, 1, &selectSuppress, &error);
	checkThrow(fStatus, error);
	fStatus = flWriteChannel(m_handle, 0, cmdLength, cmdData, &error);
	checkThrow(fStatus, error);
	if ( recvLength ) {
		fStatus = flWriteChannel(m_handle, 1, 1, &selectNoSuppress, &error);
		checkThrow(fStatus, error);
		while ( recvLength > 1024 ) {
			fStatus = flWriteChannel(m_handle, 0, 1024, recvBuf, &error);
			checkThrow(fStatus, error);
			fStatus = flReadChannel(m_handle, 0, 1024, recvBuf, &error);
			checkThrow(fStatus, error);
			recvLength -= 1024;
			recvBuf += 1024;
		}
		fStatus = flWriteChannel(m_handle, 0, recvLength, recvBuf, &error);
		checkThrow(fStatus, error);
		fStatus = flReadChannel(m_handle, 0, recvLength, recvBuf, &error);
		checkThrow(fStatus, error);
	}
	fStatus = flWriteChannel(m_handle, 1, 1, &deSelect, &error);
	checkThrow(fStatus, error);
}
