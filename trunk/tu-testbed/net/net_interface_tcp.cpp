// net_interface_tcp.cpp -- Thatcher Ulrich http://tulrich.com 2005

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// TCP implementation of net_socket


#include "net/net_interface.h"
#include "base/tu_timer.h"
#include "base/container.h"

#ifdef _WIN32

#include <winsock.h>

struct net_socket_tcp : public net_socket
{
	SOCKET m_sock;
	int m_error;
	
	net_socket_tcp(SOCKET sock)
		:
		m_sock(sock),
		m_error(0)
	{
	}

	~net_socket_tcp()
	{
		closesocket(m_sock);
	}

	virtual int get_error() const
	{
		return m_error;
	}

	virtual bool is_open() const
	{
		return get_error() == 0;
	}

	virtual bool is_readable() const
	// Return true if this socket has incoming data available.
	{
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(m_sock, &fds);
		struct timeval tv = { 0, 0 };
		select(1, &fds, NULL, NULL, &tv);
		if (FD_ISSET(m_sock, &fds)) {
			// Socket has data available.
			return true;
		}

		return false;
	}

	virtual int read(void* data, int bytes, float timeout_seconds)
	// Try to read the requested number of bytes.  Returns the
	// number of bytes actually read.
	{
		uint64 start = tu_timer::get_ticks();

		int total_bytes_read = 0;

		for (;;)
		{
			int bytes_read = recv(m_sock, (char*) data, bytes, 0);

			if (bytes_read == 0)
			{
				break;
			}

			if (bytes_read == SOCKET_ERROR)
			{
				m_error = WSAGetLastError();
				if (m_error == WSAEWOULDBLOCK)
				{
					// No data ready.
					m_error = 0;

					// Timeout?
					uint64 now = tu_timer::get_ticks();
					double elapsed = tu_timer::ticks_to_seconds(now - start);

					if (elapsed < timeout_seconds)
					{
						// TODO this spins; fix it by sleeping a bit?
						// if (time_left > 0.010f) { sleep(...); }
						continue;
					}

					// Timed out.
					return total_bytes_read;
				}
				else
				{
					// Presumably something bad.
					fprintf(stderr, "net_socket_tcp::read() error in recv, error code = %d\n",
						WSAGetLastError());
					return total_bytes_read;
				}
			}

			// Got some data.
			total_bytes_read += bytes_read;
			bytes -= bytes_read;
			data = (void*) (((char*) data) + bytes_read);

			assert(bytes >= 0);

			if (bytes == 0)
			{
				// Done.
				break;
			}
		}

		return total_bytes_read;
	}

	
	virtual int read_line(tu_string* data, int maxbytes, float timeout_seconds)
	// Try to read a string, up to the next '\n' character.
	// Returns the actual bytes read.
	//
	// If the last character in the string is not '\n', then
	// either we read maxbytes, or we timed out or the socket
	// closed or something.
	{
		assert(data);
		
		uint64 start = tu_timer::get_ticks();
		int total_bytes_read = 0;

		for (;;)
		{
			// Read a byte at a time.  Probably slow!
			char c;
			bool waiting = false;
			int bytes_read = recv(m_sock, &c, 1, 0);
			if (bytes_read == SOCKET_ERROR)
			{
				m_error = WSAGetLastError();
				if (m_error == WSAEWOULDBLOCK)
				{
					// No data ready.
					m_error = 0;
					waiting = true;
				}
				else
				{
					// Presumably something bad.
					fprintf(stderr, "net_socket_tcp::read() error in recv, error code = %d\n",
						WSAGetLastError());
					return 0;
				}
			} else if (bytes_read == 0) {
				waiting = true;
			}

			if (waiting) {
				// Timeout?
				uint64 now = tu_timer::get_ticks();
				double elapsed = tu_timer::ticks_to_seconds(now - start);

				if (elapsed < timeout_seconds)
				{
					// TODO this spins; fix it by sleeping a bit?
					// if (time_left > 0.010f) { sleep(...); }
					continue;
				}

				// Timed out.
				return 0;
			}

			assert(bytes_read == 1);

			(*data) += c;
			total_bytes_read += 1;

			if (c == '\n')
			{
				// Done.
				return total_bytes_read;
			}

			if (maxbytes && total_bytes_read >= maxbytes)
			{
				// Caller doesn't want any more bytes.
				return total_bytes_read;
			}

			// else keep reading.
		}

		return 0;
	}


	virtual int write(const void* data, int bytes, float timeout_seconds)
	// Try to write the given data buffer.  Return the number of
	// bytes actually written.
	{
		uint64 start = tu_timer::get_ticks();

		int total_bytes_written = 0;
		
		for (;;)
		{
			int bytes_sent = send(m_sock, (const char*) data, bytes, 0);
			if (bytes_sent == SOCKET_ERROR)
			{
				m_error = WSAGetLastError();
				if (m_error == WSAENOBUFS || m_error == WSAEWOULDBLOCK)
				{
					// Non-fatal.
					m_error = 0;

					uint64 now = tu_timer::get_ticks();
					double elapsed = tu_timer::ticks_to_seconds(now - start);

					if (elapsed < timeout_seconds)
					{
						// Keep trying.
						// TODO possibly sleep() here
						continue;
					}

					// Timed out.
					return total_bytes_written;
				}
			}

			total_bytes_written += bytes_sent;
			data = (const void*) (((const char*) data) + bytes_sent);
			bytes -= bytes_sent;

			assert(bytes >= 0);
			if (bytes == 0)
			{
				// Done.
				return total_bytes_written;
			}
			// else keep trying.
		}
	}
	
#if 0
	//
	// Send data back to the client
	//
	strcpy(szBuf, "From the Server");
	nRet = send(remoteSocket,				// Connected socket
				szBuf,						// Data buffer
				strlen(szBuf),				// Lenght of data
				0);							// Flags

