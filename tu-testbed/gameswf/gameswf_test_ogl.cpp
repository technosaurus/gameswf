// gameswf_test_ogl.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// A minimal test player app for the gameswf library.


#include "SDL.h"
#include "SDL_mixer.h"
#include "gameswf.h"
#include "gameswf_log.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "base/ogl.h"
#include "base/utility.h"
#include "base/container.h"
#include "base/tu_file.h"
#include "base/tu_types.h"

void	print_usage()
// Brief instructions.
{
	printf(
		"gameswf_test_ogl -- a test player for the gameswf library.\n"
		"\n"
		"This program has been donated to the Public Domain.\n"
		"See http://tulrich.com/geekstuff/gameswf.html for more info.\n"
		"\n"
		"usage: gameswf_test_ogl [options] movie_file.swf\n"
		"\n"
		"Plays a SWF (Shockwave Flash) movie, using OpenGL and the\n"
		"gameswf library.\n"
		"\n"
		"options:\n"
		"\n"
		"  -h          Print this info.\n"
		"  -s <factor> Scale the movie up/down by the specified factor\n"
		"  -a          Turn antialiasing on/off.  (obsolete)\n"
		"  -c          Build a cache file (for prerendering fonts)\n"
		"  -v          Be verbose; i.e. print log messages to stdout\n"
		"  -va         Be verbose about movie Actions\n"
		"\n"
		"keys:\n"
		"  q or ESC    Quit/Exit\n"
		"  p           Toggle Pause\n"
		"  r           Restart the movie\n"
		"  [ or -      Step back one frame\n"
		"  ] or +      Step forward one frame\n"
		"  a           Toggle antialiasing (doesn't work)\n"
		"  t           Debug.  Put some text in a dynamic text field\n"
		"  b           Toggle background color\n"
		);
}


#define OVERSIZE	1.0f


static float	s_scale = 1.0f;
static bool	s_antialiased = false;
static bool	s_cache = false;
static bool	s_verbose = false;
static bool	s_background = true;


static void	message_log(const char* message)
// Process a log message.
{
	if (s_verbose)
	{
		fputs(message, stdout);
                //flush(stdout); // needed on osx for some reason
	}
}


static void	log_callback(bool error, const char* message)
// Error callback for handling gameswf messages.
{
	if (error)
	{
		// Log, and also print to stderr.
		message_log(message);
		fputs(message, stderr);
	}
	else
	{
		message_log(message);
	}
}


static tu_file*	file_opener(const char* url)
// Callback function.  This opens files for the gameswf library.
{
	return new tu_file(url, "rb");
}


// Use SDL_mixer to handle gameswf sounds.
struct SDL_sound_handler : gameswf::sound_handler
{
	bool	m_opened;
	array<Mix_Chunk*>	m_samples;

	#define	SAMPLE_RATE 22050
	#define MIX_CHANNELS 8
	#define STEREO_MONO_CHANNELS 1

	SDL_sound_handler()
		:
		m_opened(false)
	{
		if (Mix_OpenAudio(SAMPLE_RATE, AUDIO_S16SYS, STEREO_MONO_CHANNELS, 1024) != 0)
		{
			log_callback(true, "can't open SDL_mixer!");
			log_callback(true, Mix_GetError());
		}
		else
		{
			m_opened = true;

			Mix_AllocateChannels(MIX_CHANNELS);
		}
	}

	~SDL_sound_handler()
	{
		if (m_opened)
		{
			Mix_CloseAudio();
			for (int i = 0, n = m_samples.size(); i < n; i++)
			{
				if (m_samples[i])
				{
					Mix_FreeChunk(m_samples[i]);
				}
			}
		}
		else
		{
			assert(m_samples.size() == 0);
		}
	}


	virtual int	create_sound(
		void* data,
		int data_bytes,
		int sample_count,
		format_type format,
		int sample_rate,
		bool stereo)
	// Called by gameswf to create a sample.  We'll return a sample ID that gameswf
	// can use for playing it.
	{
		if (m_opened == false)
		{
			return 0;
		}

		Sint16*	adjusted_data = 0;
		int	adjusted_size = 0;
		Mix_Chunk*	sample = 0;

		switch (format)
		{
		case FORMAT_RAW:
			convert_raw_data(&adjusted_data, &adjusted_size, data, sample_count, 1, sample_rate, stereo);
			break;

		case FORMAT_NATIVE16:
			convert_raw_data(&adjusted_data, &adjusted_size, data, sample_count, 2, sample_rate, stereo);
			break;

		case FORMAT_MP3:
			message_log("mp3 format sound requested; this demo does not handle mp3\n");
			break;

		default:
			// Unhandled format.
			message_log("unknown format sound requested; this demo does not handle it\n");
			break;
		}

		if (adjusted_data)
		{
			sample = Mix_QuickLoad_RAW((unsigned char*) adjusted_data, adjusted_size);
			Mix_VolumeChunk(sample, 128);	// full volume by default
		}

		m_samples.push_back(sample);
		return m_samples.size() - 1;
	}


