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
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <climits>
#include <vector>
#include <makestuff.h>
#include <libfpgalink.h>
#include <liberror.h>
#include <argtable2.h>
#include <libdump.h>

// ---- Definitions --------------------------------------------------------------------------------

// Exception class for application
//
class GordonException : public std::exception {
	const std::string m_msg;
	const int m_retVal;
public:
	explicit GordonException(const char *msg, int retVal = -1, bool cleanup = false) :
		m_msg(msg), m_retVal(retVal)
	{
		if ( cleanup ) {
			flFreeError(msg);
		}
	}
	explicit GordonException(const std::string &msg, int retVal = -1) :
		m_msg(msg), m_retVal(retVal)
	{ }
	virtual ~GordonException() throw() { }
	virtual const char *what() const throw() { return m_msg.c_str(); }
};

// Pure virtual base class
//
class Gordon {
	const char *const m_vp;
	struct FLContext *m_handle;
public:
	explicit Gordon(const char *vp);
	virtual ~Gordon();
	virtual void sendMessage(
		const uint8 *cmdData, size_t cmdLength = 1,
		uint8 *recvBuf = 0, size_t recvLength = 0
	) const = 0;
	FLContext *getHandle() const { return m_handle; }
	static void checkThrow(FLStatus status, const char *error);
};

// Programmer implementation using a micro SPI port
//
class GordonSPI : public Gordon {
	const BitOrder m_bitOrder;
	uint8 m_ssPort;
	uint8 m_ssBit;
public:
	GordonSPI(const char *vp, const char *portConfig, BitOrder bitOrder);
	virtual ~GordonSPI();
	void sendMessage(
		const uint8 *cmdData, size_t cmdLength = 1,
		uint8 *recvBuf = 0, size_t recvLength = 0
	) const;
};

// Programmer implementation based on GordonSPI, specifically for IceBlink board
//
class GordonIceBlink : public GordonSPI {
public:
	GordonIceBlink(const char *vp);
	virtual ~GordonIceBlink();
};

// Programmer implementation using an indirect (host->micro->FPGA->flash) link
//
class GordonIndirect : public Gordon {
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
	GordonIndirect(const char *vp, uint8 conduit);
	void sendMessage(
		const uint8 *cmdData, size_t cmdLength = 1,
		uint8 *recvBuf = 0, size_t recvLength = 0
	) const;
};

class SPIFlash;

// Programmer interface
//
class Programmer {
public:
	virtual void write(const SPIFlash *sf, size_t offset, const uint8 *data, size_t length) const = 0;
	virtual void read(const SPIFlash *sf, size_t offset, std::vector<uint8> &buf, size_t length) const = 0;
};

// Programming using command code 0x02, with 0x06 for write enable, etc.
//
class Prog02 : public Programmer {
	Prog02() { }
	Prog02(const Prog02 &other);
	Prog02 &operator=(const Prog02 &other);
public:
	static const Prog02 INSTANCE;
	void write(const SPIFlash *sf, size_t offset, const uint8 *data, size_t length) const;
	void read(const SPIFlash *sf, size_t offset, std::vector<uint8> &buf, size_t length) const;
};
const Prog02 Prog02::INSTANCE;

// Programmer for devices using an intermediate RAM buffer.
//
class Prog82 : public Programmer {
	Prog82() { }
	Prog82(const Prog82 &other);
	Prog82 &operator=(const Prog82 &other);
public:
	static const Prog82 INSTANCE;
	void write(const SPIFlash *sf, size_t offset, const uint8 *data, size_t length) const;
	void read(const SPIFlash *sf, size_t offset, std::vector<uint8> &buf, size_t length) const;
};
const Prog82 Prog82::INSTANCE;

struct Device {
	const char *const name;
	const size_t capacity;
	const size_t capacity2;
	const uint32 pageSize;
	const uint32 pageSize2;
	const uint32 bitShift;
	const uint32 bitShift2;
	const Programmer *programmer;
	const uint16 id;
};
struct Manufacturer {
	const char *const name;
	const uint8 id;
	const Device *const devices;
};

