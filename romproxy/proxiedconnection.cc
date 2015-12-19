/*
 * Runes of Magic proxy - describes a proxied connection between a local and remote peer
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
#include "proxiedconnection.h"
#include <stdio.h>

ProxiedConnection::ProxiedConnection(const Address& clientaddr, const Address& remoteaddr)
	: Client(clientaddr, remoteaddr)
{
	m_remoteclient = new Client(remoteaddr, clientaddr);
}

ProxiedConnection::~ProxiedConnection()
{
	delete m_remoteclient;
}

/* vim:set ts=2 sw=2: */
