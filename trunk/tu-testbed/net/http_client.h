// http_client.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// HTTP implementation of tu_file

#ifdef _WIN32
#include "Winsock.h"
typedef void siginfo_t;
typedef int socklen_t;                   
#define EINPROGRESS 0
#define EALREADY 0
#define strdup _strdup
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdarg.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#endif

#include "base/container.h"
#include "net/net_interface.h"

struct netfile
{
	// Return the number of bytes actually read.  EOF or an error would
	// cause that to not be equal to "bytes".

	netfile(const char* url);
	~netfile();

	static int http_read(void* dst, int bytes, void* appdata);
	static int http_tell(const void *appdata);
	static int http_close(void *appdata);
	static int http_write(const void* src, int bytes, void* appdata);
	static int http_seek(int pos, void *appdata);
	static int http_seek_to_end(void *appdata);
	static bool http_get_eof(void *appdata);

	private:

	int read(void* dst, int bytes);
	int seek(int pos);
	void close();
	bool read_response();
	bool open_uri();

	// vars
	int m_position;
	net_interface* m_iface;
	net_socket* m_ns;
	bool m_eof;
	connect_info m_ci;

	// We use set_position(back) therefore we need to use buf to store input data
	Uint8* m_buf;		// data
	int m_bufsize;	// current bufsize
	int m_size;			// data size

//	FILE* m_fd;	// for debuging
};

// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