class SPIFlash {
	const Gordon *const m_gord;
	const Manufacturer *m_manufacturer;
	const Device *m_device;
public:
	SPIFlash(const Gordon *gord);
	const char *getManufacturerName() const { return m_manufacturer->name; }
	const char *getDeviceName() const { return m_device->name; }
	size_t getCapacity() const { return m_device->capacity; }
	size_t getCapacity2() const { return m_device->capacity2; }
	uint32 getPageSize() const { return m_device->pageSize; }
	uint32 getPageSize2() const { return m_device->pageSize2; }
	uint32 getBitShift() const { return m_device->bitShift; }
	uint32 getBitShift2() const { return m_device->bitShift2; }
	void sendMessage(
		const uint8 *cmdData, size_t cmdLength = 1,
		uint8 *recvBuf = 0, size_t recvLength = 0
	) const { m_gord->sendMessage(cmdData, cmdLength, recvBuf, recvLength); }
	void write(size_t offset, const uint8 *data, size_t length) const {
		m_device->programmer->write(this, offset, data, length);
	}
	void read(size_t offset, std::vector<uint8> &buf, size_t length = 0) const {
		m_device->programmer->read(this, offset, buf, length);
	}
};

// ---- Implementations ----------------------------------------------------------------------------
using namespace std;

Gordon::Gordon(const char *vp) : m_vp(vp), m_handle(0) {
	const char *error = 0;
	FLStatus fStatus = flOpen(m_vp, &m_handle, &error);
	checkThrow(fStatus, error);
}
Gordon::~Gordon() {
	flClose(m_handle);
}

void Gordon::checkThrow(FLStatus status, const char *error) {
	if ( status ) {
		throw GordonException(error, status, true);
	}
}

GordonSPI::GordonSPI(const char *vp, const char *portConfig, BitOrder bitOrder) :
	Gordon(vp), m_bitOrder(bitOrder)
{
	const char *error = 0;
	FLStatus fStatus = flSelectConduit(getHandle(), 0, &error);
	Gordon::checkThrow(fStatus, error);
	fStatus = progOpen(getHandle(), portConfig, &error);
	Gordon::checkThrow(fStatus, error);
	m_ssPort = progGetPort(getHandle(), LP_SS);
	m_ssBit = progGetBit(getHandle(), LP_SS);
	fStatus = flSingleBitPortAccess(getHandle(), m_ssPort, m_ssBit, PIN_HIGH, NULL, &error);
	checkThrow(fStatus, error);
}

GordonSPI::~GordonSPI() {
	cout << "Closing!" << endl;
	FLStatus fStatus = progClose(getHandle(), NULL);
	(void)fStatus;
}

void GordonSPI::sendMessage(
	const uint8 *cmdData, size_t cmdLength,
	uint8 *recvBuf, size_t recvLength) const
{
	const char *error = 0;
	FLStatus fStatus = flSingleBitPortAccess(getHandle(), m_ssPort, m_ssBit, PIN_LOW, NULL, &error);
	checkThrow(fStatus, error);
	fStatus = spiSend(getHandle(), cmdData, (uint32)cmdLength, m_bitOrder, &error);
	checkThrow(fStatus, error);
	if ( recvLength ) {
		fStatus = spiRecv(getHandle(), recvBuf, (uint32)recvLength, m_bitOrder, &error);
		checkThrow(fStatus, error);
	}
	fStatus = flSingleBitPortAccess(getHandle(), m_ssPort, m_ssBit, PIN_HIGH, NULL, &error);
	checkThrow(fStatus, error);
}

