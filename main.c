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
#include <stdio.h>
#include <stdlib.h>
#include <makestuff.h>
#include <libfpgalink.h>
#include <liberror.h>
#include <argtable2.h>

typedef enum {
	GORD_SUCCESS,
	GORD_LIBERR,
	GORD_CONDUIT_RANGE,
	GORD_NO_MEMORY,
	GORD_CANNOT_LOAD,
	GORD_CANNOT_SAVE,
	GORD_ARGS
} ReturnCode;

// Send a command over SPI, and return the response.
//
static ReturnCode sendMessage(
	struct FLContext *handle, uint8 ssPort, uint8 ssBit,
	const uint8 *cmdData, uint32 cmdLength, uint8 *recvBuf, uint32 recvLength, BitOrder bitOrder,
	const char **error)
{
	ReturnCode retVal = GORD_SUCCESS;
	FLStatus fStatus = flSingleBitPortAccess(handle, ssPort, ssBit, PIN_LOW, NULL, error);
	CHECK_STATUS(fStatus, GORD_LIBERR, cleanup, "sendMessage()");
	fStatus = spiSend(handle, cmdData, cmdLength, bitOrder, error);
	CHECK_STATUS(fStatus, GORD_LIBERR, cleanup, "sendMessage()");
	if ( recvLength ) {
		fStatus = spiRecv(handle, recvBuf, recvLength, bitOrder, error);
		CHECK_STATUS(fStatus, GORD_LIBERR, cleanup, "sendMessage()");
	}
	fStatus = flSingleBitPortAccess(handle, ssPort, ssBit, PIN_HIGH, NULL, error);
	CHECK_STATUS(fStatus, GORD_LIBERR, cleanup, "sendMessage()");
cleanup:
	return retVal;
}

