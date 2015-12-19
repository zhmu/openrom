/*
 * Runes of Magic proxy - proxying utility
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
#ifndef __ROMPROXY_H__
#define __ROMPROXY_H__

#include <list>

class Proxy;
class ROMPacketLogger;

//! \brief Main ROM proxy
class ROMProxy {
public:
	ROMProxy();

	/*! \brief Runs the proxy
	 *  \param argc Argument count
	 *  \param argv Arguments
	 *  \returns Return value (to be passed to exit())
	 */
	int Run(int argc, char** argv);

	//! \brief Request the proxy to quit
	void RequestQuit();

	//! \brief Retrieve the logger, if any
	ROMPacketLogger* GetLogger() const;

	/*! \brief Are we to log proxy<->server traffic?
	 *  \returns true if so
	 */
	bool MustLogProxyServerTraffic() const;

	/*! \brief Are we to log proxy<->client traffic?
	 *  \returns true if so
	 */
	bool MustLogProxyClientTraffic() const;

	/*! \brief Retrieves the next address to bind to
	 */
	Address GetNextBindAddress();

	/*! \brief Find or create a proxy for a given address
	 *  \returns Proxy to use, or NULl if one cannot be created
	 *
	 *  The proxy will be created as necessary.
	 */
	Proxy* GetProxyForAddress(const Address& address);

	//! \brief Retrieve the debug level
	int GetDebugLevel() const;
	
protected:
	/*! \brief Displays usage information
	 *  \param progname Program name to print
	 */
	static void usage(const char* progname);

private:
	typedef std::list<Proxy*> TProxyPtrList;

	//! \brief All proxies in use
	TProxyPtrList m_proxies;

	//! \brief Logger in use
	ROMPacketLogger* m_logger;

	//! \brief Current bind address
	Address m_BindAddress;

	//! \brief Log proxy<->client traffic
	bool m_LogProxyClient;

	//! \brief Log proxy<->server traffic
	bool m_LogProxyServer;

	//! \brief Debug level
	int m_DebugLevel;

	//! \brief Do we need to quit?
	bool m_quit;
};

inline void
ROMProxy::RequestQuit()
{
	m_quit = true;
}

inline ROMPacketLogger*
ROMProxy::GetLogger() const
{
	return m_logger;
}

inline bool
ROMProxy::MustLogProxyServerTraffic() const
{
	return m_LogProxyServer;
}

inline bool
ROMProxy::MustLogProxyClientTraffic() const
{
	return m_LogProxyClient;
}

inline int
ROMProxy::GetDebugLevel() const
{
	return m_DebugLevel;
}

extern ROMProxy* g_ROMProxy;

#endif /* __ROMPROXY_H__ */
