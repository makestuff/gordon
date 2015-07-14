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
#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <exception>
#include <string>

// Exception class for entire application.
//
class GordonException : public std::exception {
	// A message and a value to return to the OS
	const std::string m_msg;
	const int m_retVal;

	// Don't allow assignment
	GordonException &operator=(const GordonException &other);
public:
	// Public API: construct from a char* message which may need cleanup, or from
	// a std::string message which will not.
	explicit GordonException(const char *msg, int retVal = -1, bool cleanup = false);
	explicit GordonException(const std::string &msg, int retVal = -1);
	virtual ~GordonException() throw() { }

	// Return the exception's properties
	virtual const char *what() const throw() { return m_msg.c_str(); }
	virtual int retVal() const throw() { return m_retVal; }
};

#endif