int main(int argc, char *argv[]) {
	ReturnCode retVal = GORD_SUCCESS, gStatus;
	struct arg_str *vpOpt = arg_str1("v", "vp", "<VID:PID[:DID]>", " VID, PID and opt. dev ID (e.g 1D50:602B:0001)");
	struct arg_uint *conOpt = arg_uint0("c", "conduit", "<conduit>", "  which comm conduit to choose (default 0x00)");
	struct arg_file *progOpt = arg_file0("p", "prog", "<binFile>", "     program the flash");
	struct arg_lit *bootOpt = arg_lit0("b", "boot", "               start the AVR bootloader");
	struct arg_lit *helpOpt  = arg_lit0("h", "help", "               print this help and exit\n");
	struct arg_end *endOpt   = arg_end(20);
	void *argTable[] = {vpOpt, conOpt, progOpt, bootOpt, helpOpt, endOpt};
	const char *progName = "gordon";
	int numErrors;
	struct FLContext *handle = NULL;
	FLStatus fStatus;
	const char *error = NULL;
	const char *vp = NULL;
	uint8 conduit = 0x00;
	FILE *file = NULL;

	if ( arg_nullcheck(argTable) != 0 ) {
		fprintf(stderr, "%s: insufficient memory\n", progName);
		FAIL(1, cleanup);
	}

	numErrors = arg_parse(argc, argv, argTable);

	if ( helpOpt->count > 0 ) {
		printf("Gordon Flash Tool Copyright (C) 2013 Chris McClelland\n\nUsage: %s", progName);
		arg_print_syntax(stdout, argTable, "\n");
		printf("\nProgram an FPGA configuration flash.\n\n");
		arg_print_glossary(stdout, argTable,"  %-10s %s\n");
		FAIL(GORD_SUCCESS, cleanup);
	}

	if ( numErrors > 0 ) {
		arg_print_errors(stdout, endOpt, progName);
		fprintf(stderr, "Try '%s --help' for more information.\n", progName);
		FAIL(GORD_ARGS, cleanup);
	}

	fStatus = flInitialise(0, &error);
	CHECK_STATUS(fStatus, GORD_LIBERR, cleanup);

	vp = vpOpt->sval[0];

	printf("Attempting to open connection to FPGALink device %s...\n", vp);
	fStatus = flOpen(vp, &handle, &error);
	CHECK_STATUS(fStatus, GORD_LIBERR, cleanup);

	// TODO: Hardware without direct micro->flash SPI connections needs to use spi-talk, which
	//       requires choosing a CommFPGA conduit.
	if ( conOpt->count ) {
		conduit = (uint8)conOpt->ival[0];
	}
	fStatus = flSelectConduit(handle, conduit, &error);
	CHECK_STATUS(fStatus, GORD_CONDUIT_RANGE, cleanup);

	if ( progOpt->count ) {
		const char *const binFile = progOpt->filename[0];
		uint16 page;
		uint32 bytesRead;
		uint8 buffer[4+256];
		uint32 i;
		const uint8 writeEnable = 0x06;
		const uint8 readStatus = 0x05;
		const uint8 bulkErase = 0xC7;
		file = fopen(binFile, "rb");
		if ( !file ) {
			errRenderStd(&error);
			FAIL(GORD_CANNOT_LOAD, cleanup);
		}

		// TODO: Factor out sendCommand(cmdData, cmdSize, recvBuf, recvSize)
		// POWER  = PC2
		// SS     = PB0
		// SCK    = PB1
		// MOSI   = PB2
		// MISO   = PB3
		// CRESET = PB6
		//
		fStatus = flMultiBitPortAccess(handle, "B6-,C2-,B0-", NULL, &error); // CRESET, POWER & SS low
		CHECK_STATUS(fStatus, GORD_LIBERR, cleanup);
		flSleep(10);
		fStatus = flSingleBitPortAccess(handle, 2, 2, PIN_HIGH, NULL, &error);  // POWER(C2) high
		CHECK_STATUS(fStatus, GORD_LIBERR, cleanup);
		flSleep(10);
		fStatus = flMultiBitPortAccess(handle, "B0+,B1-,B2-", NULL, &error); // SS hi, SCK & MOSI low
		CHECK_STATUS(fStatus, GORD_LIBERR, cleanup);

		printf("\nErasing");

		// Enable the Write Enable (WEL) bit
		gStatus = sendMessage(handle, 1, 0, &writeEnable, 1, NULL, 0, SPI_MSBFIRST, &error);
		CHECK_STATUS(gStatus, gStatus, cleanup);

		// Bulk erase
		gStatus = sendMessage(handle, 1, 0, &bulkErase, 1, NULL, 0, SPI_MSBFIRST, &error);
		CHECK_STATUS(gStatus, gStatus, cleanup);

		// Wait for the Work In Progress (WIP) bit to clear
		i = 7;
		do {
			flSleep(10);   // no point hammering it
			printf(i++ ? "." : "\n."); i &= 63;
			fflush(stdout);
			gStatus = sendMessage(handle, 1, 0, &readStatus, 1, buffer, 1, SPI_MSBFIRST, &error);
			CHECK_STATUS(gStatus, gStatus, cleanup);
		} while ( buffer[0] & 0x01 );

		printf("\n\nProgramming");
		page = 0x0000;
		i = 11;
		for ( ; ; ) {
			// Enable the Write Enable (WEL) bit
			gStatus = sendMessage(handle, 1, 0, &writeEnable, 1, NULL, 0, SPI_MSBFIRST, &error);
			CHECK_STATUS(gStatus, gStatus, cleanup);

			// Write page
			buffer[0] = 0x02;
			buffer[1] = (uint8)(page >> 8);
			buffer[2] = (uint8)page;
			buffer[3] = 0x00;
			bytesRead = (uint32)fread(buffer+4, 1, 256, file);
			if ( !bytesRead ) {
				break;
			}

			// Send it!
			gStatus = sendMessage(handle, 1, 0, buffer, 4+bytesRead, NULL, 0, SPI_MSBFIRST, &error);
			CHECK_STATUS(gStatus, gStatus, cleanup);
			
			flSleep(5);
			printf(i++ ? "." : "\n."); i &= 63;
			fflush(stdout);
			page++;
		}
		printf("\n\nDone!\n");
	}

	if ( bootOpt->count ) {
		fStatus = flBootloader(handle, &error);
		CHECK_STATUS(fStatus, GORD_LIBERR, cleanup);
	}
cleanup:
	if ( handle ) {
		// TODO: This is a bit of a hack because it will fail (silently, admittedly) if the AVR
		//       bootloader has been started.
		fStatus = flMultiBitPortAccess(handle, "B0?,B1?,B2?,B6?", NULL, NULL); // all inputs
	}
	if ( file ) {
		fclose(file);
	}
	flClose(handle);
	if ( error ) {
		fprintf(stderr, "%s\n", error);
		flFreeError(error);
	}
	return retVal;
}
