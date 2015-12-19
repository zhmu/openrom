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
#ifndef __LOGGER_H__
#define __LOGGER_H__

class LoggingSystem;

//! \brief Implements a logging producer
class Logger {
	friend class LoggingSystem;

public:
	//! \brief Enables logging
	void Enable() { m_Enabled = true; }

	//! \brief Disables logging
	void Disable() { m_Enabled = false; }

	/* Functions below are intended to be used by the LOG() macro */

	//! \brief Is the logger enabled?
	bool _IsEnabled() const { return m_Enabled; }

	//! \brief Retrieve the logger name
	const char* GetName() const { return m_Name; }

	//! \brief Performs logging
	void _Log(const char* fmt, ...);

protected:
	Logger() = delete;
	Logger(const Logger& oLogger) = delete;

	/*! \brief Constructor
	 *
	 *  This is only to be used by the LoggingSystem
	*/
	Logger(LoggingSystem& oSystem, const char* name);

	//! \brief Destroys the logger
	~Logger();

private:
	//! \brief Logger name
	char* m_Name;

	//! \brief Enabled state
	bool m_Enabled;

	//! \brief Logging system we belong to
	LoggingSystem& m_System;
};

#define LOG(l, a...) \
	(l)->_IsEnabled() ? (l)->_Log(a) : (void)0

#endif /* __LOGGER_H__ */
