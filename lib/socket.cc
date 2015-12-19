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
#include "socket.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>
#include <unistd.h>

Socket::Socket()
	: m_FD(-1)
{
}

Socket::~Socket()
{
	Close();
}

void
Socket::Close()
{
	if (!IsConnected())
		return;

	close(m_FD);
	m_FD = -1;
}

int
Socket::Read(void* buffer, int length) const
{
	assert(IsConnected());
	return recv(m_FD, buffer, length, 0);
}

int
Socket::Write(const void* buffer, int length) const
{
	assert(IsConnected());
	return write(m_FD, buffer, length);
}

/* vim:set ts=2 sw=2: */
