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
#include "tcpflowparser.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"

#define LINE_MAX 256

const char*
TCPFlowParser::ParseIPv4Address(const char* line, IPv4Address& oAddress)
{
	unsigned int a, b, c, d, e;
	char* ptr;
	a = (unsigned int)strtoul(line, &ptr, 10);
	if (*ptr != '.')
		return NULL;
	b = (unsigned int)strtoul(ptr + 1, &ptr, 10);
	if (*ptr != '.')
		return NULL;
	c = (unsigned int)strtoul(ptr + 1, &ptr, 10);
	if (*ptr != '.')
		return NULL;
	d = (unsigned int)strtoul(ptr + 1, &ptr, 10);
	if (*ptr != '.')
		return NULL;
	e = (unsigned int)strtoul(ptr + 1, &ptr, 10);
	oAddress.Address() = a << 24 | b << 16 | c << 8 | d;
	oAddress.Port() = e;
	return (const char*)ptr;
}

/* parser for /tcpflow -cD output */
int
TCPFlowParser::ParseHeader(FILE* pFile, IPv4Address& oSourceAddress, IPv4Address& oDestAddress)
{
	char line[LINE_MAX];
	line[LINE_MAX - 1] = '\0';
	if (fgets(line, LINE_MAX - 1, pFile) == NULL)
		return 0;

	/* Format '<ip>.<port>-<ip>.<port>' */
	const char* ptr = ParseIPv4Address(line, oSourceAddress);
	if (ptr == NULL)
		return -1;
	ptr = ParseIPv4Address(ptr + 1, oDestAddress);
	if (ptr == NULL)
		return -1;
	return *ptr == ':' ? 1 : -1;
}

int
TCPFlowParser::ParsePacket(FILE* pFile, char* pBuffer, int iMaxLength)
{
	int len = 0;
	while (1) {
		char line[LINE_MAX];
		line[LINE_MAX - 1] = '\0';
		if (fgets(line, LINE_MAX - 1, pFile) == NULL)
			break;

		/*
		 * There are two cases we need to distinguish:
		 *
		 * 1) <number>: <hex> <text>
		 *    Packet contents
		 * 2) Blank link
		 *    Done!
		 */
		char* ptr = strchr(line, '\n');
		if (ptr != NULL)
			*ptr = '\0';

		if (*line == '\0') {
			/* (2) - We are done */
			break;
		}

		if (strlen(line) > 4 && line[4] == ':') {
			/* (1) - */
			ptr = line + 6;
			while(*ptr != ' ') {
				/* Parse the next byte values; the fields are seperated by a space */
				char* cur = ptr;
				unsigned int v = (unsigned int)strtoul(cur, &ptr, 16);
				int num_digits = (ptr - cur) / 2;
				if (num_digits == 0)
					break;
				assert(num_digits >= 1 && num_digits <= 2);
				if (*ptr == ' ')
					ptr++;

				iMaxLength -= num_digits;
				assert(iMaxLength >= 0); /* XXX crude */

				/* Now store our digits */
				if (num_digits > 1)
					pBuffer[len++] = v >> 8;
				pBuffer[len++] = v & 0xff;
			}
			continue;
		}

		/* What's this? */
		return -1;
	}

	return len;
}

/* vim:set ts=2 sw=2: */
