// net_interface_tcp.cpp -- Thatcher Ulrich http://tulrich.com 2005

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// TCP implementation of net_socket


#include "net/net_interface.h"
#include "base/tu_timer.h"
#include "base/container.h"


#ifdef _WIN32
#include <winsock.h>
#endif // _WIN32


struct net_socket_tcp : public net_socket
{
	
#ifdef _WIN32
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

	virtual int read(void* data, int bytes, float timeout_seconds)
	// Try to read the requested number of bytes.  Returns the
	// number of bytes actually read.
	{
		uint64 start = tu_timer::get_ticks();

		int total_bytes_read = 0;

		for (;;)
		{
			int bytes_read = recv(m_sock, (char*) data, bytes, 0);
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
			int bytes_read = recv(m_sock, &c, 1, 0);
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

					if (elapsed > timeout_seconds)
					{
						// TODO this spins; fix it by sleeping a bit?
						// if (time_left > 0.010f) { sleep(...); }
						continue;
					}
				
					return 0;
				}
				else
				{
					// Presumably something bad.
					fprintf(stderr, "net_socket_tcp::read() error in recv, error code = %d\n",
						WSAGetLastError());
					return 0;
				}
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
#endif // _WIN32

};


struct net_interface_tcp : public net_interface
{
	int m_port_number;

#ifdef _WIN32
	SOCKET m_listen_sock;
#endif // _WIN32
	
	net_interface_tcp(int port_number)
		:
		m_port_number(port_number)
	{
#ifdef _WIN32
		m_listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (m_listen_sock == INVALID_SOCKET)
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
		int ret = bind(m_listen_sock, (LPSOCKADDR) &saddr, sizeof(saddr));
		if (ret == SOCKET_ERROR)
		{
			fprintf(stderr, "bind failed\n");
			closesocket(m_listen_sock);
			m_listen_sock = INVALID_SOCKET;
			return;
		}


		// gethostname

		// Start listening.
		ret = listen(m_listen_sock, SOMAXCONN);
		if (ret == SOCKET_ERROR)
		{
			fprintf(stderr, "listen() failed\n");
			closesocket(m_listen_sock);
			m_listen_sock = INVALID_SOCKET;
			return;
		}

		// Set non-blocking mode for the socket, so that
		// accept() doesn't block if there's no pending
		// connection.
		int mode = 1;
		ioctlsocket(m_listen_sock, FIONBIO, (u_long FAR*) &mode);
#endif // _WIN32
	}

	~net_interface_tcp()
	{
#ifdef _WIN32
		closesocket(m_listen_sock);
#endif // _WIN32
	}


	bool is_valid() const
	{
#ifdef _WIN32
		if (m_listen_sock == INVALID_SOCKET)
		{
			return false;
		}
#endif // _WIN32

		return true;
	}

	net_socket* accept()
	{
#ifdef _WIN32
		// Accept an incoming request.
		SOCKET	remote_socket;
		remote_socket = ::accept(m_listen_sock, NULL, NULL);
		if (remote_socket == INVALID_SOCKET)
		{
			// No connection pending, or some other error.
			return NULL;
		}

		return new net_socket_tcp(remote_socket);
#else // not _WIN32
		// TODO implement
		return NULL;
#endif // not _WIN32
	}
};



net_interface* tu_create_net_interface_tcp(int port_number)
{
#ifdef _WIN32
	WORD version_requested = MAKEWORD(1, 1);
	WSADATA wsa;

	int ret = WSAStartup(version_requested, &wsa);

	if (wsa.wVersion != version_requested)
	{	
		// TODO log_error
		fprintf(stderr, "Bad Winsock version %d\n", wsa.wVersion);
		return NULL;
	}
#endif // WIN32


	net_interface_tcp* iface = new net_interface_tcp(port_number);
	if (iface == NULL || iface->is_valid() == false)
	{
		delete iface;
		return NULL;
	}

	return iface;
}

//#ifdef _WIN32
//		WSACleanup();
//#endif // WIN32



// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:

