/*
 * Runes of Magic protocol analysis - ROM logger parsing code
 * Copyright (C) 2014-2015 Rink Springer <rink@rink.nu>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License version 3
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __ROMLOGPARSER_H__
#define __ROMLOGPARSER_H__

#include <stdio.h> // for FILE

class IPv4Address;

class ROMLogParser
{
public:
	ROMLogParser();

	/*! \brief Parses a header structure
	  * \param pFile File to parse
	 *  \param oSourceAddress Source address on success
	 *  \param oDestAddress Destination address on success
	 *  \param v2 If set, this is a version 2+ header
	 *  \returns -1 on end of file, 0 on failure, next packet length on success
	 */
	static int ParseHeader(FILE* pFile, IPv4Address& oSourceAddress, IPv4Address& oDestAddress, bool v2);

	/*! \brief Parses a packet
	  * \param pFile File to parse
	  * \param pBuffer Buffer to fill
	  * \param iLength Number of bytes to fill
	  * \returns Number of bytes filled, or -1 on parse error
	  */
	static int ReadPacket(FILE* pFile, char* pBuffer, int iLength);
};

#endif /* __ROMLOGPARSER_H__ */
