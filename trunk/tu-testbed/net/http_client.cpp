// http_client.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// HTTP implementation of tu_file

#include "base/utility.h"
#include "base/tu_file.h"
#include "net/http_client.h"

#define HTTP_PORT 80

netfile::netfile(const char* c_url) :
	m_position(0),
	m_iface(NULL),
	m_ns(NULL),
	m_size(0)
{
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

	tu_string uri = url + i;
	url[i] = 0;
	tu_string host = url;
	free(url);
	url = NULL;

	// Open a socket to receive connections on.
	m_iface = tu_create_net_interface_tcp(host, HTTP_PORT);
	if (m_iface == NULL)
	{
		fprintf(stderr, "Couldn't open net interface\n");
		return;
	}

	m_ns = m_iface->create_socket();

	if (open_uri(host, uri) == false)
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
	delete m_ns;
	m_ns = NULL;
	delete m_iface;
	m_iface = NULL;
}

int netfile::http_read(void* dst, int bytes, void* appdata) 
// Return the number of bytes actually read.  EOF or an error would
// cause that to not be equal to "bytes".
{
	assert(appdata);
	assert(dst);
	netfile* nf = (netfile*) appdata;

	int n = 0;
	if (nf->m_ns)
	{
		n = nf->m_ns->read(dst, bytes, 1);
		nf->m_position += n;
	}
	return n;
}

int netfile::http_tell(const void *appdata)
{
	assert(appdata);
	netfile* nf = (netfile*) appdata;
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
	assert(appdata);
	netfile* nf = (netfile*) appdata;

	if (nf->m_ns)
	{
		int i;
		int n = 1;
		for (i = nf->m_position; i < pos && n == 1; i++)
		{
			Uint8 b;
			n = nf->m_ns->read(&b, 1, 1);
		}
		nf->m_position = i;
	}
	return nf->m_position == pos ? 0 : TU_FILE_SEEK_ERROR;
}

int netfile::http_seek_to_end(void *appdata)
{
	assert(appdata);
	netfile* nf = (netfile*) appdata;
	return http_seek(nf->m_size, appdata);
}

bool netfile::http_get_eof(void *appdata)
// Return true if we're positioned at the end of the buffer.
{
	assert(appdata);
	netfile* nf = (netfile*) appdata;
	return nf->m_position >= nf->m_size;
}


bool netfile::open_uri(const tu_string& host_name, const tu_string& resource)
{
	assert(m_ns);

//			m_if->write_string("CONNECT xxx.com HTTP/1.1\r\n		// if proxy is used 
	m_ns->write_string(tu_string("GET ") + resource + tu_string(" HTTP/1.1\r\n"), 1);
	m_ns->write_string(tu_string("Host:") + host_name + tu_string("\r\n"), 1);
	//		m_if->write_string("Accept:*/*\r\n", 1);
	//		m_if->write_string("Accept-Language: en\r\n", 1);
	//		m_if->write_string("Accept-Encoding: gzip, deflate, chunked\r\n", 1);
	//		m_if->write_string("Accept-Encoding: *\r\n", 1);
	m_ns->write_string("User-Agent:gameswf\r\n", 1);
	m_ns->write_string("Connection:Close\r\n", 1);
	m_ns->write_string("\r\n", 1);

	bool retcode = false;

	// read headers
	while (true)
	{
		tu_string s;
		int n = m_ns->read_line(&s, 100, 10);

		// end of header
		if (n == 0 || n == 2)
		{
			break;
		}

		printf("%s", s.c_str());

		// set retcode
		if (strncmp(s.c_str(), "HTTP/", 5) == 0)
		{
			if (strncmp(s.c_str() + 9, "200", 3) == 0)
			{
				retcode = true;
			}
		}
		else
		if (strncmp(s.c_str(), "Content-Length:", 15) == 0)
		{
			m_size = atoi(s.c_str() + 15);
		}
	}
	return retcode;
}

// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
