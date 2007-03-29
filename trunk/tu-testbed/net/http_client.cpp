// http_client.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// HTTP implementation of tu_file

#include "base/utility.h"
#include "base/tu_file.h"
#include "net/http_client.h"

#define HTTP_PORT 80

netfile::netfile(const char* url) :
	m_position(0),
	m_iface(NULL)
{

	// Open a socket to receive connections on.
	m_iface = tu_create_net_interface_tcp(url, HTTP_PORT);
	if (m_iface == NULL)
	{
		fprintf(stderr, "Couldn't open net interface\n");
		exit(1);
	}

}

netfile::~netfile()
{
	delete m_iface;
}

int netfile::http_read(void* dst, int bytes, void* appdata) 
// Return the number of bytes actually read.  EOF or an error would
// cause that to not be equal to "bytes".
{
	assert(appdata);
	assert(dst);
	netfile* nf = (netfile*) appdata;

	net_socket* soc = nf->m_iface->create_socket();
	tu_string x="xxxxxxxxxxxxxxxxxxxxxx";
	int n=soc->write_string(x, 1);

	//	return fread( dst, 1, bytes, (FILE *)appdata );

	return 0;
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
	assert(appdata);
	assert(src);
	return fwrite( src, 1, bytes, (FILE *)appdata );
}

int netfile::http_seek(int pos, void *appdata)
// Return 0 on success, or TU_FILE_SEEK_ERROR on failure.
{
	assert(appdata);
	clearerr((FILE*) appdata);	// make sure EOF flag is cleared.
	int	result = fseek((FILE*)appdata, pos, SEEK_SET);
	if (result == EOF)
	{
		// @@ TODO should set m_error to something relevant based on errno.
		return TU_FILE_SEEK_ERROR;
	}
	return 0;
}

int netfile::http_seek_to_end(void *appdata)
{
	return 0;
}

bool netfile::http_get_eof(void *appdata)
{
	return true;
}

// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