GordonIceBlink::GordonIceBlink(const char *vp) :
	GordonSPI(vp, "B3B2B0B1", SPI_MSBFIRST)
{
	// POWER  = PC2
	// SS     = PB0
	// SCK    = PB1
	// MOSI   = PB2
	// MISO   = PB3
	// CRESET = PB6
	//
	const char *error = 0;
	FLStatus fStatus = flMultiBitPortAccess(getHandle(), "B6-,C2-,B0-", NULL, &error); // CRESET, POWER & SS low
	Gordon::checkThrow(fStatus, error);
	flSleep(10);
	fStatus = flSingleBitPortAccess(getHandle(), 2, 2, PIN_HIGH, NULL, &error);  // POWER(C2) high
	Gordon::checkThrow(fStatus, error);
	flSleep(10);
	fStatus = flMultiBitPortAccess(getHandle(), "B1-,B2-,B0+", NULL, &error); // SCK & MOSI low, SS high
	Gordon::checkThrow(fStatus, error);
}

GordonIceBlink::~GordonIceBlink() {
	FLStatus fStatus;
	fStatus = flMultiBitPortAccess(getHandle(), "B0?,B1?,B2?,B6?", NULL, NULL); // SCK, MOSI & CRESET inputs
	(void)fStatus;
}

const uint8 GordonIndirect::selectSuppress = (bmTURBO | bmSUPPRESS | bmFLASHCS);  // 0x07
const uint8 GordonIndirect::selectNoSuppress = (bmTURBO | bmFLASHCS);             // 0x05
const uint8 GordonIndirect::deSelect[] = {
	(bmTURBO | bmSUPPRESS | bmFLASHCS), (bmTURBO | bmSUPPRESS | bmFLASHCS),
	(bmTURBO | bmSUPPRESS | bmFLASHCS), (bmTURBO | bmSUPPRESS | bmFLASHCS),
	(bmTURBO | bmSUPPRESS | bmFLASHCS), (bmTURBO | bmSUPPRESS | bmFLASHCS),
	(bmTURBO | bmSUPPRESS | bmFLASHCS), (bmTURBO | bmSUPPRESS | bmFLASHCS),
	bmTURBO
};

GordonIndirect::GordonIndirect(const char *vp, uint8 conduit) : Gordon(vp) {
	const char *error = 0;
	FLStatus fStatus = flSelectConduit(getHandle(), conduit, &error);
	Gordon::checkThrow(fStatus, error);
}

void GordonIndirect::sendMessage(
	const uint8 *cmdData, size_t cmdLength,
	uint8 *recvBuf, size_t recvLength) const
{
	const char *error = 0;
	FLStatus fStatus = flWriteChannel(getHandle(), 1, 1, &selectSuppress, &error);
	checkThrow(fStatus, error);
	fStatus = flWriteChannel(getHandle(), 0, (uint32)cmdLength, cmdData, &error);
	checkThrow(fStatus, error);
	if ( recvLength ) {
		fStatus = flWriteChannel(getHandle(), 1, 1, &selectNoSuppress, &error);
		checkThrow(fStatus, error);
		while ( recvLength > 1024 ) {
			fStatus = flWriteChannel(getHandle(), 0, 1024, recvBuf, &error);
			checkThrow(fStatus, error);
			fStatus = flReadChannel(getHandle(), 0, 1024, recvBuf, &error);
			checkThrow(fStatus, error);
			recvLength -= 1024;
			recvBuf += 1024;
		}
		fStatus = flWriteChannel(getHandle(), 0, (uint32)recvLength, recvBuf, &error);
		checkThrow(fStatus, error);
		fStatus = flReadChannel(getHandle(), 0, (uint32)recvLength, recvBuf, &error);
		checkThrow(fStatus, error);
	}
	fStatus = flWriteChannel(getHandle(), 1, sizeof(deSelect), deSelect, &error);
	checkThrow(fStatus, error);
}

/*static size_t numBits(size_t pageSize) {
	size_t power = 0;
	while ( pageSize > (1 << power) ) {
		power++;
	}
	return power;
}
*/

