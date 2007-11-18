// gameswf_video_ogl.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// OpenGL video implementation

#include "gameswf/gameswf_types.h"
#include "gameswf/gameswf_video_ogl.h"

video_ogl::video_ogl()
{
	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, &m_texture);

	glBindTexture(GL_TEXTURE_2D, m_texture);

	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);	// GL_NEAREST ?
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
};

video_ogl::~video_ogl()
{
	glDeleteTextures(1, &m_texture);
}

void video_ogl::display(const gameswf::matrix* mat, const gameswf::rect* bounds, const gameswf::rgba& color)
{
	glBindTexture(GL_TEXTURE_2D, m_texture);
	glEnable(GL_TEXTURE_2D);

	int w = m_data->m_width;
	int h = m_data->m_height;
	int	w2p = 1; while (w2p < w) { w2p <<= 1; }
	int	h2p = 1; while (h2p < h) { h2p <<= 1; }

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w2p, h2p, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, m_data->m_data);

	m = mat;
	m_bounds = bounds;
	gameswf::point a, b, c, d;
	m->transform(&a, gameswf::point(m_bounds->m_x_min, m_bounds->m_y_min));
	m->transform(&b, gameswf::point(m_bounds->m_x_max, m_bounds->m_y_min));
	m->transform(&c, gameswf::point(m_bounds->m_x_min, m_bounds->m_y_max));
	d.m_x = b.m_x + c.m_x - a.m_x;
	d.m_y = b.m_y + c.m_y - a.m_y;

	float s_coord = (float) w / w2p;
	float t_coord = (float) h / h2p;
	glColor4ub(color.m_r, color.m_g, color.m_b, color.m_a);
	glBegin(GL_TRIANGLE_STRIP);
	{
		glTexCoord2f(0, 0);
		glVertex2f(a.m_x, a.m_y);
		glTexCoord2f(s_coord, 0);
		glVertex2f(b.m_x, b.m_y);
		glTexCoord2f(0, t_coord);
		glVertex2f(c.m_x, c.m_y);
		glTexCoord2f(s_coord, t_coord);
		glVertex2f(d.m_x, d.m_y);
	}
	glEnd();
}