	virtual void	play_sound(int sound_handle, int loop_count /* other params */)
	// Play the index'd sample.
	{
		if (sound_handle >= 0 && sound_handle < m_samples.size())
		{
			if (m_samples[sound_handle])
			{
				// Play this sample on the first available channel.
				Mix_PlayChannel(-1, m_samples[sound_handle], loop_count);
			}
		}
	}

	
	virtual void	stop_sound(int sound_handle)
	{
		if (sound_handle < 0 || sound_handle >= m_samples.size())
		{
			// Invalid handle.
			return;
		}

		for (int i = 0; i < MIX_CHANNELS; i++)
		{
			Mix_Chunk*	playing_chunk = Mix_GetChunk(i);
			if (Mix_Playing(i)
			    && playing_chunk == m_samples[sound_handle])
			{
				// Stop this channel.
				Mix_HaltChannel(i);
			}
		}
	}


	virtual void	delete_sound(int sound_handle)
	// gameswf calls this when it's done with a sample.
	{
		if (sound_handle >= 0 && sound_handle < m_samples.size())
		{
			Mix_Chunk*	chunk = m_samples[sound_handle];
			if (chunk)
			{
				delete [] (chunk->abuf);
				Mix_FreeChunk(chunk);
				m_samples[sound_handle] = 0;
			}
		}
	}


	static void convert_raw_data(
		Sint16** adjusted_data,
		int* adjusted_size,
		void* data,
		int sample_count,
		int sample_size,
		int sample_rate,
		bool stereo)
	// VERY crude sample-rate & sample-size conversion.  Converts
	// input data to the SDL_mixer output format (SAMPLE_RATE,
	// stereo, 16-bit native endianness)
	{
// 		// xxxxx debug pass-thru
// 		{
// 			int	output_sample_count = sample_count * (stereo ? 2 : 1);
// 			Sint16*	out_data = new Sint16[output_sample_count];
// 			*adjusted_data = out_data;
// 			*adjusted_size = output_sample_count * 2;	// 2 bytes per sample
// 			memcpy(out_data, data, *adjusted_size);
// 			return;
// 		}
// 		// xxxxx

//		if (stereo == false) { sample_rate >>= 1; }	// simple hack to handle dup'ing mono to stereo
		if (stereo == true) { sample_rate <<= 1; }	// simple hack to lose half the samples to get mono from stereo

		// Brain-dead sample-rate conversion: duplicate or
		// skip input samples an integral number of times.
		int	inc = 1;	// increment
		int	dup = 1;	// duplicate
		if (sample_rate > SAMPLE_RATE)
		{
			inc = sample_rate / SAMPLE_RATE;
		}
		else if (sample_rate < SAMPLE_RATE)
		{
			dup = SAMPLE_RATE / sample_rate;
		}

		int	output_sample_count = (sample_count * dup) / inc;
		Sint16*	out_data = new Sint16[output_sample_count];
		*adjusted_data = out_data;
		*adjusted_size = output_sample_count * 2;	// 2 bytes per sample

		if (sample_size == 1)
		{
			// Expand from 8 bit to 16 bit.
			Uint8*	in = (Uint8*) data;
			for (int i = 0; i < output_sample_count; i++)
			{
				Uint8	val = *in;
				for (int j = 0; j < dup; j++)
				{
					*out_data++ = (int(val) - 128);
				}
				in += inc;
			}
		}
		else
		{
			// 16-bit to 16-bit conversion.
			Sint16*	in = (Sint16*) data;
			for (int i = 0; i < output_sample_count; i += dup)
			{
				Sint16	val = *in;
				for (int j = 0; j < dup; j++)
				{
					*out_data++ = val;
				}
				in += inc;
			}
		}
	}

};

namespace gameswf
{

	// bitmap_info_ogl declaration
	struct bitmap_info_ogl : public bitmap_info
	{
		bitmap_info_ogl(create_empty e);
		bitmap_info_ogl(image::rgb* im);
		bitmap_info_ogl(image::rgba* im);
		virtual void set_alpha_image(int width, int height, Uint8* data);
	};


	struct render_handler_ogl : public render_handler
	{
		// Some renderer state.

		// Enable/disable antialiasing.
		bool	m_enable_antialias;

		// Output size.
		float	m_display_width;
		float	m_display_height;

		matrix	m_current_matrix;
		cxform	m_current_cxform;
                
                void set_antialiased(bool enable)
                {
                    m_enable_antialias = enable;
                }

		static void make_next_miplevel(int* width, int* height, Uint8* data)
		// Utility.  Mutates *width, *height and *data to create the
		// next mip level.
		{
			assert(width);
			assert(height);
			assert(data);

			int	new_w = *width >> 1;
			int	new_h = *height >> 1;
			if (new_w < 1) new_w = 1;
			if (new_h < 1) new_h = 1;
			
			if (new_w * 2 != *width  || new_h * 2 != *height)
			{
				// Image can't be shrunk along (at least) one
				// of its dimensions, so don't bother
				// resampling.  Technically we should, but
				// it's pretty useless at this point.  Just
				// change the image dimensions and leave the
				// existing pixels.
			}
			else
			{
				// Resample.  Simple average 2x2 --> 1, in-place.
				for (int j = 0; j < new_h; j++) {
					Uint8*	out = ((Uint8*) data) + j * new_w;
					Uint8*	in = ((Uint8*) data) + (j << 1) * *width;
					for (int i = 0; i < new_w; i++) {
						int	a;
						a = (*(in + 0) + *(in + 1) + *(in + 0 + *width) + *(in + 1 + *width));
						*(out) = a >> 2;
						out++;
						in += 2;
					}
				}
			}

			// Munge parameters to reflect the shrunken image.
			*width = new_w;
			*height = new_h;
		}

		struct fill_style
		{
			enum mode
			{
				INVALID,
				COLOR,
				BITMAP_WRAP,
				BITMAP_CLAMP,
				LINEAR_GRADIENT,
				RADIAL_GRADIENT,
			};
			mode	m_mode;
			rgba	m_color;
			const bitmap_info*	m_bitmap_info;
			matrix	m_bitmap_matrix;
			cxform	m_bitmap_color_transform;

			fill_style()
				:
				m_mode(INVALID)
			{
			}

			void	apply(/*const matrix& current_matrix*/) const
			// Push our style into OpenGL.
			{
				assert(m_mode != INVALID);

				if (m_mode == COLOR)
				{
					m_color.apply();
					glDisable(GL_TEXTURE_2D);
				}
				else if (m_mode == BITMAP_WRAP
					 || m_mode == BITMAP_CLAMP)
				{
					assert(m_bitmap_info != NULL);

					m_color.apply();

					if (m_bitmap_info == NULL)
					{
						glDisable(GL_TEXTURE_2D);
					}
					else
					{
						// Set up the texture for rendering.

						{
							// For the moment we can only handle the modulate part of the
							// color transform...
							// How would we handle any additive part?  Realistically we
							// need to use a pixel shader.  Although there is a GL_COLOR_SUM
							// extension that can set an offset for R,G,B (but apparently not A).
							glColor4f(m_bitmap_color_transform.m_[0][0],
								  m_bitmap_color_transform.m_[1][0],
								  m_bitmap_color_transform.m_[2][0],
								  m_bitmap_color_transform.m_[3][0]
								  );
						}

						glBindTexture(GL_TEXTURE_2D, m_bitmap_info->m_texture_id);
						glEnable(GL_TEXTURE_2D);
						glEnable(GL_TEXTURE_GEN_S);
						glEnable(GL_TEXTURE_GEN_T);
					
						if (m_mode == BITMAP_CLAMP)
						{
							glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
							glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
						}
						else
						{
							assert(m_mode == BITMAP_WRAP);
							glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
							glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
						}

						// Set up the bitmap matrix for texgen.

						float	inv_width = 1.0f / m_bitmap_info->m_original_width;
						float	inv_height = 1.0f / m_bitmap_info->m_original_height;

						const matrix&	m = m_bitmap_matrix;
						glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
						float	p[4] = { 0, 0, 0, 0 };
						p[0] = m.m_[0][0] * inv_width;
						p[1] = m.m_[0][1] * inv_width;
						p[3] = m.m_[0][2] * inv_width;
						glTexGenfv(GL_S, GL_OBJECT_PLANE, p);

						glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
						p[0] = m.m_[1][0] * inv_height;
						p[1] = m.m_[1][1] * inv_height;
						p[3] = m.m_[1][2] * inv_height;
						glTexGenfv(GL_T, GL_OBJECT_PLANE, p);
					}
				}
			}

			void	disable() { m_mode = INVALID; }
			void	set_color(rgba color) { m_mode = COLOR; m_color = color; }
			void	set_bitmap(const bitmap_info* bi, const matrix& m, bitmap_wrap_mode wm, const cxform& color_transform)
			{
				m_mode = (wm == WRAP_REPEAT) ? BITMAP_WRAP : BITMAP_CLAMP;
				m_color = rgba();
				m_bitmap_info = bi;
				m_bitmap_matrix = m;
				m_bitmap_color_transform = color_transform;
			}
			bool	is_valid() const { return m_mode != INVALID; }
		};


		// Style state.
		enum style_index
		{
			LEFT_STYLE = 0,
			RIGHT_STYLE,
			LINE_STYLE,

			STYLE_COUNT
		};
		fill_style	m_current_styles[STYLE_COUNT];


		bitmap_info*	create_bitmap_info(image::rgb* im)
		// Given an image, returns a pointer to a bitmap_info struct
		// that can later be passed to fill_styleX_bitmap(), to set a
		// bitmap fill style.
		{
			return new bitmap_info_ogl(im);
		}


		bitmap_info*	create_bitmap_info(image::rgba* im)
		// Given an image, returns a pointer to a bitmap_info struct
		// that can later be passed to fill_style_bitmap(), to set a
		// bitmap fill style.
		//
		// This version takes an image with an alpha channel.
		{
			return new bitmap_info_ogl(im);
		}


		bitmap_info*	create_bitmap_info_blank()
		// Creates and returns an empty bitmap_info structure.  Image data
		// can be bound to this info later, via set_alpha_image().
		{
			return new bitmap_info_ogl(bitmap_info::empty);
		}


		void	set_alpha_image(bitmap_info* bi, int w, int h, Uint8* data)
		// Set the specified bitmap_info so that it contains an alpha
		// texture with the given data (1 byte per texel).
		//
		// Munges *data (in order to make mipmaps)!!
		{
			assert(bi);

			bi->set_alpha_image(w, h, data);
		}


		void	delete_bitmap_info(bitmap_info* bi)
		// Delete the given bitmap info struct.
		{
			delete bi;
		}


		void	begin_display(
			rgba background_color,
			int viewport_x0, int viewport_y0,
			int viewport_width, int viewport_height,
			float x0, float x1, float y0, float y1)
		// Set up to render a full frame from a movie and fills the
		// background.  Sets up necessary transforms, to scale the
		// movie to fit within the given dimensions.  Call
		// end_display() when you're done.
		//
		// The rectangle (viewport_x0, viewport_y0, viewport_x0 +
		// viewport_width, viewport_y0 + viewport_height) defines the
		// window coordinates taken up by the movie.
		//
		// The rectangle (x0, y0, x1, y1) defines the pixel
		// coordinates of the movie that correspond to the viewport
		// bounds.
		{
			m_display_width = fabsf(x1 - x0);
			m_display_height = fabsf(y1 - y0);

			glViewport(viewport_x0, viewport_y0, viewport_width, viewport_height);

			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glOrtho(x0, x1, y0, y1, -1, 1);

			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);	// GL_MODULATE

			glDisable(GL_TEXTURE_2D);

			// Clear the background, if background color has alpha > 0.
			if (background_color.m_a > 0)
			{
				// Draw a big quad.
				background_color.apply();
				glBegin(GL_QUADS);
				glVertex2f(x0, y0);
				glVertex2f(x1, y0);
				glVertex2f(x1, y1);
				glVertex2f(x0, y1);
				glEnd();
			}

			// Old unused code.  Might get revived someday.
			#if 0
					// See if we want to, and can, use multitexture
					// antialiasing.
					s_multitexture_antialias = false;
					if (m_enable_antialias)
					{
						int	tex_units = 0;
						glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &tex_units);
						if (tex_units >= 2)
						{
							s_multitexture_antialias = true;
						}