void Prog02::read(const SPIFlash *sf, size_t offset, vector<uint8> &buf, size_t length) const {
	const uint32 pageSize = sf->getPageSize2();
	const uint32 bitShift = sf->getBitShift2();
	const size_t capacity = sf->getCapacity2();
	const uint32 pageNum = (uint32)(offset / pageSize);
	const uint32 pageOffset = (uint32)(offset % pageSize);
	const uint32 address = (pageNum << bitShift) | pageOffset; 
	const uint8 command[] = {
		0x03,
		(uint8)(address >> 16),
		(uint8)(address >> 8),
		(uint8)address
	};
	if ( length == 0 ) {
		length = capacity;
	} else if ( length > capacity ) {
		stringstream msg;
		msg << "Requested " << length << " bytes from a device with a capacity of only " << capacity << " bytes";
		throw GordonException(msg.str());
	}
	buf.resize(length);
	sf->sendMessage(command, 4, &buf.front(), length);
}

void Prog02::write(const SPIFlash *sf, size_t offset, const uint8 *data, size_t length) const {
	(void)sf;
	(void)offset;
	(void)data;
	(void)length;
	cout << "Prog02::write()" << endl;
}

#define BM_POWER2 0x01
#define BM_READY 0x80

void Prog82::read(const SPIFlash *sf, size_t offset, vector<uint8> &buf, size_t length = 0) const {
	const uint8 readStatus = 0xD7; // "read status" opcode
	uint8 status;
	sf->sendMessage(&readStatus, 1, &status, 1);
	uint32 pageSize;
	uint32 bitShift;
	size_t capacity;
	if ( status & BM_POWER2 ) {
		pageSize = sf->getPageSize2();
		bitShift = sf->getBitShift2();
		capacity = sf->getCapacity2();
	} else {
		pageSize = sf->getPageSize();
		bitShift = sf->getBitShift();
		capacity = sf->getCapacity();
	}
	const uint32 pageNum = (uint32)(offset / pageSize);
	const uint32 pageOffset = (uint32)(offset % pageSize);
	const uint32 address = (pageNum << bitShift) | pageOffset; 
	const uint8 command[] = {
		0x03,
		(uint8)(address >> 16),
		(uint8)(address >> 8),
		(uint8)address
	};
	if ( length == 0 ) {
		length = capacity;
	} else if ( length > capacity ) {
		stringstream msg;
		msg << "Requested " << length << " bytes from a device with a capacity of only " << capacity << " bytes";
		throw GordonException(msg.str());
	}
	buf.resize(length);
	sf->sendMessage(command, 4, &buf.front(), length);
}

