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
#include <string>
#include "exception.h"

using namespace std;

GordonException::GordonException(const char *msg, int retVal, bool cleanup) :
	m_msg(msg), m_retVal(retVal)
{
	if ( cleanup ) {
		flFreeError(msg);
	}
}

GordonException::GordonException(const string &msg, int retVal) :
	m_msg(msg), m_retVal(retVal)
{ }