	closesocket(remoteSocket);
#endif // 0
};


struct net_interface_tcp : public net_interface
{
	int m_port_number;
	SOCKET m_socket;

	//	server
	net_interface_tcp(int port_number) :
		m_port_number(port_number),
		m_socket(INVALID_SOCKET)
	{
		m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (m_socket == INVALID_SOCKET)
		{
			fprintf(stderr, "can't open listen socket\n");
			return;
		}

		// Set the address.
		SOCKADDR_IN saddr;
		saddr.sin_family = AF_INET;
		saddr.sin_addr.s_addr = INADDR_ANY;
		saddr.sin_port = htons(m_port_number);

		// bind the address
		int ret = bind(m_socket, (LPSOCKADDR) &saddr, sizeof(saddr));
		if (ret == SOCKET_ERROR)
		{
			fprintf(stderr, "bind failed\n");
			closesocket(m_socket);
			m_socket = INVALID_SOCKET;
			return;
		}

		// gethostname

		// Start listening.
		ret = listen(m_socket, SOMAXCONN);
		if (ret == SOCKET_ERROR)
		{
			fprintf(stderr, "listen() failed\n");
			closesocket(m_socket);
			m_socket = INVALID_SOCKET;
			return;
		}

		// Set non-blocking mode for the socket, so that
		// accept() doesn't block if there's no pending
		// connection.
		int mode = 1;
		ioctlsocket(m_socket, FIONBIO, (u_long FAR*) &mode);
	}

	// client
	net_interface_tcp(const connect_info* ci) :	m_socket(INVALID_SOCKET)
	{
		assert(ci);

		m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (m_socket == INVALID_SOCKET)
		{
			fprintf(stderr, "can't open listen socket\n");
			return;
		}

		// Set the address.
		SOCKADDR_IN saddr;
		saddr.sin_family = AF_INET;
		saddr.sin_addr.s_addr = INADDR_ANY;
		m_port_number = ci->m_proxy_port > 0 ? ci->m_proxy_port : ci->m_port;
		saddr.sin_port = htons(m_port_number);

		hostent* he;
		const char* host = ci->m_proxy_port > 0 ? ci->m_proxy.c_str() : ci->m_host.c_str();
		if (host[0] >= '0' && host[0] <= '9')	// absolue address ?
		{
			Uint32 addr = inet_addr(host);
			he = gethostbyaddr((char *) &addr, 4, AF_INET);
		}
		else
		{
			he = gethostbyname(host);
		}

		if (he == NULL)
		{
			fprintf(stderr, "can't find '%s'\n", host);
			closesocket(m_socket);
			m_socket = INVALID_SOCKET;
			return;
		}

		// get server address
		memcpy(&saddr.sin_addr, he->h_addr, he->h_length);

		//      printf("The IP address is '%d.%d.%d.%d'\n", 
		//				saddr.sin_addr.S_un.S_un_b.s_b1,
		//				saddr.sin_addr.S_un.S_un_b.s_b2,
		//				saddr.sin_addr.S_un.S_un_b.s_b3,
		//				saddr.sin_addr.S_un.S_un_b.s_b4);

		int rc = connect(m_socket, (struct sockaddr*) &saddr, sizeof(struct sockaddr));
		if (rc != 0)
		{
			fprintf(stderr, "can't connect to '%s'\n", host);
			closesocket(m_socket);
			m_socket = INVALID_SOCKET;
		}


		// Set non-blocking mode for the socket, so that
		// accept() doesn't block if there's no pending
		// connection.
		int mode = 1;
		ioctlsocket(m_socket, FIONBIO, (u_long FAR*) &mode);
	}

	~net_interface_tcp()
	{
		closesocket(m_socket);
	}

	bool is_valid() const
	{
		if (m_socket == INVALID_SOCKET)
		{
			return false;
		}

		return true;
	}

	net_socket* create_socket()
	{
		return new net_socket_tcp(m_socket);
	}

	net_socket* accept()
	{
		// Accept an incoming request.
		SOCKET	remote_socket;
		remote_socket = ::accept(m_socket, NULL, NULL);
		if (remote_socket == INVALID_SOCKET)
		{
			// No connection pending, or some other error.
			return NULL;
		}

		return new net_socket_tcp(remote_socket);
		// TODO implement
		return NULL;
	}
};

bool net_init()
{
	WORD version_requested = MAKEWORD(1, 1);
	WSADATA wsa;

	int ret = WSAStartup(version_requested, &wsa);

	if (wsa.wVersion != version_requested)
	{	
		// TODO log_error
		fprintf(stderr, "Bad Winsock version %d\n", wsa.wVersion);
		return false;
	}
	return true;
}

// server
net_interface* tu_create_net_interface_tcp(int port_number)
{
	if (net_init())
	{
		net_interface_tcp* iface = new net_interface_tcp(port_number);
		if (iface == NULL || iface->is_valid() == false)
		{
			delete iface;
			return NULL;
		}
		return iface;
	}
	return NULL;
}

// client
net_interface* tu_create_net_interface_tcp(const connect_info* ci)
{
	if (net_init())
	{
		net_interface_tcp* iface = new net_interface_tcp(ci);
		if (iface == NULL || iface->is_valid() == false)
		{
			delete iface;
			return NULL;
		}
		return iface;
	}
	return NULL;
}


//		WSACleanup();


#else  // not _WIN32


// ...


#endif  // not _WIN32


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:

