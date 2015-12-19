/*
 * Runes of Magic proxy - address
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
#include "address.h"
#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

Address::Address()
{
	Reset();
}

Address::Address(const Address& source, const sockaddr* sa, socklen_t sa_len)
	: m_socklen(sa_len), m_family(source.m_family),
	  m_socktype(source.m_socktype), m_protocol(source.m_protocol)
{
	assert(m_socklen <= sizeof(m_sockaddr));
	memcpy(&m_sockaddr, sa, sa_len);
}

void
Address::Reset()
{
	m_socklen = -1;
	m_family = 0;
	m_socktype = 0;
	m_protocol = 0;
}

bool
Address::Resolve(const char* hostname, const char* service)
{
	Reset();

	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	struct addrinfo* result;
	int gai_result = getaddrinfo(hostname, service, &hints, &result);
	if (gai_result != 0)
		return false;

	for (struct addrinfo* rp = result; rp != NULL; rp = rp->ai_next) {
		// See if we support the socket type
		int fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (fd < 0)
			continue; // cannot connect to this
		close(fd);

		// Sanity check
		assert(rp->ai_addrlen <= sizeof(m_sockaddr));

		// This type worked - store the details
		memcpy(&m_sockaddr, rp->ai_addr, rp->ai_addrlen);
		m_socklen = rp->ai_addrlen;
		m_family = rp->ai_family;
		m_socktype = rp->ai_socktype;
		m_protocol = rp->ai_protocol;
		break;
	}
	freeaddrinfo(result);
	return m_socklen > 0;
}

int
Address::Connect() const
{
	assert(m_socklen > 0);

	int fd = socket(m_family, m_socktype, m_protocol);
	if (fd < 0)
		return -1;

	if (connect(fd, &m_sockaddr, m_socklen) < 0) {
		close(fd);
		fd = -1;
	}

	return fd;
}

void
Address::GetIPv4Address(uint32_t& ip, uint16_t& port) const
{
	if (m_family == AF_INET) {
		struct sockaddr_in* sin = (struct sockaddr_in*)&m_sockaddr;
		ip = ntohl(sin->sin_addr.s_addr);
		port = ntohs(sin->sin_port);
	} else {
		ip = 0; port = 0;
	}
}

void
Address::SetPort(uint16_t port)
{
	assert(m_family == AF_INET);
	struct sockaddr_in* sin = (struct sockaddr_in*)&m_sockaddr;
	sin->sin_port = htons(port);
}

void
Address::ToString(char* s, int len) const
{
	if (m_family == AF_INET) {
		struct sockaddr_in* sin = (struct sockaddr_in*)&m_sockaddr;
		snprintf(s, len, "%s:%u", inet_ntoa(sin->sin_addr), ntohs(sin->sin_port));
	} else {
		strncpy(s, "?", len);
	}
}

bool
Address::operator==(const Address& rhs) const
{
	if (m_socklen != rhs.m_socklen || m_family != rhs.m_family || m_socktype != rhs.m_socktype || m_protocol != rhs.m_protocol)
		return false;

	return memcmp(&m_sockaddr, &rhs.m_sockaddr, m_socklen) == 0;
}

/* vim:set ts=2 sw=2: */
