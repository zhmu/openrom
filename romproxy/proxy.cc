/*
 * Runes of Magic proxy - proxy
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
#include <stdio.h> // for NULL
#include "proxy.h"
#include "proxyserver.h"

Proxy::Proxy(const Address& local, const Address& remote)
	: m_local(local), m_remote(remote)
{
	m_server = new ProxyServer(*this);
}

Proxy::~Proxy()
{
	delete m_server;
}

bool
Proxy::Initialize()
{
	if (!m_server->Listen(m_local))
		return false;

	return true;
}

int
Proxy::FillFdSet(int max_fd, fd_set* fds) const
{
	FD_SET(m_server->GetFD(), fds);
	if (m_server->GetFD() > max_fd)
		max_fd = m_server->GetFD();

	for (auto it = m_connections.begin(); it != m_connections.end(); it++) {
		ProxiedConnection& connection = **it;

		// Add both the connection and the connection it uses itself
		int fd1 = connection.GetLocalClient().GetFD();
		int fd2 = connection.GetRemoteClient().GetFD();
		FD_SET(fd1, fds);
		FD_SET(fd2, fds);

		// Keep track of largest FD
		if (fd1 > max_fd)
			max_fd = fd1;
		if (fd2 > max_fd)
			max_fd = fd2;
	}
	return max_fd;
}

bool
Proxy::ProcessFdSet(const fd_set* fds)
{
	if (FD_ISSET(m_server->GetFD(), fds)) {
		// A new connection approaches!
		m_server->Accept();
	}

	// See if any connection has something to report	
	auto it = m_connections.begin();
	while (it != m_connections.end()) {
		ProxiedConnection& connection = **it;

		// Allow the local and remote parts of the connection to handle the event
		bool ok = true;
		if (FD_ISSET(connection.GetLocalClient().GetFD(), fds))
			ok &= connection.OnLocalEvent();
		if (FD_ISSET(connection.GetRemoteClient().GetFD(), fds))
			ok &= connection.OnRemoteEvent();

		// Close the connection if needed
		if (!ok) {
			delete *it;
			it = m_connections.erase(it);
		} else
			it++;
	}

	// For now, never give up...
	return true;
}

void
Proxy::AddConnection(ProxiedConnection& connection)
{
	m_connections.push_back(&connection);
}

Proxy::ProxyServer::ProxyServer(Proxy& proxy)
	: m_proxy(proxy)
{
}

Client*
Proxy::ProxyServer::CreateClient(const Address& localaddr, const Address& remoteaddr) const
{
	ProxiedConnection* connection = m_proxy.CreateConnection(localaddr, remoteaddr);
	if (connection == NULL || !connection->GetRemoteClient().Connect(m_proxy.GetRemoteAddress())) {
		delete connection;
		return NULL;
	}
	connection->Ready();
	m_proxy.AddConnection(*connection);
	return connection;
}

/* vim:set ts=2 sw=2: */
