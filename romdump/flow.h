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
#ifndef __FLOW_H__
#define __FLOW_H__

#include <list>
#include <map>
#include "connection.h"
#include "types.h"

class Flow {
	friend class TCPFlows;
public:
	Flow(const Connection& oConnection, unsigned int iDataBufferSize);
	~Flow();

	const Connection& GetConnection() { return m_Connection; }
	unsigned int GetDataLength() const { return m_DataBufferUsed; }
	const char* GetData() const { return m_Data; }
	void Append(const char* pBuffer, int iLength);

	unsigned int& CurrentDataOffset() { return m_CurrentDataOffset; }

private:
	const Connection& m_Connection;
	char* m_Data;
	unsigned int m_DataBufferSize;
	unsigned int m_DataBufferUsed;
	unsigned int m_CurrentDataOffset;
};

#endif /* __FLOW_H__*/
