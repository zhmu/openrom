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
#ifndef __BUFFER_H__
#define __BUFFER_H__

#include <stdint.h>

//! \brief Buffer for data of either side
class Buffer {
public:
	Buffer();
	~Buffer();

	/*! \brief Adds data to the buffer
	 *  \param data Data to add
	 *  \param len Number of bytes to add
	 *  \returns true on success
	 *
	 *  This function will fail if the current buffer is too full
	 */
	bool AddData(const void* data, int len);

	/*! \brief Flushes data from the buffer
	 *  \param len Number of bytes to flush
	 *
	 *  The caller must nsure that there is at least this amount of
	 *  data in the buffer.
	 */
	void FlushData(int len);

	/*! \brief Retrieve (a port of) data
	 *  \param len Length to retrieve
	 *  \returns Pointer to the data, or NULL if not available
	 *
	 *  Peeking at data will not be removed from the buffer; call
	 *  FlushData() for that.
	 */
	const uint8_t* PeekData(int len) const;

	/*! \brief Retrieve the amount of data available
	 *  \returns Data available, in bytes
	 */
	int GetAmountOfDataAvailable() const;

private:
	//! \brief Buffer size for both local and remote side
	static const unsigned int s_BufferSize = 262144;

	//! \brief Data
	uint8_t* m_Data;

	//! \brief Current amount of data available
	int m_DataAvailable;
};

inline int
Buffer::GetAmountOfDataAvailable() const
{
	return m_DataAvailable;
}

#endif /* __BUFFER_H__ */