void Prog82::write(const SPIFlash *sf, size_t offset, const uint8 *data, size_t length) const {
	const uint8 readStatus = 0xD7; // "read status" opcode
	uint8 status;
	sf->sendMessage(&readStatus, 1, &status, 1);
	uint32 pageSize;
	uint32 bitShift;
	size_t capacity;
	if ( status & BM_POWER2 ) {
		pageSize = sf->getPageSize2();
		bitShift = sf->getBitShift2();
		capacity = sf->getCapacity2();
	} else {
		pageSize = sf->getPageSize();
		bitShift = sf->getBitShift();
		capacity = sf->getCapacity();
	}
	uint32 pageNum = (uint32)(offset / pageSize);
	const uint32 startOffset = (uint32)(offset % pageSize);
	const size_t endByte = offset + length;
	const uint32 endPage = (uint32)(endByte / pageSize);
	const uint32 endOffset = (uint32)(endByte % pageSize);
	uint32 address;
	uint32 count;
	uint8 command1[8448 + 4]; // enough space for flash chips with pages up to 8448 bytes
	uint8 command2[8448 + 4];
	if ( length > capacity ) {
		stringstream msg;
		msg << "Attempt to write " << length << " bytes to a device with a capacity of only "
			 << capacity << " bytes";
		throw GordonException(msg.str());
	}
	if ( startOffset == 0 && endOffset == 0 ) {
		// No reading necessary - the chunk is block-aligned at both ends
		count = endPage - pageNum;
		command1[0] = 0x82;  // "write through first buffer" opcode
		while ( count-- ) {
			address = pageNum << bitShift;
			command1[1] = (uint8)(address >> 16);
			command1[2] = (uint8)(address >> 8);
			command1[3] = (uint8)address;
			memcpy(command1 + 4, data, pageSize);
			sf->sendMessage(command1, pageSize + 4);
			data += pageSize;
			length -= pageSize;
			pageNum++;
			do {
				sf->sendMessage(&readStatus, 1, &status, 1);
			} while ( !(status & BM_READY) );
		}
	} else if ( endPage == pageNum || (endPage == pageNum + 1 && endOffset == 0) ) {
		// The chunk fits inside one block - read, update, then write back
		address = pageNum << bitShift;
		command1[0] = 0x03;  // "read flash" opcode
		command1[1] = (uint8)(address >> 16);
		command1[2] = (uint8)(address >> 8);
		command1[3] = (uint8)address;
		sf->sendMessage(command1, 4, command1 + 4, pageSize);
		memcpy(command1 + 4 + startOffset, data, length);
		command1[0] = 0x82;  // "write through first buffer" opcode
		sf->sendMessage(command1, pageSize + 4);
		do {
			sf->sendMessage(&readStatus, 1, &status, 1);
		} while ( !(status & BM_READY) );
	} else if ( startOffset == 0 ) {
		// The chunk is block-aligned at the start
		address = endPage << bitShift;
		command1[0] = 0x03;  // "read flash" opcode
		command1[1] = (uint8)(address >> 16);
		command1[2] = (uint8)(address >> 8);
		command1[3] = (uint8)address;
		sf->sendMessage(command1, 4, command1 + 4, pageSize);
		count = endPage - pageNum;
		command2[0] = 0x82;  // "write through first buffer" opcode
		while ( count-- ) {
			address = pageNum << bitShift;
			command2[1] = (uint8)(address >> 16);
			command2[2] = (uint8)(address >> 8);
			command2[3] = (uint8)address;
			memcpy(command2 + 4, data, pageSize);
			sf->sendMessage(command2, pageSize + 4);
			data += pageSize;
			length -= pageSize;
			pageNum++;
			do {
				sf->sendMessage(&readStatus, 1, &status, 1);
			} while ( !(status & BM_READY) );
		}
		command1[0] = 0x82;
		memcpy(command1 + 4, data, length);
		sf->sendMessage(command1, pageSize + 4);
		do {
			sf->sendMessage(&readStatus, 1, &status, 1);
		} while ( !(status & BM_READY) );
	} else if ( endOffset == 0 ) {
		// The chunk is block-aligned at the end
		address = pageNum << bitShift;
		command1[0] = 0x03;  // "read flash" opcode
		command1[1] = (uint8)(address >> 16);
		command1[2] = (uint8)(address >> 8);
		command1[3] = (uint8)address;
		sf->sendMessage(command1, 4, command1 + 4, pageSize);
		memcpy(command1 + 4 + startOffset, data, pageSize - startOffset);
		command1[0] = 0x82;  // "write through first buffer" opcode
		sf->sendMessage(command1, pageSize + 4);
		data += pageSize - startOffset;
		length -= pageSize - startOffset;
		pageNum++;
		do {
			sf->sendMessage(&readStatus, 1, &status, 1);
		} while ( !(status & BM_READY) );
		count = endPage - pageNum;
		while ( count-- ) {
			address = pageNum << bitShift;
			command1[1] = (uint8)(address >> 16);
			command1[2] = (uint8)(address >> 8);
			command1[3] = (uint8)address;
			memcpy(command1 + 4, data, pageSize);
			sf->sendMessage(command1, pageSize + 4);
			data += pageSize;
			length -= pageSize;
			pageNum++;
			do {
				sf->sendMessage(&readStatus, 1, &status, 1);
			} while ( !(status & BM_READY) );
		}
	} else {
		// The chunk is non-aligned at both ends
		address = endPage << bitShift;  // read end page
		command1[0] = 0x03;  // "read flash" opcode
		command1[1] = (uint8)(address >> 16);
		command1[2] = (uint8)(address >> 8);
		command1[3] = (uint8)address;
		sf->sendMessage(command1, 4, command1 + 4, pageSize);
		address = pageNum << bitShift;  // now read start page
		command2[0] = 0x03;  // "read flash" opcode
		command2[1] = (uint8)(address >> 16);
		command2[2] = (uint8)(address >> 8);
		command2[3] = (uint8)address;
		sf->sendMessage(command2, 4, command2 + 4, pageSize);
		memcpy(command2 + 4 + startOffset, data, pageSize - startOffset);
		command2[0] = 0x82;  // "write through first buffer" opcode
		sf->sendMessage(command2, pageSize + 4); // rewrite start page
		data += pageSize - startOffset;
		length -= pageSize - startOffset;
		pageNum++;
		do {
			sf->sendMessage(&readStatus, 1, &status, 1);
		} while ( !(status & BM_READY) );
		count = endPage - pageNum;
		while ( count-- ) {
			address = pageNum << bitShift; // overwrite intervening pages (if any)
			command2[1] = (uint8)(address >> 16);
			command2[2] = (uint8)(address >> 8);
			command2[3] = (uint8)address;
			memcpy(command2 + 4, data, pageSize);
			sf->sendMessage(command2, pageSize + 4);
			data += pageSize;
			length -= pageSize;
			pageNum++;
			do {
				sf->sendMessage(&readStatus, 1, &status, 1);
			} while ( !(status & BM_READY) );
		}
		command1[0] = 0x82;
		memcpy(command1 + 4, data, length);
		sf->sendMessage(command1, pageSize + 4);  // finally rewrite end page
		do {
			sf->sendMessage(&readStatus, 1, &status, 1);
		} while ( !(status & BM_READY) );
	}
}

