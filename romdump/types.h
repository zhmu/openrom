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
#ifndef __TYPES_H__
#define __TYPES_H__

#include <stdint.h>
#include <string>

typedef uint32_t ipv4_addr_t;
typedef uint16_t ipv4_port_t;

class IPv4Address
{
public:
	IPv4Address() : m_Address(0), m_Port(0) { }
	IPv4Address(ipv4_addr_t oAddress, ipv4_port_t oPort) : m_Address(oAddress), m_Port(oPort) { }

	ipv4_addr_t& Address() { return m_Address; }
	ipv4_port_t& Port() { return m_Port; }

	bool operator==(const IPv4Address& oRHS) const {
		return m_Address == oRHS.m_Address && m_Port == oRHS.m_Port;
	}

	bool operator!=(const IPv4Address& oRHS) const {
		return !(*this == oRHS);
	}

	bool operator<(const IPv4Address& oRHS) const {
		if (m_Address == oRHS.m_Address)
			return m_Port < oRHS.m_Port;
		return m_Address < oRHS.m_Address;
	}

	std::string ToString() const;
	

protected:
	ipv4_addr_t m_Address;
	ipv4_port_t m_Port;
};

#endif /* __TYPES_H__ */
