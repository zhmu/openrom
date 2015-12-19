/*
 * Runes of Magic proxy - types
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
#ifndef __TYPES_H__
#define __TYPES_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <stdint.h>

/*! \brief Contains an network address
 */
class Address {
	friend class Server;
public:
	Address();
	Address(const Address& source, const sockaddr* sa, socklen_t sa_len);

	/*! \brief Resolves a hostname/IP address and service/port to an address
	 *  \param hostname Hostname or IP address to use, or NULL
	 *  \param service Service/port to use
	 *  \returns true on success
	 *
	 *  This function will make a dummy connection to each entry to ensure
	 *  that it is connectable.
	 */
	bool Resolve(const char* hostname, const char* service);

	/*! \brief Connects to the address contained
	 *  \returns File descriptor on success, or -1 on failure
	 */
	int Connect() const;

	//! \brief Resets the address
	void Reset();

	/*! \brief Converts the address to human-readable form
	 *  \param s String to fill
	 *  \param len Maximum number of bytes to fill
	 */
	void ToString(char* s, int len) const;

	/*! \brief Retrieves IPv4 address
	 *  \param ip To be filled with IP address
	 *  \param port To be filled with port
	 *
	 *  This function will set both members to zero on failure.
	 */
	void GetIPv4Address(uint32_t& ip, uint16_t& port) const;

	//! \brief Sets the port to use
	void SetPort(uint16_t port);

	//! \brief Comparison operator
	bool operator==(const Address& rhs) const;

protected:
	//! \brief Socket address
	struct sockaddr m_sockaddr;

	//! \brief Number of relevant bytes in the socket address
	int m_socklen;

	//! \brief Address family
	int m_family;

	//! \brief Socket type to use
	int m_socktype;

	//! \brief Protocol to use
	int m_protocol;

};

#endif /* __TYPES_H__ */
