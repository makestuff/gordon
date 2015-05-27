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
#include <cstdio>
#include <cstdlib>
#include <makestuff.h>
#include <argtable2.h>
#include "exception.h"
#include "transport_direct.h"
#include "transport_indirect.h"
#include "transport_iceblink.h"
#include "flash_chips.h"
#include "region_programmer.h"
#include "janitors.h"
#include "util.h"

using namespace std;

class FLContextJanitor {
	FLContext *const m_ptr;
	FLContextJanitor &operator=(const FLContextJanitor &other);
	FLContextJanitor(const FLContextJanitor &other);
public:
	explicit FLContextJanitor(FLContext *ptr) : m_ptr(ptr) { }
	~FLContextJanitor() { if ( m_ptr) { flClose(m_ptr); } }
};

int main(int argc, char *argv[]) {
	int retVal = 0;
	struct arg_str *vpOpt = arg_str1("v", "vp", "<VID:PID[:DID]>", " VID, PID and opt. dev ID (e.g 1D50:602B:0001)");
	struct arg_str *txOpt = arg_str0("t", "transport", "<spec>", "   specify the flash communication mechanism");
	struct arg_str *writeOpt = arg_str0("w", "write", "<f:a>", "        write file f to address a");
	struct arg_str *readOpt = arg_str0("r", "read", "<f:a:l>", "       read l bytes into file f from address a");
	struct arg_lit *swapOpt = arg_lit0("s", "swap", "               bit-swap the flash data read or written");
	struct arg_lit *bootOpt = arg_lit0("b", "boot", "               start the AVR bootloader");
	struct arg_lit *helpOpt  = arg_lit0("h", "help", "               print this help and exit\n");
	struct arg_end *endOpt   = arg_end(20);
	void *argTable[] = {vpOpt, txOpt, writeOpt, readOpt, swapOpt, bootOpt, helpOpt, endOpt};
	const char *const progName = "gordon";
	try {
		int numErrors;
		FLStatus fStatus;
		const char *error = NULL;
		const char *vp = NULL;
		Transport *transport = NULL;
		struct FLContext *handle = NULL;

		if ( arg_nullcheck(argTable) != 0 ) {
			throw GordonException("Insufficient memory");
		}

		numErrors = arg_parse(argc, argv, argTable);

		if ( helpOpt->count > 0 ) {
			printf("Gordon Flash Tool Copyright (C) 2013-2015 Chris McClelland\n\nUsage: %s", progName);
			arg_print_syntax(stdout, argTable, "\n");
			printf("\nProgram an FPGA configuration flash.\n\n");
			arg_print_glossary(stdout, argTable,"  %-10s %s\n");
			arg_freetable(argTable, sizeof(argTable)/sizeof(argTable[0]));
			return 0;
		}

		if ( numErrors > 0 ) {
			arg_print_errors(stderr, endOpt, progName);
			throw GordonException("Try '%s --help' for more information.");
		}

		fStatus = flInitialise(0, &error);
		TransportUSB::checkThrow(fStatus, error);
		vp = vpOpt->sval[0];
		fStatus = flOpen(vp, &handle, &error);
		TransportUSB::checkThrow(fStatus, error);
		FLContextJanitor cxtJan(handle);

		// If reading or writing a flash chip, a transport spec must be supplied.
		if ( readOpt->count || writeOpt->count ) {
			if ( txOpt->count == 0 ) {
				throw GordonException("If you specify -r or -w then -t is required");
			}
			const char *txSpec = txOpt->sval[0];
			if ( startsWith(txSpec, "direct:") ) {
				transport = new TransportDirect(handle, txSpec + 7);
			} else if ( startsWith(txSpec, "indirect:") ) {
				transport = new TransportIndirect(handle, txSpec + 9);
			} else if ( startsWith(txSpec, "iceblink") ) {
				transport = new TransportIceBlink(handle);
			} else {
				throw GordonException("Invalid argument to option -t|--transport=<spec>.");
			}
		}
		Janitor<Transport> txJan(transport);

		// Read from flash.
		if ( readOpt->count ) {
			const FlashChip *const flashChip = findChip(transport);
			RegionProgrammer prog(transport, flashChip);
			printf("Device: %s %s\n", flashChip->vendorName, flashChip->deviceName);
			const char *opt = readOpt->sval[0], *ptr = opt;
			char ch = *ptr;
			while ( ch && ch != ':' ) {
				ch = *++ptr;
			}
			if ( ch != ':' ) {
				throw GordonException("Invalid argument to option -r|--read=<binFile:address:length>.");
			}
			string fileName(opt, ptr-opt);
			ptr++;
			uint32 address = (uint32)strtoul(ptr, (char**)&ptr, 0);
			if ( *ptr != ':' ) {
				throw GordonException("Invalid argument to option -r|--read=<binFile:address:length>.");
			}
			ptr++;
			uint32 length = (uint32)strtoul(ptr, NULL, 0);
			uint8 *buffer = new uint8[length];
			ArrayJanitor<uint8> bufJan(buffer);
			prog.read(address, length, buffer);
			if ( swapOpt->count ) {
				spiBitSwap(length, buffer);
			}
			FILE *file = fopen(fileName.c_str(), "wb");
			if ( !file ) {
				throw GordonException("Unable to open file for writing.");
			}
			if ( length != (uint32)fwrite(buffer, 1, length, file) ) {
				throw GordonException("Unable to write entire buffer to file.");
			}
			fclose(file);
		}

		// Write to flash.
		if ( writeOpt->count ) {
			const FlashChip *const flashChip = findChip(transport);
			RegionProgrammer prog(transport, flashChip);
			printf("Device: %s %s\n", flashChip->vendorName, flashChip->deviceName);
			const char *opt = writeOpt->sval[0], *ptr = opt;
			char ch = *ptr;
			while ( ch && ch != ':' ) {
				ch = *++ptr;
			}
			if ( ch != ':' ) {
				throw GordonException("Invalid argument to option -w|--write=<f:a>.");
			}
			string fileName(opt, ptr-opt);
			ptr++;
			uint32 address = (uint32)strtoul(ptr, (char**)&ptr, 0);
			if ( *ptr != '\0' ) {
				throw GordonException("Invalid argument to option -w|--write=<f:a>.");
			}
			ptr++;
			size_t length;
			uint8 *file = loadFile(fileName.c_str(), &length);
			if ( !file ) {
				throw GordonException("Unable to read from file.");
			}
			if ( swapOpt->count ) {
				spiBitSwap((uint32)length, file);
			}
			AllocJanitor fileJan(file);
			prog.write(address, (uint32)length, file);
		}

		// Put an FPGALink/AVR device in DFU mode ready for updating its firmware.
		if ( bootOpt->count ) {
			fStatus = flBootloader(handle, &error);
			TransportUSB::checkThrow(fStatus, error);
		}
	}
	catch ( const GordonException &ex ) {
		fprintf(stderr, "%s\n", ex.what());
		retVal = ex.retVal();
	}
	catch ( const exception &ex ) {
		fprintf(stderr, "%s\n", ex.what());
		retVal = -1;
	}
	arg_freetable(argTable,sizeof(argTable)/sizeof(argTable[0]));
	return retVal;
}
