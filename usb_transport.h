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
#ifndef USB_TRANSPORT_H
#define USB_TRANSPORT_H

#include <transport.h>
#include <libfpgalink.h>

// Base of all Transports based on FPGALink. This is implemented by DirectTransport
// and IndirectTransport and their subclasses.
//
class USBTransport : public Transport {
protected:
	FLContext *const m_handle;
public:
	explicit USBTransport(FLContext *handle) : m_handle(handle) { }
	virtual ~USBTransport() { }

	// Check a return code and throw if necessary
	static void checkThrow(FLStatus status, const char *error);
};

#endif
