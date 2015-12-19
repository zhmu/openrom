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
#include "romlogparser.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "romstructs.h"
#include "types.h"

#define LINE_MAX 256

int
ROMLogParser::ParseHeader(FILE* pFile, IPv4Address& oSourceAddress, IPv4Address& oDestAddress, bool v2)
{
	struct ROM::LoggerPacket lp;
	if (!fread(&lp, sizeof(lp), 1, pFile))
		return 0;

	oSourceAddress.Address() = lp.lp_source_ip;
	oSourceAddress.Port() = lp.lp_source_port;
	oDestAddress.Address() = lp.lp_dest_ip;
	oDestAddress.Port() = lp.lp_dest_port;
	if (!v2) {
		fseek(pFile, -2, SEEK_CUR);
		lp.lp_len2 = 0;
	}
	return lp.lp_len1 + (lp.lp_len2 << 16);
}

int
ROMLogParser::ReadPacket(FILE* pFile, char* pBuffer, int length)
{
	return fread(pBuffer, length, 1, pFile) * length;
}

/* vim:set ts=2 sw=2: */