						// Make sure we have an edge texture available.
						if (s_multitexture_antialias == true
							&& s_edge_texture_id == 0)
						{
							// Very simple texture: 2 texels wide, 1 texel high.
							// Both texels are white; left texel is all clear, right texel is all opaque.
							unsigned char	edge_data[8] = { 255, 255, 255, 0, 255, 255, 255, 255 };

							ogl::active_texture(GL_TEXTURE1_ARB);
							glEnable(GL_TEXTURE_2D);
							glGenTextures(1, &s_edge_texture_id);
							glBindTexture(GL_TEXTURE_2D, s_edge_texture_id);

							glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
							glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
							glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
							glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

							glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, edge_data);

							glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);	// @@ should we use a 1D texture???

							glDisable(GL_TEXTURE_2D);
							ogl::active_texture(GL_TEXTURE0_ARB);
							glDisable(GL_TEXTURE_2D);
						}
					}
			#endif // 0
		}


		void	end_display()
		// Clean up after rendering a frame.  Client program is still
		// responsible for calling glSwapBuffers() or whatever.
		{
			glMatrixMode(GL_MODELVIEW);
			glPopMatrix();
		}


		void	set_matrix(const matrix& m)
		// Set the current transform for mesh & line-strip rendering.
		{
			m_current_matrix = m;
		}


		void	set_cxform(const cxform& cx)
		// Set the current color transform for mesh & line-strip rendering.
		{
			m_current_cxform = cx;
		}
                
                void 	apply_matrix(const matrix& m)
                // multiply current matrix with opengl matrix
                {
                        float	mat[16];
                        memset(&mat[0], 0, sizeof(mat));
                        mat[0] = m.m_[0][0];
                        mat[1] = m.m_[1][0];
                        mat[4] = m.m_[0][1];
                        mat[5] = m.m_[1][1];
                        mat[10] = 1;
                        mat[12] = m.m_[0][2];
                        mat[13] = m.m_[1][2];
                        mat[15] = 1;
                        glMultMatrixf(mat);
                }

                void 	apply_color(const rgba& c)
                // multiply current matrix with opengl matrix
                {
                        glColor4ub(c.m_r, c.m_g, c.m_b, c.m_a);
                }

		void	fill_style_disable(int fill_side)
		// Don't fill on the {0 == left, 1 == right} side of a path.
		{
			assert(fill_side >= 0 && fill_side < 2);

			m_current_styles[fill_side].disable();
		}


		void	line_style_disable()
		// Don't draw a line on this path.
		{
			m_current_styles[LINE_STYLE].disable();
		}


		void	fill_style_color(int fill_side, rgba color)
		// Set fill style for the left interior of the shape.  If
		// enable is false, turn off fill for the left interior.
		{
			assert(fill_side >= 0 && fill_side < 2);

			m_current_styles[fill_side].set_color(m_current_cxform.transform(color));
		}


		void	line_style_color(rgba color)
		// Set the line style of the shape.  If enable is false, turn
		// off lines for following curve segments.
		{
			m_current_styles[LINE_STYLE].set_color(m_current_cxform.transform(color));
		}


		void	fill_style_bitmap(int fill_side, const bitmap_info* bi, const matrix& m, bitmap_wrap_mode wm)
		{
			assert(fill_side >= 0 && fill_side < 2);
			m_current_styles[fill_side].set_bitmap(bi, m, wm, m_current_cxform);
		}
                
                void	line_style_width(float width)
		{
			// WK: what to do here???
		}


		void	draw_mesh(const float coords[], int vertex_count)
		{
			// Set up current style.
			m_current_styles[LEFT_STYLE].apply();

			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			m_current_matrix.apply();

			// Send the tris to OpenGL
			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(2, GL_FLOAT, sizeof(float) * 2, coords);
			glDrawArrays(GL_TRIANGLES, 0, vertex_count);
			glDisableClientState(GL_VERTEX_ARRAY);

			glPopMatrix();
		}


		void	draw_line_strip(const float coords[], int vertex_count)
		// Draw the line strip formed by the sequence of points.
		{
			// Set up current style.
			m_current_styles[LINE_STYLE].apply();

			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			m_current_matrix.apply();

			// Send the line-strip to OpenGL
			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(2, GL_FLOAT, sizeof(float) * 2, coords);
			glDrawArrays(GL_LINE_STRIP, 0, vertex_count);
			glDisableClientState(GL_VERTEX_ARRAY);

			glPopMatrix();
		}


		void	draw_bitmap(const matrix& m, const bitmap_info* bi, const rect& coords, const rect& uv_coords, rgba color)
		// Draw a rectangle textured with the given bitmap, with the
		// given color.  Apply given transform; ignore any currently
		// set transforms.
		//
		// Intended for textured glyph rendering.
		{
			assert(bi);

		//	color = s_cxform_stack.back().transform(color);
			color.apply();

			point a, b, c, d;
			m.transform(&a, point(coords.m_x_min, coords.m_y_min));
			m.transform(&b, point(coords.m_x_max, coords.m_y_min));
			m.transform(&c, point(coords.m_x_min, coords.m_y_max));
			d.m_x = b.m_x + c.m_x - a.m_x;
			d.m_y = b.m_y + c.m_y - a.m_y;

			glBindTexture(GL_TEXTURE_2D, bi->m_texture_id);
			glEnable(GL_TEXTURE_2D);
			glDisable(GL_TEXTURE_GEN_S);
			glDisable(GL_TEXTURE_GEN_T);

			glBegin(GL_TRIANGLE_STRIP);

			glTexCoord2f(uv_coords.m_x_min, uv_coords.m_y_min);
			glVertex2f(a.m_x, a.m_y);

			glTexCoord2f(uv_coords.m_x_max, uv_coords.m_y_min);
			glVertex2f(b.m_x, b.m_y);

			glTexCoord2f(uv_coords.m_x_min, uv_coords.m_y_max);
			glVertex2f(c.m_x, c.m_y);

			glTexCoord2f(uv_coords.m_x_max, uv_coords.m_y_max);
			glVertex2f(d.m_x, d.m_y);

			glEnd();
		}
                
                void begin_submit_mask()
                {
                    glEnable(GL_STENCIL_TEST); 
                    glClearStencil(0);
                    glClear(GL_STENCIL_BUFFER_BIT);
                    glColorMask(0,0,0,0); 	// disable framebuffer writes
                    glEnable(GL_STENCIL_TEST);	// enable stencil buffer for "marking" the mask
                    glStencilFunc(GL_ALWAYS, 1, 1);	// always passes, 1 bit plane, 1 as mask
                    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);	// we set the stencil buffer to 1 where we draw any polygon
                                                                // keep if test fails, keep if test passes but buffer test fails
                                                                // replace if test passes 
                }
                
                void end_submit_mask()
                {            
                    glColorMask(1,1,1,1);	// enable framebuffer writes
                    glStencilFunc(GL_EQUAL, 1, 1);	// we draw only where the stencil is 1 (where the mask was drawn)
                    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);	// don't change the stencil buffer    
                }
                
                void end_mask()
                {              
                    glDisable(GL_STENCIL_TEST); 
                }
                
	};	// end struct render_handler_ogl


	// bitmap_info_ogl implementation

	bitmap_info_ogl::bitmap_info_ogl(create_empty e)
	{
		// A null texture.  Needs to be initialized later.
	}

	bitmap_info_ogl::bitmap_info_ogl(image::rgb* im)
	{
		assert(im);

		// Create the texture.

		glEnable(GL_TEXTURE_2D);
		glGenTextures(1, (GLuint*)&m_texture_id);
		glBindTexture(GL_TEXTURE_2D, m_texture_id);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST /* LINEAR_MIPMAP_LINEAR */);
	
		m_original_width = im->m_width;
		m_original_height = im->m_height;

		int	w = 1; while (w < im->m_width) { w <<= 1; }
		int	h = 1; while (h < im->m_height) { h <<= 1; }

		image::rgb*	rescaled = image::create_rgb(w, h);
		image::resample(rescaled, 0, 0, w - 1, h - 1,
				im, 0, 0, (float) im->m_width, (float) im->m_height);
	
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, rescaled->m_data);
//		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_image->m_width, m_image->m_height, GL_RGB, GL_UNSIGNED_BYTE, m_image->m_data);

		delete rescaled;
	}


	bitmap_info_ogl::bitmap_info_ogl(image::rgba* im)
	// Version of the constructor that takes an image with alpha.
	{
		assert(im);

		// Create the texture.

		glEnable(GL_TEXTURE_2D);
		glGenTextures(1, (GLuint*)&m_texture_id);
		glBindTexture(GL_TEXTURE_2D, m_texture_id);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);	// GL_NEAREST ?
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST /* LINEAR_MIPMAP_LINEAR */);
	
		m_original_width = im->m_width;
		m_original_height = im->m_height;

		int	w = 1; while (w < im->m_width) { w <<= 1; }
		int	h = 1; while (h < im->m_height) { h <<= 1; }

		if (w != im->m_width
		    || h != im->m_height)
		{
			image::rgba*	rescaled = image::create_rgba(w, h);
			image::resample(rescaled, 0, 0, w - 1, h - 1,
					im, 0, 0, (float) im->m_width, (float) im->m_height);
	
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rescaled->m_data);

			delete rescaled;
		}
		else
		{
			// Use original image directly.
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, im->m_data);
		}
	}


	void bitmap_info_ogl::set_alpha_image(int width, int height, Uint8* data)
	// Initialize this bitmap_info to an alpha image
	// containing the specified data (1 byte per texel).
	//
	// !! Munges *data in order to create mipmaps !!
	{
		assert(m_texture_id == 0);	// only call this on an empty bitmap_info
		assert(data);
		
		// Create the texture.

		glEnable(GL_TEXTURE_2D);
		glGenTextures(1, (GLuint*)&m_texture_id);
		glBindTexture(GL_TEXTURE_2D, m_texture_id);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);	// GL_NEAREST ?
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	
		m_original_width = width;
		m_original_height = height;

		#ifndef NDEBUG
		// You must use power-of-two dimensions!!
		int	w = 1; while (w < width) { w <<= 1; }
		int	h = 1; while (h < height) { h <<= 1; }
		assert(w == width);
		assert(h == height);
		#endif // not NDEBUG

		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, data);

		// Build mips.
		int	level = 1;
		while (width > 1 || height > 1)
		{
			render_handler_ogl::make_next_miplevel(&width, &height, data);
			glTexImage2D(GL_TEXTURE_2D, level, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, data);
			level++;
		}
        }
};	// end namespace gameswf



