/*
 * Runes of Magic proxy - standard FIFO buffer
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
#include "buffer.h"
#include <assert.h>
#include <stdio.h>
#include <string.h> // for memcpy() and NULL

Buffer::Buffer()
{
	m_Data = new uint8_t[s_BufferSize];
	m_DataAvailable = 0;
}

Buffer::~Buffer()
{
	delete[] m_Data;
}

bool
Buffer::AddData(const void* data, int len)
{
	if (m_DataAvailable + len >= s_BufferSize)
		return false;

	memcpy(&m_Data[m_DataAvailable], data, len);
	m_DataAvailable += len;
	return true;
}

void
Buffer::FlushData(int len)
{
	assert(m_DataAvailable >= len);

	unsigned int left = m_DataAvailable - len;
	memmove(&m_Data[0], &m_Data[len], left);
	m_DataAvailable = left;
}
	
const uint8_t*
Buffer::PeekData(int len) const
{
	if (m_DataAvailable < len)
		return NULL;
	return &m_Data[0];
}

/* vim:set ts=2 sw=2: */
