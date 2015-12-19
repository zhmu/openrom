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
#ifndef __SERVER_H__
#define __SERVER_H__

#include "address.h"
#include "socket.h"

class Client;

class Server : public Socket {
public:
	/*! \brief Listens on a given address
	 *  \param address Address to listen on
	 *  \returns true on success
	 */
	bool Listen(const Address& address);

	/*! \brief Accepts a new connection
	 *  \returns Client, or NULL on failure
	 *
	 *  This will block if no pending new connections are available
	 */
	Client* Accept();

protected:
	//! \brief Called to create a new client object
	virtual Client* CreateClient(const Address& localaddr, const Address& remoteaddr) const = 0;

	//! \brief Address the server is bound to
	Address m_BindAddress;
};

#endif /*  __SERVER_H__ */
