/*
 * Runes of Magic proxy - client
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
#include "client.h"
#include <assert.h>
#include "address.h"

Client::Client()
	: m_MustDrop(false)
{
}

Client::Client(const Address& localaddr, const Address& remoteaddr)
	: m_MustDrop(false), m_LocalAddress(localaddr), m_RemoteAddress(remoteaddr)
{
}

bool
Client::Connect(const Address& address)
{
	int fd = address.Connect();
	if (fd < 0)
		return false;

	{
		struct sockaddr sa;
		socklen_t sa_len = sizeof(sa);
		if (getsockname(fd, &sa, &sa_len) == 0) {
			m_LocalAddress = Address(address, &sa, sa_len);
		} else {
			m_LocalAddress = Address();
		}
	}
	m_RemoteAddress = address;
	SetFD(fd);
	return true;
}

void
Client::SetFD(int fd)
{
	assert(!IsConnected());
	m_FD = fd;
}

void
Client::OnEvent()
{
}

/* vim:set ts=2 sw=2: */
