// postscript.cpp	-- by Thatcher Ulrich <tu@tulrich.com>

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some helpers for generating Postscript graphics.


#include "engine/postscript.h"

#include "engine/tu_file.h"
#include <stdarg.h>


// Loosely translated into C++ from:
// -- ps.lua
// -- lua interface to postscript
// -- Luiz Henrique de Figueiredo (lhf@csg.uwaterloo.ca)
// -- 14 May 96
//
// From the Lua 4.0.1 distribution, see http://www.lua.org



postscript::postscript(tu_file* out, const char* title)
	:
	m_out(out),
	m_page(0),
	m_x0(1000),
	m_y0(1000),
	m_x1(0),
	m_y1(0),
	m_empty(true)
// Initialize the file & this struct, etc.
{
	assert(m_out != NULL);

	if (title == NULL)
	{
		title = "no title";
	}
	
	m_out->printf(
		"%%!PS-Adobe-2.0 EPSF-1.2\n"
		"%%%%Title: %s\n", title);
	m_out->printf(
		"%%%%Creator: postscript.cpp from tu-testbed\n"
		"%%%%CreationDate: 1 1 2001\n"
		"%%%%Pages: (atend)\n"
		"%%%%BoundingBox: (atend)\n"
		"%%%%EndComments\n"
		"%%%%BeginProcSet: postscript.cpp\n"
		"/s { stroke } bind def\n"
		"/f { fill } bind def\n"
		"/m { moveto } bind def\n"
		"/l { lineto } bind def\n"
		"/L { moveto lineto stroke } bind def\n"
		"/t { show } bind def\n"
		"/o { 0 360 arc stroke } bind def\n"
		"/O { 0 360 arc fill } bind def\n"
		"/p { 3 0 360 arc fil } bind def\n"
		"/F { findfont exch scalefont setfont } bind def\n"
		"/LS { 0 setdash } bind def\n"
		"/LW { setlinewidth } bind def\n"
		"%%%%EndProcSet: postscript.cpp\n"
		"%%%%EndProlog\n"
		"%%%%BeginSetup\n"
		"0 setlinewidth\n"
		"1 setlinejoin\n"
		"1 setlinecap\n"
		"10 /Times-Roman F\n"
		"%%%%EndSetup\n\n"
		"%%%%Page: 1 1\n");
}


postscript::~postscript()
// End the Postscript info.
{
	m_out->printf(
		"stroke\n"
		"showpage\n"
		"%%%%Trailer\n"
		"%%%%Pages: %d %d\n"
		"%%%%BoundingBox: %d %d %d %d\n"
		"%%%%EOF\n",
		m_page + 1, m_page + 1,
		m_x0, m_y0, m_x1, m_y1
		);
	
}


void	postscript::clear()
// New page.
{
	if (m_empty) return;

	m_page++;
	m_out->printf("showpage\n%%%%Page: %d %d\n", m_page + 1, m_page + 1);

	m_empty = true;
}


void	postscript::comment(const char* s)
// Insert a comment into the output.
{
	m_out->printf("%% %s\n", s);
}


void	postscript::rgbcolor(int r, int g, int b)
// Set the pen color.
// @@ need to look up the units for this!
{
	m_out->printf("%d %d %d setrgbcolor\n", r, g, b);
}


void	postscript::gray(float amount)
// 0 == black, 1 == white
{
	m_out->printf("%d setgray\n", int(amount * 255.0f));	// @@ need to look up the units for this!
}


void	postscript::line(int x0, int y0, int x1, int y1)
{
	m_out->printf(
		"%d %d %d %d L\n",
		x1, y1, x0, y0);

	update(x0, y0);
	update(x1, y1);
}


void	postscript::moveto(int x0, int y0)
{
	m_out->printf(
		"%d %d m\n",
		x0, y0);

	update(x0, y0);
}


void	postscript::lineto(int x0, int y0)
{
	m_out->printf(
		"%d %d l\n",
		x0, y0);

	update(x0, y0);
}


void	postscript::linewidth(int w)
{
	m_out->printf("%d LW\n", w);
}


// @@ linestyle ?
	

void	postscript::font(const char* name, float size)
{
	m_out->printf("%f /%s F\n", size, name);
}


#ifdef WIN32
#define vsnprintf	_vsnprintf
#endif // WIN32


void	postscript::printf(float x, float y, const char* fmt, ...)
{
	static const int	BUFFER_SIZE = 1000;

	char	buffer[BUFFER_SIZE];

	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buffer, BUFFER_SIZE, fmt, ap);
	va_end(ap);

	m_out->printf("%f %f m (%s) t\n", x, y, buffer);
}


void	postscript::circle(float x, float y, float radius)
{
	m_out->printf("%f %f %f o\n", x, y, radius);
	update(x - radius, y - radius);
	update(x + radius, y + radius);
}


void	postscript::disk(float x, float y, float radius)
{
	m_out->printf("%f %f %f O\n", x, y, radius);
	update(x - radius, y - radius);
	update(x + radius, y + radius);
}


void	postscript::dot(float x, float y)
{
	m_out->printf("%f %f p\n", x, y);
	update(x, y);
}


void	postscript::rectangle(int x0, int x1, int y0, int y1)
{
	m_out->printf(
		"%d %d m "
		"%d %d l "
		"%d %d l "
		"%d %d l s\n",
		x0, y0,
		x1, y0,
		x1, y1,
		x0, y1);
	update(x0, y0);
	update(x1, y1);
}


void	postscript::box(int x0, int x1, int y0, int y1)
{
	m_out->printf(
		"%d %d m "
		"%d %d l "
		"%d %d l "
		"%d %d l f\n",
		x0, y0,
		x1, y0,
		x1, y1,
		x0, y1);
	update(x0, y0);
	update(x1, y1);
}


void	postscript::update(float x, float y)
// enlarge the bounding box if necessary.
{
	if (x < m_x0) m_x0 = int(floorf(x));
	if (x > m_x1) m_x1 = int(ceilf(x));
	if (y < m_y0) m_y0 = int(floorf(y));
	if (y > m_y1) m_y1 = int(ceilf(y));

	m_empty = false;
}


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
