// postscript.h	-- by Thatcher Ulrich <tu@tulrich.com>

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some helpers for generating Postscript graphics.

#ifndef POSTSCRIPT_H
#define POSTSCRIPT_H


// Loosely translated into C++ from:
// -- ps.lua
// -- lua interface to postscript
// -- Luiz Henrique de Figueiredo (lhf@csg.uwaterloo.ca)
// -- 14 May 96
//
// From the Lua 4.0.1 distribution, see http://www.lua.org


class tu_file;


// @@ TODO all these functions need to take floats, not ints...

struct postscript
{
	postscript(tu_file* out, const char* title);
	~postscript();

	void	clear();	// New page
	void	comment(const char* s);
	void	rgbcolor(int r, int g, int b);
	void	black();
	void	gray(float amount);	// 0 == black, 1 == white

	void	line(int x0, int y0, int x1, int y1);
	void	moveto(int x0, int y0);
	void	lineto(int x0, int y0);
	void	linewidth(int w);
	// linestyle ?
	
	void	font(const char* name, float size);
	void	printf(float x, float y, const char* fmt, ...);	// printf-style output

	void	circle(float x, float y, float radius);
	void	disk(float x, float y, float radius);
	void	dot(float x, float y);

	void	rectangle(int x0, int x1, int y0, int y1);
	void	box(int x0, int x1, int y0, int y1);

private:
	void	update(float x0, float y0);	// enlarge the bounding box if necessary.

	tu_file*	m_out;
	int	m_page;
	int	m_x0, m_x1, m_y0, m_y1;	// bounding box
	bool	m_empty;
};


#endif // POSTSCRIPT_H



// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
