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
#include "transport_iceblink.h"

TransportIceBlink::TransportIceBlink(FLContext *handle) :
	TransportDirect(handle, "B3B2B0B1")
{
	// POWER  = PC2
	// SS     = PB0
	// SCK    = PB1
	// MOSI   = PB2
	// MISO   = PB3
	// CRESET = PB6
	//
	const char *error = 0;
	FLStatus fStatus = flMultiBitPortAccess(m_handle, "B6-,C2-,B0-", NULL, &error); // CRESET, POWER & SS low
	Transport::checkThrow(fStatus, error);
	flSleep(10);
	fStatus = flSingleBitPortAccess(m_handle, 2, 2, PIN_HIGH, NULL, &error);  // POWER(C2) high
	Transport::checkThrow(fStatus, error);
	flSleep(10);
	fStatus = flMultiBitPortAccess(m_handle, "B1-,B2-,B0+", NULL, &error); // SCK & MOSI low, SS high
	Transport::checkThrow(fStatus, error);
}

TransportIceBlink::~TransportIceBlink() {
	FLStatus fStatus;
	fStatus = flMultiBitPortAccess(m_handle, "B0?,B1?,B2?,B6?", NULL, NULL); // SCK, MOSI & CRESET inputs
	(void)fStatus;
}
