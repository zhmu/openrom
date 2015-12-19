/*
 * Runes of Magic protocol analysis - connection administration
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
#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#include "types.h"

class Connection {
public:
	Connection(const IPv4Address& oSource, const IPv4Address& oDest) : m_Source(oSource), m_Dest(oDest) { }

	bool operator<(const Connection& oRHS) const {
		if (m_Source == oRHS.m_Source)
			return m_Dest < oRHS.m_Dest;
		return m_Source < oRHS.m_Source;
	}

	const IPv4Address& GetSource() const { return m_Source; }
	const IPv4Address& GetDest() const { return m_Dest; }

private:
	//! \brief IPv4 connection source
	IPv4Address m_Source;

	//! \brief IPv4 connection destination
	IPv4Address m_Dest;
};

#endif /* __CONNECTION_H__ */

