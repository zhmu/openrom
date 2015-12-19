/*
 * Runes of Magic proxy - server
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
#include "server.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>
#include <stdlib.h> // for NULL
#include <unistd.h> // for close()
#include "address.h"
#include "client.h"

bool
Server::Listen(const Address& address)
{
	assert(!IsConnected());

	m_BindAddress.Reset();
	int fd = socket(address.m_family, address.m_socktype, address.m_protocol);
	if (fd < 0)
		return false;

	// Allow rebinding to prevent annoying delays while developing
	{
		int on = 1;
		setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	}

	if (bind(fd, &address.m_sockaddr, address.m_socklen) < 0 ||
	    listen(fd, 5) < 0) {
		close(fd);
		return false;
	}

	m_BindAddress = address;
	m_FD = fd;
	return true;
}

Client*
Server::Accept()
{
	assert(IsConnected());

	struct sockaddr sa;
	socklen_t sa_len = sizeof(sa);
	int fd = accept(m_FD, &sa, &sa_len);
	if (fd < 0)
		return NULL;

	Client* client = CreateClient(Address(m_BindAddress, &sa, sa_len), m_BindAddress);
	if (client == NULL) {
		close(fd);
		return NULL;
	}
	client->SetFD(fd);
	return client;
}

/* vim:set ts=2 sw=2: */
