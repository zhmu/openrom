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
#include <err.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <list>
#include "address.h"
#include "rompacketlogger.h"
#include "romproxy.h"
#include "gameproxy.h"
#include "loginproxy.h"

ROMProxy* g_ROMProxy = NULL;

void
ROMProxy::usage(const char* progname)
{
	fprintf(stderr, "usage: %s [-h?cs] [-b ip[:port]] [-d level] [-l log.rom] loginserver:port\n", progname);
	fprintf(stderr, "\n");
	fprintf(stderr, "  -h, -?             this help\n");
	fprintf(stderr, "  -b ip:port         bind to the given hostname:service\n");
	fprintf(stderr, "  -c                 log client <-> proxy traffic\n");
	fprintf(stderr, "  -d level           set debug level\n");
	fprintf(stderr, "  -s                 log server <-> proxy traffic\n");
	fprintf(stderr, "  -l log.rom         log packets to log.rom\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "loginserver:port is the login server to proxy\n");
	fprintf(stderr, "If neither -c nor -s is supplied, -c will be assumed\n");
}

ROMProxy::ROMProxy()
	: m_quit(false), m_LogProxyServer(false), m_LogProxyClient(false), m_DebugLevel(0)
{
}

static void
sigint(int)
{
	g_ROMProxy->RequestQuit();
}

Address
ROMProxy::GetNextBindAddress()
{
	uint32_t ip;
	uint16_t port;
	m_BindAddress.GetIPv4Address(ip, port);
	m_BindAddress.SetPort(port + 1);

	Address next_addr(m_BindAddress);
	return next_addr;
}

int
ROMProxy::Run(int argc, char** argv)
{
	char* bind_addr = NULL;
	char* log_file = NULL;
	{
		int opt;
		while ((opt = getopt(argc, argv, "?hb:cd:l:s")) != -1) {
			switch(opt) {
				case 'h':
				case '?':
					usage(argv[0]);
					return EXIT_FAILURE;
				case 'b':
					bind_addr = optarg;
					break;
				case 'c':
					m_LogProxyClient = true;
					break;
				case 'd': {
					char* ptr;
					m_DebugLevel = strtoul(optarg, &ptr, 10);
					if (*ptr != '\0')
						err(1, "cannot parse debug level");
					break;
				}
				case 's':
					m_LogProxyServer = true;
					break;
				case 'l':
					log_file = optarg;
					break;
			}
		}
	}

	if (optind != argc - 1) {
		fprintf(stderr, "missing loginserver:port to use\n");
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	// Look up the login server to use; and if we can connect to anything
	Address loginserver_address;
	char* loginserver_port = NULL;
	{
		char* colon = strchr(argv[optind], ':');
		if (colon == NULL)
			errx(1, "missing : in loginserver:port pair");
		*colon = '\0';
		loginserver_port = colon + 1;

		if (!loginserver_address.Resolve(argv[optind], loginserver_port))
			errx(1, "cannot resolve login server address");
	}

	// Create the address on which we will bind as login server
	Address localserver_address;
	{
		const char* bind_host = NULL;
		const char* bind_port = loginserver_port;
		if (bind_addr != NULL) {
			// Isolate bind_addr in bind_host:bind_port
			char* colon = strchr(bind_addr, ':');
			if (colon != NULL) {
				*colon = '\0';
				bind_port = colon + 1;
			}
			bind_host = bind_addr;
		}
		if (!localserver_address.Resolve(bind_host, bind_port))
			errx(1, "cannot resolve address to bind to");
	}

	// Set up logging
	if (log_file != NULL) {
		m_logger = new ROMPacketLogger;
		if (!m_logger->Open(log_file))
			err(1, "cannot create logfile");

		// Default to log traffic between client<->proxy
		if (!m_LogProxyServer && !m_LogProxyClient)
			m_LogProxyClient = true;
	}

	LoginProxy loginproxy(localserver_address, loginserver_address);
	if (!loginproxy.Initialize())
			err(1, "cannot create local server");
	m_BindAddress = localserver_address;

	m_proxies.push_back(&loginproxy);

	signal(SIGINT, sigint);
	while(!m_quit) {
		fd_set fds;

		int max_fd = 0;
		FD_ZERO(&fds);
		for (auto it = m_proxies.begin(); it != m_proxies.end(); it++)
			max_fd = (*it)->FillFdSet(max_fd, &fds);

		// Wait until a file descriptor wants our attention
		if (select(max_fd + 1, &fds, NULL, NULL, NULL) < 0) {
			if (errno == EINTR)
				continue;
			err(1, "select");
		}

		{
			auto it = m_proxies.begin();
			while (it != m_proxies.end()) {
				Proxy* proxy = *it;
				if (!proxy->ProcessFdSet(&fds)) {
					delete proxy;
					it = m_proxies.erase(it);
				} else
					it++;
			}
		}
	}

	delete m_logger;
	m_logger = NULL;
	return 0;
}

Proxy*
ROMProxy::GetProxyForAddress(const Address& address)
{
	for (auto it = m_proxies.begin(); it != m_proxies.end(); it++) {
	 	Proxy& proxy = **it;
		if (proxy.GetRemoteAddress() == address)
			return &proxy;
	}

	// Not found; make a new one
	GameProxy* proxy = new GameProxy(GetNextBindAddress(), address);
	if (!proxy->Initialize()) {
		delete proxy;
		return NULL;
	}

	if (GetDebugLevel() > 1) {
		char tmp[64];
		address.ToString(tmp, sizeof(tmp));
		fprintf(stderr, "ROMProxy::GetProxyForAddress(): created new proxy for %s\n", tmp);
	}
	m_proxies.push_back(proxy);
	return proxy;
}

int
main(int argc, char** argv)
{
	g_ROMProxy = new ROMProxy;
	int ret = g_ROMProxy->Run(argc, argv);
	delete g_ROMProxy;

	return ret;
}

/* vim:set ts=2 sw=2: */
