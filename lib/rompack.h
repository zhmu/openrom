/*
 * Open Runes of Magic server - data packing/unpacking
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
#ifndef __ROMPACK_H__
#define __ROMPACK_H__

#include <stdint.h>

class ROMPack
{
public:
	/*! \brief Unpacks a ROMPack-compressed chunk
	 *  \param src Source data to unpack
	 *  \param srclen Number of bytes in source buffer
	 *  \param dst Destination buffer
	 *  \param outlen Set to number of bytes unpacked
	 *  \returns true on success
	 *
	 *  Note that there is currently no check on the output length (which
	 *  can grow quite large) - supplying a large buffer is recommended.
	 */
	static bool Unpack(const uint8_t* src, int srclen, uint8_t* dst, int* outlen);

	/*! \brief Packs data to a ROMPack-compressed chunk
	 *  \param src Source data to pack
	 *  \param srclen Number of bytes in source buffer
	 *  \param dst Destination buffer
	 *  \param outlen Set to number of bytes packed
	 *  \returns true on success
	 *
	 *  Note that the output length buffer can actually be larger than the
	 *  input buffer.
	 */
	static bool Pack(const uint8_t* src, int srclen, uint8_t* dst, int* outlen);
};

#endif /* __ROMPACK_H__*/
