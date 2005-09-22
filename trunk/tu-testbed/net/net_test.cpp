// net_test.cpp -- Thatcher Ulrich http://tulrich.com 2005

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Small test program to exercise tu-testbed/net/ library.


#include <stdio.h>
#include "net/http_server.h"
#include "net/net_interface.h"
#include "base/tu_timer.h"


struct my_handler : public http_handler
{
	virtual void handle_request(http_server* server, const tu_string& key, http_request* req)
	{
		tu_string out;
		out += "<html><head><title>tu-testbed net_test</title></head><body>";
		req->dump_html(&out);

		// Show the current timer.
		char buf[200];
		sprintf(buf, "Current timer: %f<br>\n", tu_timer::ticks_to_seconds(tu_timer::get_ticks()));
		out += buf;

		out += "</body></html>\n";

		server->send_html_response(req, out);
	}
};


static bool s_quit = false;


struct quit_handler : public http_handler
{
	virtual void handle_request(http_server* server, const tu_string& key, http_request* req)
	{
		s_quit = true;
	}
};


int main(int argc, const char** argv)
{
	int port = 0x7EEC; // 32492.  "7EEC" looks vaguely like "TWEAK"

	if (argc > 1) {
		port = atoi(argv[1]);
	}
	
	net_interface* iface = tu_create_net_interface_tcp(port);
	if (iface == NULL)
	{
		fprintf(stderr, "Couldn't open net interface\n");
		exit(1);
	}

	http_server* server = new http_server(iface);

	my_handler handler;
	server->add_handler("/status", &handler);

	quit_handler quit_h;
	server->add_handler("/quit", &quit_h);
	
	printf("Point a browser at http://localhost:%d to test http_server\n", port);
	fflush(stdout);

	while (s_quit == false)
	{
		server->update();
	}
}


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:

