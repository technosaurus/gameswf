// gameswf_video_ogl.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// superfast NVIDIA OpenGL video implementation

#include "gameswf_types.h"
#include "gameswf_video_ogl.h"

// YUV_video_ogl implementation

static GLfloat yuv2rgb[2][4] = {{0.500000f, 0.413650f, 0.944700f, 0.f},	{0.851850f, 0.320550f, 0.500000f, 1.f}};
static GLint iquad[] = {-1, 1, 1, 1, 1, -1, -1, -1};

YUV_video_ogl_NV::YUV_video_ogl_NV()
{
	glEnable(GL_TEXTURE_2D);
	glGenTextures(NB_TEXS, texids);

	for (int i = 0; i < 3; ++i)
	{
		GLenum units[3] = {GL_TEXTURE0_ARB, GL_TEXTURE0_ARB, GL_TEXTURE1_ARB};
		planes[i].id = texids[i];
		planes[i].unit = i[units];
	}
};

YUV_video_ogl_NV::~YUV_video_ogl_NV()
{
	glDeleteTextures(NB_TEXS, texids);
}

void YUV_video_ogl_NV::YUV_tex_params()
{
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}

void YUV_video_ogl_NV::bind_tex()
{
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	for (int i = 0; i < 3; ++i)
	{
		ogl::active_texture(planes[i].unit);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, planes[i].id);

		YUV_tex_params();

		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE8,
			planes[i].p2w, planes[i].p2h, 0, GL_LUMINANCE,
			GL_UNSIGNED_BYTE, NULL);
	}

	planes[T] = planes[U];
	planes[T].unit = GL_TEXTURE1_ARB;

	glBindTexture(GL_TEXTURE_2D, planes[T].id);
	YUV_tex_params();
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, planes[T].p2w, planes[T].p2h,
		0, GL_RGB, GL_INT, NULL);

	ogl::combine_color(yuv2rgb[0], yuv2rgb[1]);
}

void YUV_video_ogl_NV::draw()
{
	glPushAttrib(GL_VIEWPORT_BIT);
	glPushMatrix();
	glLoadIdentity();

	glViewport(0, 0, planes[T].w, planes[T].h);

	glDrawBuffer(GL_AUX0);
	glReadBuffer(GL_AUX0);

	ogl::combine_UV();

	ogl::active_texture(planes[U].unit);
	glBindTexture(GL_TEXTURE_2D, planes[U].id);
	ogl::active_texture(planes[V].unit);
	glBindTexture(GL_TEXTURE_2D, planes[V].id);

	glBegin(GL_QUADS);
	{
		for (int i = 0; i < 4; ++i)
		{
			ogl::multi_tex_coord_2fv(planes[U].unit, planes[1].coords[i]);
			ogl::multi_tex_coord_2fv(planes[V].unit, planes[2].coords[i]);
			glVertex2iv(iquad + i * 2);
		}
	}
	glEnd();

	ogl::active_texture(planes[T].unit);
	glBindTexture(GL_TEXTURE_2D, planes[T].id);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, planes[T].w, planes[T].h);

	glPopMatrix();
	glPopAttrib();	// restore ViewPort

	gameswf::point a, b, c, d;
	m->transform(&a, gameswf::point(m_bounds->m_x_min, m_bounds->m_y_min));
	m->transform(&b, gameswf::point(m_bounds->m_x_max, m_bounds->m_y_min));
	m->transform(&c, gameswf::point(m_bounds->m_x_min, m_bounds->m_y_max));
	d.m_x = b.m_x + c.m_x - a.m_x;
	d.m_y = b.m_y + c.m_y - a.m_y;

	GLfloat fquad[8];
	fquad[0] = a.m_x; fquad[1] = a.m_y;
	fquad[2] = b.m_x; fquad[3] = b.m_y;
	fquad[4] = d.m_x; fquad[5] = d.m_y;
	fquad[6] = c.m_x; fquad[7] = c.m_y;

	glDrawBuffer(GL_BACK);

	ogl::active_texture(planes[Y].unit);
	glBindTexture(GL_TEXTURE_2D, planes[Y].id);

	ogl::combine_final();

	glBegin(GL_QUADS);
	{
		for (int i = 0; i < 4; ++i)
		{
			ogl::multi_tex_coord_2fv(planes[Y].unit, planes[Y].coords[i]);
			ogl::multi_tex_coord_2fv(planes[T].unit, planes[T].coords[i]);
			glVertex2fv(fquad + i * 2);
		}
	}
	glEnd();
}

void YUV_video_ogl_NV::upload_data()
{
	unsigned char*   ptr = m_data;
	for (int i = 0; i < 3; ++i)
	{
		GLint als[4] = {4, 1, 2, 1};
		glBindTexture(GL_TEXTURE_2D, planes[i].id);

		glPixelStorei(GL_UNPACK_ALIGNMENT, als[planes[i].offset & 3]);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, planes[i].w);

		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, planes[i].w, planes[i].h,
			GL_LUMINANCE, GL_UNSIGNED_BYTE, ptr);

		ptr += planes[i].size;
	}
}

void YUV_video_ogl_NV::display(const gameswf::matrix* mat, const gameswf::rect* bounds, const gameswf::rgba& color)
{
	glPushAttrib(GL_ENABLE_BIT);
	//		glPushAttrib(GL_ALL_ATTRIB_BITS);

	glColor4ub(color.m_r, color.m_g, color.m_b, color.m_a);

	m = mat;
	m_bounds = bounds;

	bind_tex();
	upload_data();
	draw();

	glPopAttrib();
}



YUV_video_ogl::YUV_video_ogl()
{
	for (int i = 0; i < 3; ++i)
	{
		planes[i].id = 0;
		planes[i].unit = 0;
	}
}

YUV_video_ogl::~YUV_video_ogl()
{
}

void YUV_video_ogl::display(const gameswf::matrix* mat, const gameswf::rect* bounds, const gameswf::rgba& color)
{
	glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);
	static GLfloat yuv_rgb[16] = {
		1, 1, 1, 0,
		0, -0.3946517043589703515f, 2.032110091743119266f, 0,
		1.139837398373983740f, -0.5805986066674976801f, 0, 0,
		0, 0, 0, 1
	};

	glMatrixMode(GL_COLOR);
	glPushMatrix();
	glLoadMatrixf(yuv_rgb);
	glPixelTransferf(GL_GREEN_BIAS, -0.5f);
	glPixelTransferf(GL_BLUE_BIAS, -0.5f);

	m = mat;
	m_bounds = bounds;

	gameswf::point a, b, c, d;
	m->transform(&a, gameswf::point(m_bounds->m_x_min, m_bounds->m_y_min));
	m->transform(&b, gameswf::point(m_bounds->m_x_max, m_bounds->m_y_min));
	m->transform(&c, gameswf::point(m_bounds->m_x_min, m_bounds->m_y_max));
	d.m_x = b.m_x + c.m_x - a.m_x;
	d.m_y = b.m_y + c.m_y - a.m_y;

	float w_bounds = TWIPS_TO_PIXELS(b.m_x - a.m_x);
	float h_bounds = TWIPS_TO_PIXELS(c.m_y - a.m_y);
	GLenum rgb[3] = {GL_RED, GL_GREEN, GL_BLUE};

	unsigned char*   ptr = m_data;
	float xpos = a.m_x < 0 ? 0.0f : a.m_x;	//hack
	float ypos = a.m_y < 0 ? 0.0f : a.m_y;	//hack
	glRasterPos2f(xpos, ypos);	//hack

	glDisable(GL_TEXTURE_2D);

	for (int i = 0; i < 3; ++i)
	{
		float zx = w_bounds / (float) planes[i].w;
		float zy = h_bounds / (float) planes[i].h;
		glPixelZoom(zx, - zy);	// flip & zoom image

		if (i > 0)
		{
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);
		}
		glDrawPixels(planes[i].w, planes[i].h, rgb[i], GL_UNSIGNED_BYTE, ptr);
		ptr += planes[i].size;
	}

	glMatrixMode(GL_COLOR);
	glPopMatrix();

	glPopAttrib();
}


