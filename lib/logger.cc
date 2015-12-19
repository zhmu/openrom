/*
 * Open Runes of Magic server - logging functionality
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
#include "logger.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "loggingsystem.h"

Logger::Logger(LoggingSystem& oSystem, const char* name)
	: m_System(oSystem), m_Enabled(false)
{
	m_Name = strdup(name);
}

Logger::~Logger()
{
	free(m_Name);
}

void
Logger::_Log(const char* fmt, ...)
{
	char temp[1024];

	va_list va;
	va_start(va, fmt);
	vsnprintf(temp, sizeof(temp), fmt, va);
	temp[sizeof(temp) - 1] = '\0';
	va_end(va);

	m_System.PerformLog(*this, temp);
}

/* vim:set ts=2 sw=2: */