//#ifndef __MACH__
//#undef main	// SDL wackiness, but needed for macosx
//#endif
int	main(int argc, char *argv[])
{
	assert(tu_types_validate());

	const char* infile = NULL;

	for (int arg = 1; arg < argc; arg++)
	{
		if (argv[arg][0] == '-')
		{
			// Looks like an option.

			if (argv[arg][1] == 'h')
			{
				// Help.
				print_usage();
				exit(1);
			}
			else if (argv[arg][1] == 's')
			{
				// Scale.
				arg++;
				if (arg < argc)
				{
					s_scale = fclamp((float) atof(argv[arg]), 0.01f, 100.f);
				}
				else
				{
					printf("-s arg must be followed by a scale value\n");
					print_usage();
					exit(1);
				}
			}
			else if (argv[arg][1] == 'a')
			{
				// Set antialiasing on or off.
				arg++;
				if (arg < argc)
				{
					s_antialiased = atoi(argv[arg]) ? true : false;
				}
				else
				{
					printf("-a arg must be followed by 0 or 1 to disable/enable antialiasing\n");
					print_usage();
					exit(1);
				}
			}
			else if (argv[arg][1] == 'c')
			{
				// Build cache file.
				s_cache = true;
			}
			else if (argv[arg][1] == 'v')
			{
				// Be verbose; i.e. print log messages to stdout.
				s_verbose = true;

				if (argv[arg][2] == 'a')
				{
					// Enable spew re: action.
					gameswf::set_verbose_action(true);
				}
				else if (argv[arg][2] == 'p')
				{
					// Enable parse spew.
					gameswf::set_verbose_parse(true);
				}
				// ...
			}
		}
		else
		{
			infile = argv[arg];
		}
	}

	if (infile == NULL)
	{
		printf("no input file\n");
		print_usage();
		exit(1);
	}

	gameswf::register_file_opener_callback(file_opener);
	gameswf::set_log_callback(log_callback);
	//gameswf::set_antialiased(s_antialiased);
        
	SDL_sound_handler*	sound = new SDL_sound_handler;
	gameswf::set_sound_handler(sound);
        
       	gameswf::render_handler_ogl*	render = new gameswf::render_handler_ogl;
	gameswf::set_render_handler(render); 

	// Load the movie.
	tu_file*	in = new tu_file(infile, "rb");
	if (in->get_error())
	{
		printf("can't open '%s' for input\n", infile);
		delete in;
		exit(1);
	}
	gameswf::movie_interface*	m = gameswf::create_movie(in);
	delete in;
	in = NULL;

	tu_string	cache_name(infile);
	cache_name += ".cache";

	if (s_cache)
	{
		printf("\nsaving %s...\n", cache_name.c_str());
		gameswf::fontlib::save_cached_font_data(cache_name);
		exit(0);
	}
	
	// Initialize the SDL subsystems we're using.
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO /* | SDL_INIT_JOYSTICK | SDL_INIT_CDROM*/))
	{
		fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
		exit(1);
	}
	atexit(SDL_Quit);

