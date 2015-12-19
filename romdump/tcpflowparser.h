/*
 * Runes of Magic protocol analysis - tcpflow parsing code
 * Copyright (C) 2013-2015 Rink Springer <rink@rink.nu>
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
#ifndef __TCPFLOWPARSER_H__
#define __TCPFLOWPARSER_H__

#include <stdio.h> // for FILE

class IPv4Address;

class TCPFlowParser
{
public:
	TCPFlowParser();

	/*! \brief Parses a header line
	  * \param pFile File to parse
	 *  \param oSourceAddress Source address on success
	 *  \param oDestAddress Destination address on success
	 *  \returns -1 on end of file, 0 on failure, 1 success
	 */
	static int ParseHeader(FILE* pFile, IPv4Address& oSourceAddress, IPv4Address& oDestAddress);

	/*! \brief Parses a packet
	  * \param pFile File to parse
	  * \param pBuffer Buffer to fill
	  * \param iMaxLength Maximum number of bytes to fill
	  * \returns Number of bytes filled, or -1 on parse error
	  */
	static int ParsePacket(FILE* pFile, char* pBuffer, int iMaxLength);

protected:
	/*! \brief Parses an IPv4 address from a string
	 *  \param sLine Line to parse
	 *  \param oAddress Address to fill
	 *  \returns Position after IPv4 address or NULL on failure
	 */
	static const char* ParseIPv4Address(const char* sLine, IPv4Address& oAddress);
};

#endif /* __TCPFLOWPARSER_H__ */
