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
#include "csvsysparser.h"
#include <stdio.h>
#include <string.h>

#define LINE_MAX 1024

CSVSysParser::CSVSysParser()
	: m_Default("?")
{
}

bool
CSVSysParser::Load(const char* fname)
{
	FILE* f = fopen(fname, "rt");
	if (f == NULL)
		return false;

	while(true) {
		char line[LINE_MAX];
		line[LINE_MAX - 1] = '\0';
		if (fgets(line, LINE_MAX - 1, f) == NULL)
			break;

		// Remove the newline, if any
		{
			char* ptr = strchr(line, '\n');
			if (ptr != NULL)
				*ptr = '\0';
		}

		// The file must be in format 'identifier,string'; isolate both
		// parts
		char* string_content = strchr(line, ',');
		if (string_content == NULL)
			continue; // corrupt line
		*string_content++ = '\0';

		// We are only interested in 'Sys..._name' identifiers; reject any that do not match
		if (strncmp(line, "Sys", 3) != 0)
			continue;
		if (strlen(line) < 5 || strcmp(line + strlen(line) - 5, "_name") != 0)
			continue;

		char* ptr;
		unsigned int id = strtoul(line + 3, &ptr, 10);
		if (*ptr != '_')
			continue;

		// This item worked; store it
		m_Strings.insert(std::pair<int, std::string>(id, string_content));
	}
}

const std::string&
CSVSysParser::Lookup(int n) const
{
	auto it = m_Strings.find(n);
	if (it == m_Strings.end())
		return m_Default;
	return it->second;
}

/* vim:set ts=2 sw=2: */