static const Device m_atmelDevices[] = {
	{"AT45DB161D", 4096*528, 1<<21, 528, 512, 10, 9, &Prog82::INSTANCE, 0x2600},
	{0,            0,        0,     0,   0,   0,  0, 0,                 0x0000}
};
static const Device m_micronDevices[] = {
	{"M25P40", 1<<19, 1<<19, 256, 256, 8, 8, &Prog02::INSTANCE, 0x2013},
	{0,        0,     0,     0,   0,   0, 0, 0,                 0x0000}
};
static const Manufacturer m_manufacturers[] = {
	{"Atmel", 0x1F, m_atmelDevices},
	{"Micron", 0x20, m_micronDevices},
	{0, 0x00, 0}
};

SPIFlash::SPIFlash(const Gordon *gord) : m_gord(gord) {
	const uint8 readIdent = 0x9F;  // JEDEC ID command
	uint8 buf[1024];
	uint8 manuID;
	uint16 devID;
	const struct Manufacturer *manuIter = m_manufacturers;
	const struct Device *devIter;
	m_gord->sendMessage(&readIdent, 1, buf, 1024);

	// Get the manufacturer
	manuID = buf[0];
	while ( manuIter->name && manuID != manuIter->id ) {
		manuIter++;
	}
	if ( !manuIter->name ) {
		stringstream msg;
		msg << "Unrecognised manufacturer: " << hex << (uint32)manuID;
		throw GordonException(msg.str());
	}
	m_manufacturer = manuIter;
	devIter = manuIter->devices;

	// Get the capacity
	devID = (uint16)((buf[1] << 8) | buf[2]);
	while ( devIter->name && devID != devIter->id ) {
		devIter++;
	}
	if ( !devIter->name ) {
		stringstream msg;
		msg << "Unrecognised device: " << hex << (uint32)devID;
		throw GordonException(msg.str());
	}
	m_device = devIter;
}