//	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
//	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
//	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
////	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 5);
////	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1);

	int	width = int(m->get_width() * s_scale);
	int	height = int(m->get_height() * s_scale);

	// Set the video mode.
	if (SDL_SetVideoMode(width, height, 16 /* 32 */, SDL_OPENGL) == 0)
	{
		fprintf(stderr, "SDL_SetVideoMode() failed.");
		exit(1);
	}

	ogl::open();

	// Try to open cached font textures
	if (!gameswf::fontlib::load_cached_font_data(cache_name))
	{
		// Generate cached textured versions of fonts.
		gameswf::fontlib::generate_font_bitmaps();
	}

	// Turn on alpha blending.
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Turn on line smoothing.  Antialiased lines can be used to
	// smooth the outsides of shapes.
	glEnable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);	// GL_NICEST, GL_FASTEST, GL_DONT_CARE

	glMatrixMode(GL_PROJECTION);
	glOrtho(-OVERSIZE, OVERSIZE, OVERSIZE, -OVERSIZE, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Mouse state.
	int	mouse_x = 0;
	int	mouse_y = 0;
	int	mouse_buttons = 0;

	bool	paused = false;
	float	speed_scale = 1.0f;
	Uint32	last_ticks = SDL_GetTicks();
	for (;;)
	{
		Uint32	ticks = SDL_GetTicks();
		int	delta_ticks = ticks - last_ticks;
		float	delta_t = delta_ticks / 1000.f;
		last_ticks = ticks;

		if (paused == true)
		{
			delta_t = 0.0f;
		}

		// Handle input.
		SDL_Event	event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
			case SDL_KEYDOWN:
			{
				int	key = event.key.keysym.sym;

				if (key == SDLK_q || key == SDLK_ESCAPE)
				{
					goto done;
				}
				else if (key == SDLK_p)
				{
					// Toggle paused state.
					paused = ! paused;
					printf("paused = %d\n", int(paused));
				}
				else if (key == SDLK_r)
				{
					// Restart the movie.
					m->restart();
				}
#if 0
				else if (key == SDLK_MINUS)
				{
					hilite_depth--;
					printf("hilite depth = %d\n", hilite_depth);
				}
				else if (key == SDLK_EQUALS)
				{
					hilite_depth++;
					printf("hilite depth = %d\n", hilite_depth);
				}
#endif // 0
				else if (key == SDLK_LEFTBRACKET || key == SDLK_KP_MINUS)
				{
					paused = true;
					//delta_t = -0.1f;
					m->goto_frame(m->get_current_frame()-1);
				}
				else if (key == SDLK_RIGHTBRACKET || key == SDLK_KP_PLUS)
				{
					paused = true;
					//delta_t = +0.1f;
					m->goto_frame(m->get_current_frame()+1);
				}
				else if (key == SDLK_a)
				{
					// Toggle antialiasing.
					s_antialiased = !s_antialiased;
					//gameswf::set_antialiased(s_antialiased);
				}
				else if (key == SDLK_t)
				{
					// test text replacement:
					m->set_edit_text("test_text", "set_edit_text was here...\nanother line of text for you to see in the text box");
				}
				else if (key == SDLK_b)
				{
					// toggle background color.
					s_background = !s_background;
				}

				break;
			}

			case SDL_MOUSEMOTION:
				mouse_x = (int) (event.motion.x / s_scale);
				mouse_y = (int) (event.motion.y / s_scale);
				break;

			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
			{
				int	mask = 1 << (event.button.button);
				if (event.button.state == SDL_PRESSED)
				{
					mouse_buttons |= mask;
				}
				else
				{
					mouse_buttons &= ~mask;
				}
				break;
			}

			case SDL_QUIT:
				goto done;
				break;

			default:
				break;
			}
		}

		m->set_display_viewport(0, 0, width, height);
		m->set_background_alpha(s_background ? 1.0f : 0.05f);

		m->notify_mouse_state(mouse_x, mouse_y, mouse_buttons);

		m->advance(delta_t * speed_scale);

		glDisable(GL_DEPTH_TEST);	// Disable depth testing.
		glDrawBuffer(GL_BACK);

		m->display();

		SDL_GL_SwapBuffers();

		SDL_Delay(10);
	}

done:
	delete sound;
	return 0;
}


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
