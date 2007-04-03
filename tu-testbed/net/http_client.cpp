// http_client.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// HTTP implementation of tu_file

#include "base/utility.h"
#include "base/tu_file.h"
#include "net/http_client.h"

#define HTTP_TIMEOUT 10	// sec
//#define USE_PROXY

netfile::netfile(const char* c_url) :
	m_position(0),
	m_iface(NULL),
	m_ns(NULL),
	m_eof(false),
	m_buf(NULL),
	m_bufsize(0),
	m_size(0)
{
	m_ci.m_port = 80;

#ifdef USE_PROXY
	m_ci.m_proxy = "192.168.1.201";
	m_ci.m_proxy_port = 8080;
#else
	m_ci.m_proxy = "";
	m_ci.m_proxy_port = 0;
#endif

	char* url = strdup(c_url);

	// get host name from url
	// find the first '/'
	int i, n;
	for (i = 0, n = strlen(url); url[i] != '/' && i < n; i++);
	if (i == n)
	{
		// '/' is not found
		fprintf(stderr, "invalid url '%s'\n", url);
		free(url);
		return;
	}

	m_ci.m_uri = url + i;
	url[i] = 0;
	m_ci.m_host = url;
	free(url);
	url = NULL;

	// for debuging
//	m_fd = fopen("c:\\xxx.swf", "rb");
//	m_ns = (net_socket*)1;
//	return;

	// Open a socket to receive connections on.
	m_iface = tu_create_net_interface_tcp(&m_ci);
	if (m_iface == NULL)
	{
		fprintf(stderr, "Couldn't open net interface\n");
		return;
	}

	m_ns = m_iface->create_socket();

	if (open_uri() == false)
	{
		close();
	}
}

netfile::~netfile()
{
	close();
}

void netfile::close()
{
//	delete m_ns;
	m_ns = NULL;
	delete m_iface;
	m_iface = NULL;
	free(m_buf);
	m_buf = NULL;
//	fclose(m_fd);
}

int netfile::http_read(void* dst, int bytes, void* appdata) 
// Return the number of bytes actually read.  EOF or an error would
// cause that to not be equal to "bytes".
{
	netfile* nf = (netfile*) appdata;
	return nf->read(dst, bytes);
}

int netfile::read(void* dst, int bytes) 
{
	assert(dst);

	if (m_ns == NULL)
	{
		return 0;
	}

	// ensure buf
	while (m_bufsize < m_position + bytes)
	{
		m_bufsize += 4096;
		m_buf = (Uint8*) realloc(m_buf, m_bufsize);
	}

	// not enough data in the buffer
	if (m_position + bytes > m_size)
	{
		int n = m_ns->read(m_buf + m_size, m_position + bytes - m_size, HTTP_TIMEOUT);
//		int n = fread(m_buf + m_size, 1, m_position + bytes - m_size, m_fd);
		m_size += n;
	}

	int n = imin(bytes, m_size - m_position);
	memcpy(dst, m_buf + m_position, n);
	m_position += n;
	m_eof = n < bytes ? true : false;
	
//	printf("read %d %d %d\n", bytes, n, m_position);
	return n;
}

int netfile::http_tell(const void *appdata)
{
	assert(appdata);
	netfile* nf = (netfile*) appdata;
//	printf("tell %d\n", nf->m_position);
	return nf->m_position;
}

int netfile::http_close(void* appdata)
{
	assert(appdata);
	netfile* nf = (netfile*) appdata;
	delete nf;
	return 0;
}

int netfile::http_write(const void* src, int bytes, void* appdata)
// Return the number of bytes actually written.
{
	// todo
	assert(0);
	assert(appdata);
	assert(src);
	return 0;
}

int netfile::http_seek(int pos, void *appdata)
// Return 0 on success, or TU_FILE_SEEK_ERROR on failure.
{
	netfile* nf = (netfile*) appdata;
	return nf->seek(pos);
}

int netfile::seek(int pos)
// Return 0 on success, or TU_FILE_SEEK_ERROR on failure.
{
	if (m_ns == NULL)
	{
		return TU_FILE_SEEK_ERROR;
	}

	if (pos < m_position)
	{
		m_position = pos;
		m_eof = false;
	}
	else
	{
		int n = 1;
		while (m_position < pos && n == 1)
		{
			Uint8 b;
			n = read(&b, 1);
		}

	}
	//	printf("seek %d %d\n", pos, nf->m_position);
	return m_position == pos ? TU_FILE_NO_ERROR : TU_FILE_SEEK_ERROR;
}

int netfile::http_seek_to_end(void *appdata)
{
	assert(appdata);
	netfile* nf = (netfile*) appdata;

	int n = 0;
	do
	{
		Uint8 b;
		n = http_read(&b, 1, appdata);
	}
	while (n == 1);
	return 0;
}

bool netfile::http_get_eof(void *appdata)
// Return true if we're positioned at the end of the buffer.
{
	assert(appdata);
	netfile* nf = (netfile*) appdata;
	return nf->m_eof;
}

// read http response
bool netfile::read_response()
{
	bool rc = false;
	while (true)
	{
		tu_string s;
		int n = m_ns->read_line(&s, 100, HTTP_TIMEOUT);

		// end of header
		if (n <= 2)
		{
			break;
		}

		printf("%s", s.c_str());

		// set retcode
		if (strncmp(s.c_str(), "HTTP/", 5) == 0)
		{
			if (strncmp(s.c_str() + 9, "200", 3) == 0)
			{
				rc = true;
			}
		}
	}
	return rc;
}

bool netfile::open_uri()
{
	assert(m_ns);

	if (m_ci.m_proxy_port > 0)
	{
		char buf[80];
		sprintf(buf, "CONNECT %s:%d HTTP/1.0\r\n", m_ci.m_host.c_str(), m_ci.m_port);
		m_ns->write_string(buf, 1);
		m_ns->write_string("User-Agent:gameswf\r\n", 1);
		m_ns->write_string("Connection:Keep-Alive\r\n", 1);
		m_ns->write_string("\r\n", 1);
		if (read_response() == false)
		{
			return false;
		}
	}

	// We use HTTP/1.0 because we do not wont get chunked encoding data
	m_ns->write_string(tu_string("GET ") + m_ci.m_uri + tu_string(" HTTP/1.0\r\n"), 1);
	m_ns->write_string(tu_string("Host:") + m_ci.m_host + tu_string("\r\n"), 1);
//	m_ns->write_string("Accept:*\r\n", 1);
//	m_ns->write_string("Accept-Language: en\r\n", 1);
//	m_ns->write_string("Accept-Encoding: gzip, deflate, chunked\r\n", 1);
//	m_ns->write_string("Accept-Encoding: *\r\n", 1);
	m_ns->write_string("User-Agent:gameswf\r\n", 1);
	m_ns->write_string("Connection:Close\r\n", 1);
	m_ns->write_string("\r\n", 1);
 
	return read_response();
}

// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
