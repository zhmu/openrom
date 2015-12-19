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
#ifndef __LOGGINGSYSTEM_H__
#define __LOGGINGSYSTEM_H__

#include <list>

class Logger;

//! \brief Handles the logging system itself
class LoggingSystem {
	friend class Logger;

public:
	//! \brief Constructor
	LoggingSystem();
	
	//! \brief Destructor
	~LoggingSystem();

	/*! \brief Retrieve a logger object
	 *  \param name Name of the logger to use
	 *  \returns Pointer to the new object
	 *
	 *  The logger will be created as necessary.
	 */
	Logger* Get(const char* name);

protected:
	/*! \brief Called by a logger when there is something to log
	 *  \param oLogger Logger requesting the log
	 *  \param sMessage String to log
	 */
	void PerformLog(Logger& oLogger, const char* sMessage);

private:
	typedef std::list<Logger*> TLoggerPtrList;
	TLoggerPtrList m_Logger;
	
};

extern LoggingSystem* g_LoggingSystem;

#endif /* __LOGGINGSYSTEM_H__ */