template<typename T> class Janitor {
	const T *const m_ptr;
public:
	Janitor(const T *ptr) : m_ptr(ptr) { }
	~Janitor() { delete m_ptr; }
};
template<typename T> class ArrayJanitor {
	const T *const m_ptr;
public:
	ArrayJanitor(const T *ptr) : m_ptr(ptr) { }
	~ArrayJanitor() { delete []m_ptr; }
};

#include "random.h"

int main(int argc, char *argv[]) {
	int retVal = 0;
	struct arg_str *vpOpt = arg_str1("v", "vp", "<VID:PID[:DID]>", "  VID, PID and opt. dev ID (e.g 1D50:602B:0001)");
	struct arg_uint *conOpt = arg_uint0("c", "conduit", "<conduit>", "   which comm conduit to choose (default 0x00)");
	struct arg_file *progOpt = arg_file0("p", "prog", "<binFile>", "      program the flash");
	struct arg_str *readOpt = arg_str0("r", "read", "<binFile:size>", " read the flash");
	struct arg_lit *fooOpt = arg_lit0("f", "foo", "                foo sandbox");
	struct arg_lit *bootOpt = arg_lit0("b", "boot", "                start the AVR bootloader");
	struct arg_lit *helpOpt  = arg_lit0("h", "help", "                print this help and exit\n");
	struct arg_end *endOpt   = arg_end(20);
	void *argTable[] = {vpOpt, conOpt, progOpt, readOpt, fooOpt, bootOpt, helpOpt, endOpt};
	const char *const progName = "gordon";
	try {
		int numErrors;
		FLStatus fStatus;
		const char *error = NULL;
		const char *vp = NULL;
		uint8 conduit = 0x00;
		Gordon *gord;

		if ( arg_nullcheck(argTable) != 0 ) {
			throw GordonException("Insufficient memory");
		}
		
		numErrors = arg_parse(argc, argv, argTable);

		if ( helpOpt->count > 0 ) {
			cout << "Gordon Flash Tool Copyright (C) 2013 Chris McClelland\n\nUsage: " << progName;
			arg_print_syntax(stdout, argTable, "\n");
			cout << "\nProgram an FPGA configuration flash.\n\n";
			arg_print_glossary(stdout, argTable,"  %-10s %s\n");
			arg_freetable(argTable, sizeof(argTable)/sizeof(argTable[0]));
			return 0;
		}
		
		if ( numErrors > 0 ) {
			arg_print_errors(stderr, endOpt, progName);
			throw GordonException("Try '%s --help' for more information.");
		}
		
		fStatus = flInitialise(0, &error);
		Gordon::checkThrow(fStatus, error);

		vp = vpOpt->sval[0];

		// TODO: Hardware without direct micro->flash SPI connections needs to use spi-talk, which
		//       requires choosing a CommFPGA conduit.
		if ( conOpt->count ) {
			conduit = (uint8)conOpt->ival[0];
		}

		printf("Attempting to open connection to FPGALink device %s...\n", vp);
		if ( conduit ) {
			gord = new GordonIndirect(vp, conduit);
		} else {
			//gord = new GordonIceBlink(vp);
			gord = new GordonSPI(vp, "B3B2B0B1", SPI_MSBFIRST);
		}
		Janitor<Gordon> janitor(gord);

		if ( fooOpt->count ) {
			//const uint8 writeBuffer[] = {0x84, 0x00, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78};
			//const uint8 readBuffer[] = {0xD1, 0x00, 0x00, 0x00};
			//uint8 result[4];
			//gord->sendMessage(writeBuffer, 8);
			//gord->sendMessage(readBuffer, 4, result, 4);
			//dump(0, result, 4);
			SPIFlash f(gord);
			//const uint8 foo[] = {0xCA, 0xFE, 0xBA, 0xBE};
			cout << "Manufacturer: " << f.getManufacturerName() << endl;
			cout << "Device: " << f.getDeviceName() << endl;
			cout << "Capacity: " << f.getCapacity() << endl;
			//f.write(0, fourPages1, sizeof(fourPages1));
			vector<uint8> readback;
			f.read(0, readback); //, sizeof(fourPages1));
			dump(0, &readback.front(), sizeof(fourPages1));
			//f.write(528+528-32, fourPages2, 528 + 64);
			//cout << "Updated..." << endl;
			//f.read(0, readback, sizeof(fourPages1));
			//dump(0, &readback.front(), sizeof(fourPages1));
		}

		if ( readOpt->count ) {
			const uint8 readData[] = {0x03, 0x00, 0x00, 0x00};
			const char *opt = readOpt->sval[0], *ptr = opt;
			char ch = *ptr;
			while ( ch && ch != ':' ) {
				ch = *++ptr;
			}
			if ( ch != ':' ) {
				throw GordonException("invalid argument to option -r|--read=<binFile:size>");
			}
			string binFile(opt, ptr-opt);
			ptr++;
			size_t size = strtoul(ptr, NULL, 0);
			cout << "binFile = " << binFile << "; size = " << size << endl;
			uint8 *buffer = new uint8[size];
			Janitor<uint8> bufJan(buffer);
			gord->sendMessage(readData, 4, buffer, size);
			ofstream file(binFile.c_str(), ios::out | ios::binary);
			if ( !file.is_open() ) {
				throw GordonException("Unable to open file for writing");
			}
			file.write((const char *)buffer, size);
		}

		if ( progOpt->count ) {
			uint16 page;
			size_t bytesRead;
			uint8 buffer[4+256];
			uint32 i;
			const uint8 writeEnable = 0x06;
			const uint8 readStatus = 0x05;
			const uint8 bulkErase = 0xC7;
			const char *const binFile = progOpt->filename[0];
			ifstream file(binFile, ios::in | ios::binary);
			if ( !file.is_open() ) {
				throw GordonException("Unable to open file for reading");
			}
			
			cout << "\nErasing";
			
			// Enable the Write Enable (WEL) bit
			gord->sendMessage(&writeEnable);
			
			// Bulk erase
			gord->sendMessage(&bulkErase);
			
			// Wait for the Work In Progress (WIP) bit to clear
			i = 7;
			do {
				flSleep(10);   // no point hammering it
				cout << (i++ ? "." : "\n.") << flush; i &= 63;
				gord->sendMessage(&readStatus, 1, buffer, 1);
			} while ( buffer[0] & 0x01 );
			
			cout << "\n\nProgramming";
			page = 0x0000;
			i = 11;
			for ( ; ; ) {
				// Enable the Write Enable (WEL) bit
				gord->sendMessage(&writeEnable);
				
				// Write page
				buffer[0] = 0x02;
				buffer[1] = (uint8)(page >> 8);
				buffer[2] = (uint8)page;
				buffer[3] = 0x00;
				file.read((char*)buffer+4, 256L);
				bytesRead = file.gcount();
				if ( !bytesRead ) {
					break;
				}
				
				// Send it!
				gord->sendMessage(buffer, bytesRead + 4);
				
				flSleep(5);
				cout << (i++ ? "." : "\n.") << flush; i &= 63;
				page++;
			}
			cout << "\n\nDone!" << endl;
		}
		
		if ( bootOpt->count ) {
			fStatus = flBootloader(gord->getHandle(), &error);
			Gordon::checkThrow(fStatus, error);
		}
	}
	catch ( const GordonException &ex ) {
		cerr << progName << ": " << ex.what() << endl;
		retVal = 1;
	}
	arg_freetable(argTable,sizeof(argTable)/sizeof(argTable[0]));
	return retVal;
}
