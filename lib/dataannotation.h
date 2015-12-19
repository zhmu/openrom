/*
 * Runes of Magic protocol analysis - protocol data annotation interface
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
#ifndef __DATA_ANNOTATION_H__
#define __DATA_ANNOTATION_H__

#include <stdint.h>
#include "protocoldefinition.h" // XXX can't forward declare nested classes

//! \brief Annotation interface
class XDataAnnotation {
public:
	virtual ~XDataAnnotation() { }

	/*! \brief Called when an annotation is to be applied
	 *  \param value Value to look up
	 *  \returns Annotation result
	 */
	virtual const char* Lookup(uint32_t value) = 0;

	/*! \brief Applies an annotation action
	 *  \param pType Current type node
	 */
	virtual void Apply(ProtocolDefinition::Type* pType) { }
};

#endif /* __DATA_ANNOTATION_H__ */
