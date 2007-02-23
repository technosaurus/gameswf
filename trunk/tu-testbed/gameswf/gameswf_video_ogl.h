// gameswf_video_ogl.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// superfast NVIDIA video implementation

#ifndef GAMESWF_VIDEO_OGL_H
#define GAMESWF_VIDEO_OGL_H

#include "gameswf_video_base.h"
#include "base/ogl.h"

struct YUV_video_ogl_NV : public gameswf::YUV_video
{

	enum {Y, U, V, T, NB_TEXS};

	YUV_video_ogl_NV(int width, int height);
	~YUV_video_ogl_NV();

private:
	GLuint texids[NB_TEXS];
	void YUV_tex_params();
	void bind_tex();
	void draw();
	void upload_data();
	void display(const gameswf::matrix* mat, const gameswf::rect* bounds, const gameswf::rgba& color);
};

struct YUV_video_ogl : public gameswf::YUV_video
{
	YUV_video_ogl(int width, int height);
	~YUV_video_ogl();

	void display(const gameswf::matrix* mat, const gameswf::rect* bounds, const gameswf::rgba& color);

};

#endif