/*
 * Runes of Magic protocol analysis - flow buffer
 * Copyright (C) 2013-2015 Rink Springer <rink@rink.nu>
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
#include "flow.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "romstate.h"

Flow::Flow(const Connection& oConnection, unsigned int iDataBufferSize)
 : m_Connection(oConnection), m_DataBufferSize(iDataBufferSize), m_DataBufferUsed(0), m_CurrentDataOffset(0)
{
	assert(m_DataBufferSize > 0);
	m_Data = new char[m_DataBufferSize];
}

Flow::~Flow()
{
	delete[] m_Data;
}

void
Flow::Append(const char* pBuffer, int iLength)
{
	while (m_DataBufferUsed + iLength > m_DataBufferSize) {
		// No fit; need to resize
		char* pNewBuffer = new char[m_DataBufferSize * 2];
		memcpy(pNewBuffer, m_Data, m_DataBufferUsed);
		delete[] m_Data;
		m_Data = pNewBuffer;

		m_DataBufferSize *= 2;
	}

	memcpy(m_Data + m_DataBufferUsed, pBuffer, iLength);
	m_DataBufferUsed += iLength;
}

/* vim:set ts=2 sw=2: */
