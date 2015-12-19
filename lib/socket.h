/*
 * Runes of Magic proxy - socket interface
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
#ifndef __SOCKET_H__
#define __SOCKET_H__

class Socket
{
public:
	Socket();
	virtual ~Socket();

	/*! \brief Close the socket
	 *
	 *  This will do nothing if the socket isn't connected.  
	 */
	void Close();

	//! \brief Is the socket connected?
	bool IsConnected() const;

	//! \brief Retrieve the socket's backing file descriptor
	int GetFD() const;

	/*! \brief Reads data from the socket
	 *  \param buffer Buffer to fill
	 *  \param length Number of bytes to read
	 *  \returns Number of bytes read
	 */
	int Read(void* buffer, int length) const;

	/*! \brief Writes data to the socket
	 *  \param buffer Buffer to transmit
	 *  \param length Number of bytes to transmit
	 *  \returns Number of bytes transmitted
	 */
	int Write(const void* buffer, int length) const;

protected:
	//! \brief File descriptor of the socket
	int m_FD;
};

inline bool
Socket::IsConnected() const
{
	return m_FD >= 0;
}

inline int
Socket::GetFD() const
{
	return m_FD;
}


#endif /* __SOCKET_H__ */
