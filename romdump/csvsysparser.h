/*
 * Runes of Magic protocol analysis - CSV Sys_...name parsing code
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
#ifndef __CSVSYSPARSER_H__
#define __CSVSYSPARSER_H__

#include <map>
#include <string>

//! \brief Parses Sys..._name from a CSV file
class CSVSysParser
{
public:
	CSVSysParser();

	/*! \brief Loads strings from a CSV file
	 *  \param fname File to read
	 *  \returns true on success
	 */
	bool Load(const char* fname);

	/*! \brief Looks a given entry up
	 *  \param n Number to look up
	 *  \returns String found
	 *
	 *  This function will return a default entry if nothing was found.
	 */
	const std::string& Lookup(int n) const;

protected:
	typedef std::map<int, std::string> TintStringMap;
	TintStringMap m_Strings;

	//! \brief Default entry when nothing else is available
	std::string m_Default;
};

#endif /* __CSVSYSPARSER_H__ */
