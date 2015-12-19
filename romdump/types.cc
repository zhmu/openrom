/*
 * Runes of Magic protocol analysis - basic types
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
#include "types.h"
#include <stdio.h>

std::string
IPv4Address::ToString() const
{
	char sTemp[64];
	sprintf(sTemp, "%d.%d.%d.%d:%d",
	 m_Address >> 24,
	 (m_Address >> 16) & 0xff,
	 (m_Address >>  8) & 0xff,
	 m_Address & 0xff,
	 m_Port);
	return sTemp;
}

/* vim:set ts=2 sw=2: */
