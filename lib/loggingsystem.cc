/*
 * Open Runes of Magic server - logging system
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
#include "loggingsystem.h"
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "logger.h"

LoggingSystem* g_LoggingSystem = NULL;

LoggingSystem::LoggingSystem()
{
}

LoggingSystem::~LoggingSystem()
{
	for (auto it = m_Logger.begin(); it != m_Logger.end(); it++)
		delete *it;
}

Logger*
LoggingSystem::Get(const char* name)
{
	// XXX lock
	for (auto& pLogger: m_Logger) {
		if (strcmp(pLogger->GetName(), name) != 0)
			continue;

		// Found the logger; just return it
		return pLogger;
	}

	// Not found; we need to make a new logger
	Logger* pLogger = new Logger(*this, name);
	m_Logger.push_back(pLogger);
	return pLogger;
}

void
LoggingSystem::PerformLog(Logger& oLogger, const char* sMessage)
{
	// XXX lock, threadsafe time functions
	struct timeval tv;
	gettimeofday(&tv, NULL);

	struct tm* tm = localtime(&tv.tv_sec);
	fprintf(stderr, "[%02u:%02u:%02u.%03u - %s] - %s\n",
	 tm->tm_hour, tm->tm_min, tm->tm_sec, tv.tv_usec / 1000, oLogger.GetName(), sMessage);
}

/* vim:set ts=2 sw=2: */
