// gameswf_video_ogl.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#ifndef GAMESWF_VIDEO_OGL_H
#define GAMESWF_VIDEO_OGL_H

#include "gameswf_video_base.h"
#include "base/ogl.h"

struct video_ogl : public gameswf::video_handler
{
	GLuint m_texture;

	video_ogl();
	~video_ogl();

	void display(const gameswf::matrix* mat, const gameswf::rect* bounds, const gameswf::rgba& color);

};

#endif
