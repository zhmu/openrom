/*
 * Runes of Magic protocol analysis - protocol data transformation interface
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
#ifndef __DATA_TRANSFORMATION_H__
#define __DATA_TRANSFORMATION_H__

#include <stdint.h>

//! \brief Transformation interface
class XDataTransformation {
public:
	virtual ~XDataTransformation() { }

	/*! \brief Called when a transformation is to be applied
	 *  \param pSource Source to transform
	 *  \param iSourceLen Bytes to process
	 *  \param pDest Destination buffer
	 *  \param oDestLen Destination buffer size on input
	 *  \returns true on success
	 *
	 *  This function is to set oDestLen to the number of bytes placed in pDest on success.
	 */
	virtual bool Apply(const uint8_t* pSource, int iSourceLen, uint8_t* pDest, int& oDestLen) = 0;

	/*! \brief Called to guesstimate the destination buffer size
	 *  \param pSource Source that is to be guessed
	 *  \param iSourceLen Source length, in bytes
	 *  \returns Estimated size, or -1 if this cannot be decoded
	 *
	 *  The buffer must never be too small to contain the output.
	 */
	virtual int EstimateBufferSize(const uint8_t* pSource, int iSourceLen) = 0;
};

#endif /* __DATA_TRANSFORMATION_H__ */
