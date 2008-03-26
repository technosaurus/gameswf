// gameswf_3ds.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Lib3ds plugin implementation for gameswf library

#include "base/tu_config.h"

#if TU_CONFIG_LINK_TO_LIB3DS == 1

#include <SDL_opengl.h>	// for opengl const
#include <lib3ds/file.h>
#include <lib3ds/camera.h>
#include <lib3ds/mesh.h>
#include <lib3ds/node.h>
#include <lib3ds/material.h>
#include <lib3ds/matrix.h>
#include <lib3ds/vector.h>
#include <lib3ds/light.h>

#include "gameswf_3ds_inst.h"

extern PFNGLACTIVETEXTUREPROC glActiveTexture;
extern PFNGLGENFRAMEBUFFERSEXTPROC glGenFramebuffersEXT;
extern PFNGLBINDFRAMEBUFFEREXTPROC glBindFramebufferEXT;
extern PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2DEXT;
extern PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glCheckFramebufferStatusEXT;
extern PFNGLDELETEPROGRAMPROC glDeleteProgram;
extern PFNGLDELETESHADERPROC glDeleteShader;
extern PFNGLCREATESHADERPROC glCreateShader;
extern PFNGLSHADERSOURCEPROC glShaderSource;
extern PFNGLCOMPILESHADERPROC glCompileShader;
extern PFNGLGETSHADERIVPROC glGetShaderiv;
extern PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
extern PFNGLCREATEPROGRAMPROC glCreateProgram;
extern PFNGLATTACHSHADERPROC glAttachShader;
extern PFNGLLINKPROGRAMPROC glLinkProgram;
extern PFNGLGETPROGRAMIVPROC glGetProgramiv;
extern PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
extern PFNGLVALIDATEPROGRAMPROC glValidateProgram;
extern PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
extern PFNGLUNIFORM1FPROC glUniform1f;
extern PFNGLUNIFORM1IPROC glUniform1i;
extern PFNGLUSEPROGRAMPROC glUseProgram;

void gluPerspective(float fov, float aspect, float znear, float zfar);
void gluLookAt(float eyex, float eyey, float eyez, float centerx, float centery, 
							 float centerz, float upx, float upy, float upz);

// shaders's bodies
#include "gameswf_3ds_shader.h"

namespace gameswf
{

	void	as_3d_play(const fn_call& fn)
	{
		x3ds_instance* x3ds = cast_to<x3ds_instance>(fn.this_ptr);
		assert(x3ds);

		x3ds->m_play_state = character::PLAY;
	}

	void	as_3d_stop(const fn_call& fn)
	{
		x3ds_instance* x3ds = cast_to<x3ds_instance>(fn.this_ptr);
		assert(x3ds);

		x3ds->m_play_state = character::STOP;
	}

	void	as_3d_goto_and_play(const fn_call& fn)
	{
		x3ds_instance* x3ds = cast_to<x3ds_instance>(fn.this_ptr);
		assert(x3ds);

		if (fn.nargs < 1)
		{
			log_error("error: 3d_goto_and_play needs one arg\n");
			return;
		}

		x3ds->goto_frame((float) fn.arg(0).to_number());
		x3ds->m_play_state = character::PLAY;
	}

	void	as_3d_goto_and_stop(const fn_call& fn)
	{
		x3ds_instance* x3ds = cast_to<x3ds_instance>(fn.this_ptr);
		assert(x3ds);

		if (fn.nargs < 1)
		{
			log_error("error: 3d_goto_and_stop needs one arg\n");
			return;
		}

		x3ds->goto_frame((float) fn.arg(0).to_number());
		x3ds->m_play_state = character::STOP;
	}

	void	as_map_material(const fn_call& fn)
	{
		x3ds_instance* x3ds = cast_to<x3ds_instance>(fn.this_ptr);
		if (x3ds && fn.nargs == 2)
		{
			x3ds->m_map[fn.arg(0).to_tu_string()] = fn.arg(1);
		}
	}

	//
	// x3ds_instance
	//

	x3ds_instance::x3ds_instance(x3ds_definition* def, character* parent, int id)	:
		character(parent, id),
		m_def(def),
		m_current_frame(0.0f),
		m_play_state(PLAY)
	{
		assert(m_def != NULL);

		// take the first camera
		m_camera = m_def->m_file->cameras;

		set_member("mapMaterial", as_map_material);
		set_member("play", as_3d_play);
		set_member("stop", as_3d_stop);
		set_member("gotoAndStop", as_3d_goto_and_stop);
		set_member("gotoAndPlay", as_3d_goto_and_play);
		set_member("_currentframe", m_current_frame + 1);
		set_member("_totalframes", m_def->m_file->frames);
//		set_member("nextFrame", &as_3d_next_frame);
//		set_member("prevFrame", &as_3d_prev_frame);

		// Init GL
		// Shadow texture
		glActiveTexture(GL_TEXTURE2);
		glGenTextures(1, &m_shadow_id);
		glBindTexture(GL_TEXTURE_2D, m_shadow_id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, SHADOW_SIZE, SHADOW_SIZE, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
		glActiveTexture(GL_TEXTURE0);

		// TODO: if no framebuffer
		// Create FBO and attach texture to it
		glGenFramebuffersEXT(1, &m_shadow_fb);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_shadow_fb);
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, m_shadow_id, 0);
		glDrawBuffer(GL_FALSE);
		glReadBuffer(GL_FALSE);

		// verify all is well and restore state
		GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
		switch (status)
		{
			case GL_FRAMEBUFFER_COMPLETE_EXT:
				break;

			case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
				log_error("FBO configuration unsupported\n");
				return;

			default:
				log_error("FBO programmer error\n");
				return;
		}

		glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, 0);

		// TODO: use one program for all instances
		if (!upload_shaders(m_program_id, m_vshader_id, m_fshader_id))
		{
			m_program_id = 0;
		}

		Lib3dsVector bmin, bmax;
		GLfloat tmp[3];
		lib3ds_file_bounding_box_of_nodes(m_def->m_file, LIB3DS_TRUE, LIB3DS_FALSE, LIB3DS_FALSE, bmin, bmax);
		m_center[0] = (bmin[0] + bmax[0])/2;
		m_center[1] = (bmin[1] + bmax[1])/2;
		m_center[2] = (bmin[2] + bmax[2])/2;
		tmp[0] = (bmax[0] - bmin[0])/2;
		tmp[1] = (bmax[1] - bmin[1])/2;
		tmp[2] = (bmax[2] - bmin[2])/2;
		m_radius = sqrt(tmp[0]*tmp[0] + tmp[1]*tmp[1] + tmp[2]*tmp[2]);
	}

	void x3ds_instance::clear_obj(Lib3dsNode* node)
	{
		for (Lib3dsNode* p = node; p != 0; p = p->next)
		{
			if (p->user.p)
			{
				x3ds_object *obj = (x3ds_object *)p->user.p;
				glDeleteLists(obj->m_glist_id, 1);
				glDeleteLists(obj->m_flist_id, 1);
				free(obj);
			}

			clear_obj(node->childs);
		}
	}

	x3ds_instance::~x3ds_instance()
	{
		clear_obj(m_def->m_file->nodes);
		m_def->remove_camera(m_camera);

		if (glDeleteProgram && glDeleteShader)
		{
			if (m_program_id != 0)
			{
				glDeleteProgram(m_program_id);
				glDeleteShader(m_vshader_id);
				glDeleteShader(m_fshader_id);
			}
		}
	}

	void	x3ds_instance::goto_frame(float frame)
	{
		m_current_frame = fmod(frame, (float) m_def->m_file->frames);
		set_member("_currentframe", m_current_frame + 1);
		lib3ds_file_eval(m_def->m_file, m_current_frame);
	}

	void	x3ds_instance::advance(float delta_time)
	{
		on_event(event_id::ENTER_FRAME);
		if (m_play_state == PLAY)
		{
			goto_frame(m_current_frame + 1.0f);
		}
	}

	bool	x3ds_instance::get_member(const tu_stringi& name, as_value* val)
	{
		if (character::get_member(name, val) == false)
		{
			return m_variables.get(name, val);
		}
		return true;
	}

	bool	x3ds_instance::set_member(const tu_stringi& name, const as_value& val)
	{
		if (character::set_member(name, val) == false)
		{
			m_variables.set(name, val);
		};
		return true;
	}

	void	x3ds_instance::update_light()
	{
		Lib3dsLight* l = m_def->m_file->lights;
		for (int gl_light = GL_LIGHT0; gl_light <= GL_LIGHT7; gl_light++)
		{
			if (l)
			{
				// try light nodes if possible
				Lib3dsNode* ln = lib3ds_file_node_by_name(m_def->m_file, l->name, LIB3DS_LIGHT_NODE);
				if (ln)
				{
					l->color[0] = ln->data.light.col[0];
					l->color[1] = ln->data.light.col[1];
					l->color[2] = ln->data.light.col[2];
					l->position[0] = ln->data.light.pos[0];
					l->position[1] = ln->data.light.pos[1];
					l->position[2] = ln->data.light.pos[2];
				}

				// try spot nodes if possible
				Lib3dsNode* sn = lib3ds_file_node_by_name(m_def->m_file, l->name, LIB3DS_SPOT_NODE);
				if (sn)
				{
					l->spot[0] = sn->data.spot.pos[0];
					l->spot[1] = sn->data.spot.pos[1];
					l->spot[2] = sn->data.spot.pos[2];
				}

				if (gl_light == GL_LIGHT0)
				{
					m_light_pos[0] = l->position[0];
					m_light_pos[1] = l->position[1];
					m_light_pos[2] = l->position[2];

					if (l->spot_light)
					{
						m_light_dir[0] = l->spot[0] - l->position[0];
						m_light_dir[1] = l->spot[1] - l->position[1];
						m_light_dir[2] = l->spot[2] - l->position[2];
					}
					else
					{
						m_light_dir[0] = m_center[0] - l->position[0];
						m_light_dir[1] = m_center[1] - l->position[1];
						m_light_dir[2] = m_center[2] - l->position[2];
					}
				}

				l = l->next;
			}
		}
	}

	// apply light
	// Set them from light nodes if possible.
	// If not, use the light objects directly.
	void	x3ds_instance::set_light()
	{
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, m_def->m_file->ambient);

		Lib3dsLight* l = m_def->m_file->lights;
		for (int gl_light = GL_LIGHT0; gl_light <= GL_LIGHT7; gl_light++)
		{
			if (l)
			{
				// default we use light objects directly
				GLfloat color[4];
				color[0] = l->color[0];
				color[1] = l->color[1];
				color[2] = l->color[2];
				color[3] = 1;

				// default position
				GLfloat position[4];
				position[0] = l->position[0];
				position[1] = l->position[1];
				position[2] = l->position[2];
				position[3] = 1;

				static const GLfloat a[] = {0.0f, 0.0f, 0.0f, 1.0f};
				glLightfv(gl_light, GL_AMBIENT, a);
				glLightfv(gl_light, GL_DIFFUSE, color);
				glLightfv(gl_light, GL_SPECULAR, color);
				glLightfv(gl_light, GL_POSITION, position);
//				glLightf(gl_light, GL_LINEAR_ATTENUATION, l->attenuation);
//				glLightf(gl_light, GL_CONSTANT_ATTENUATION, 0);
//				glLightf(gl_light, GL_QUADRATIC_ATTENUATION, 0);

				if (l->spot_light)
				{
//					position[0] = l->spot[0] - l->position[0];
//					position[1] = l->spot[1] - l->position[1];
//					position[2] = l->spot[2] - l->position[2];
//					glLightfv(gl_light, GL_SPOT_DIRECTION, position);
//					glLightf(gl_light, GL_SPOT_CUTOFF, l->fall_off);
//					glLightf(gl_light, GL_SPOT_EXPONENT, 0);	// hack
				}

				l = l->next;
			}
		}
	}

	bool	x3ds_instance::upload_shaders(GLuint &program_id, GLuint &vshader_id, GLuint &fshader_id)
	{
		GLint success;

		// Create shader objects and specify shader text
		vshader_id = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vshader_id, 1, s_3ds_shader + 1, NULL);

		// Compile shaders and check for any errors
		glCompileShader(vshader_id);
		glGetShaderiv(vshader_id, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			GLchar info_log[MAX_INFO_LOG_SIZE];
			glGetShaderInfoLog(vshader_id, MAX_INFO_LOG_SIZE, NULL, info_log);
			log_error("Error in vertex shader compilation!\n");
			log_error("Info log: %s\n", info_log);
			return false;
		}

		fshader_id = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fshader_id, 1, s_3ds_shader + 0, NULL);

		// Compile shaders and check for any errors
		glCompileShader(fshader_id);
		glGetShaderiv(fshader_id, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			GLchar info_log[MAX_INFO_LOG_SIZE];
			glGetShaderInfoLog(fshader_id, MAX_INFO_LOG_SIZE, NULL, info_log);
			log_error("Error in fragment shader compilation!\n");
			log_error("Info log: %s\n", info_log);
			return false;
		}

		// Create program object, attach shader, then link
		program_id = glCreateProgram();
		glAttachShader(program_id, vshader_id);
		glAttachShader(program_id, fshader_id);

		glLinkProgram(program_id);
		glGetProgramiv(program_id, GL_LINK_STATUS, &success);
		if (!success)
		{
			GLchar info_log[MAX_INFO_LOG_SIZE];
			glGetProgramInfoLog(program_id, MAX_INFO_LOG_SIZE, NULL, info_log);
			log_error("Error in program linkage!\n");
			log_error("Info log: %s\n", info_log);
			return false;
		}

		// Program object has changed, so we should revalidate
		m_needs_validation = GL_TRUE;
		return true;
	}

	void	x3ds_instance::validate_shader()
	{
		if (m_needs_validation && (m_program_id != 0))
		{
			GLint success;

			glValidateProgram(m_program_id);
			glGetProgramiv(m_program_id, GL_VALIDATE_STATUS, &success);
			if (!success)
			{
				GLchar info_log[MAX_INFO_LOG_SIZE];
				glGetProgramInfoLog(m_program_id, MAX_INFO_LOG_SIZE, NULL, info_log);
				log_error("Error in program validation!\n");
				log_error("Info log: %s\n", info_log);
			}
			else
			{
				m_needs_validation = GL_FALSE;
			}
		}
	}

	void	x3ds_instance::set_shader_args(GLfloat tex_p, GLfloat bump_p)
	{
		GLint uniform_loc;

		uniform_loc = glGetUniformLocation(m_program_id, "texturePercent");
		if (uniform_loc != -1)
		{
			glUniform1f(uniform_loc, tex_p);
		}
		uniform_loc = glGetUniformLocation(m_program_id, "bumpPercent");
		if (uniform_loc != -1)
		{
			glUniform1f(uniform_loc, bump_p);
		}
		validate_shader();
	}

	void	x3ds_instance::generate_shadow()
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		GLfloat fov = 60.0f;
		GLfloat sx = m_light_pos[0] - m_center[0];
		GLfloat sy = m_light_pos[1] - m_center[1];
		GLfloat sz = m_light_pos[2] - m_center[2];
		GLfloat distance = sqrt(sx*sx + sy*sy + sz*sz);
		GLfloat znear = (distance - m_radius > 1.0) ? distance - m_radius : 1.0f;
		GLfloat zfar = distance + m_radius;

		gluPerspective(fov, 1.0f, znear, zfar);

		glGetFloatv(GL_PROJECTION_MATRIX, m_light_projection);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		gluLookAt(m_light_pos[0], m_light_pos[1], m_light_pos[2], 
				  m_light_dir[0], m_light_dir[1], m_light_dir[2], 0.0f, 1.0f, 0.0f);
		glGetFloatv(GL_MODELVIEW_MATRIX, m_light_modelview);

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_shadow_fb);
		glPushAttrib(GL_VIEWPORT_BIT); // save the viewport 
		glViewport (0, 0, SHADOW_SIZE, SHADOW_SIZE);

		// Clear the depth buffer only
		glClear(GL_DEPTH_BUFFER_BIT);

		// All we care about here is resulting depth values
		glColorMask(0, 0, 0, 0);

		// Overcome imprecision
		glEnable(GL_POLYGON_OFFSET_FILL);

		// Draw objects in the scene
		for (Lib3dsNode* p = m_def->m_file->nodes; p != 0; p = p->next)
		{
			render_node(p, false);
		}
		
		// Restore normal drawing state
		glColorMask(1, 1, 1, 1);
		glDisable(GL_POLYGON_OFFSET_FILL);

		glPopAttrib(); // restore the viewport 
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}

	bool	x3ds_instance::invert_matrix(float dst[16], float src[16])
	{
		#define SWAP_ROWS(a, b) { float *_tmp = a; (a)=(b); (b)=_tmp; }
		#define MAT(m,r,c) (m)[(c)*4+(r)]

		float wtmp[4][8];
		float m0, m1, m2, m3, s;
		float *r0, *r1, *r2, *r3;

		r0 = wtmp[0], r1 = wtmp[1], r2 = wtmp[2], r3 = wtmp[3];

		r0[0] = MAT(src,0,0), r0[1] = MAT(src,0,1),
		r0[2] = MAT(src,0,2), r0[3] = MAT(src,0,3),
		r0[4] = 1.0, r0[5] = r0[6] = r0[7] = 0.0,

		r1[0] = MAT(src,1,0), r1[1] = MAT(src,1,1),
		r1[2] = MAT(src,1,2), r1[3] = MAT(src,1,3),
		r1[5] = 1.0, r1[4] = r1[6] = r1[7] = 0.0,

		r2[0] = MAT(src,2,0), r2[1] = MAT(src,2,1),
		r2[2] = MAT(src,2,2), r2[3] = MAT(src,2,3),
		r2[6] = 1.0, r2[4] = r2[5] = r2[7] = 0.0,

		r3[0] = MAT(src,3,0), r3[1] = MAT(src,3,1),
		r3[2] = MAT(src,3,2), r3[3] = MAT(src,3,3),
		r3[7] = 1.0, r3[4] = r3[5] = r3[6] = 0.0;

		/* choose pivot - or die */
		if (fabs(r3[0])>fabs(r2[0])) SWAP_ROWS(r3, r2);
		if (fabs(r2[0])>fabs(r1[0])) SWAP_ROWS(r2, r1);
		if (fabs(r1[0])>fabs(r0[0])) SWAP_ROWS(r1, r0);
		if (0.0 == r0[0])  return false;

		/* eliminate first variable     */
		m1 = r1[0]/r0[0]; m2 = r2[0]/r0[0]; m3 = r3[0]/r0[0];
		s = r0[1]; r1[1] -= m1 * s; r2[1] -= m2 * s; r3[1] -= m3 * s;
		s = r0[2]; r1[2] -= m1 * s; r2[2] -= m2 * s; r3[2] -= m3 * s;
		s = r0[3]; r1[3] -= m1 * s; r2[3] -= m2 * s; r3[3] -= m3 * s;
		s = r0[4];
		if (s != 0.0) { r1[4] -= m1 * s; r2[4] -= m2 * s; r3[4] -= m3 * s; }
		s = r0[5];
		if (s != 0.0) { r1[5] -= m1 * s; r2[5] -= m2 * s; r3[5] -= m3 * s; }
		s = r0[6];
		if (s != 0.0) { r1[6] -= m1 * s; r2[6] -= m2 * s; r3[6] -= m3 * s; }
		s = r0[7];
		if (s != 0.0) { r1[7] -= m1 * s; r2[7] -= m2 * s; r3[7] -= m3 * s; }

		/* choose pivot - or die */
		if (fabs(r3[1])>fabs(r2[1])) SWAP_ROWS(r3, r2);
		if (fabs(r2[1])>fabs(r1[1])) SWAP_ROWS(r2, r1);
		if (0.0 == r1[1])  return false;

		/* eliminate second variable */
		m2 = r2[1]/r1[1]; m3 = r3[1]/r1[1];
		r2[2] -= m2 * r1[2]; r3[2] -= m3 * r1[2];
		r2[3] -= m2 * r1[3]; r3[3] -= m3 * r1[3];
		s = r1[4]; if (0.0 != s) { r2[4] -= m2 * s; r3[4] -= m3 * s; }
		s = r1[5]; if (0.0 != s) { r2[5] -= m2 * s; r3[5] -= m3 * s; }
		s = r1[6]; if (0.0 != s) { r2[6] -= m2 * s; r3[6] -= m3 * s; }
		s = r1[7]; if (0.0 != s) { r2[7] -= m2 * s; r3[7] -= m3 * s; }

		/* choose pivot - or die */
		if (fabs(r3[2])>fabs(r2[2])) SWAP_ROWS(r3, r2);
		if (0.0 == r2[2])  return false;

		/* eliminate third variable */
		m3 = r3[2]/r2[2];
		r3[3] -= m3 * r2[3], r3[4] -= m3 * r2[4],
		r3[5] -= m3 * r2[5], r3[6] -= m3 * r2[6],
		r3[7] -= m3 * r2[7];

		/* last check */
		if (0.0 == r3[3]) return false;

		s = 1.0f/r3[3];              /* now back substitute row 3 */
		r3[4] *= s; r3[5] *= s; r3[6] *= s; r3[7] *= s;

		m2 = r2[3];                 /* now back substitute row 2 */
		s  = 1.0f/r2[2];
		r2[4] = s * (r2[4] - r3[4] * m2), r2[5] = s * (r2[5] - r3[5] * m2),
		r2[6] = s * (r2[6] - r3[6] * m2), r2[7] = s * (r2[7] - r3[7] * m2);
		m1 = r1[3];
		r1[4] -= r3[4] * m1, r1[5] -= r3[5] * m1,
		r1[6] -= r3[6] * m1, r1[7] -= r3[7] * m1;
		m0 = r0[3];
		r0[4] -= r3[4] * m0, r0[5] -= r3[5] * m0,
		r0[6] -= r3[6] * m0, r0[7] -= r3[7] * m0;

		m1 = r1[2];                 /* now back substitute row 1 */
		s  = 1.0f/r1[1];
		r1[4] = s * (r1[4] - r2[4] * m1), r1[5] = s * (r1[5] - r2[5] * m1),
		r1[6] = s * (r1[6] - r2[6] * m1), r1[7] = s * (r1[7] - r2[7] * m1);
		m0 = r0[2];
		r0[4] -= r2[4] * m0, r0[5] -= r2[5] * m0,
		r0[6] -= r2[6] * m0, r0[7] -= r2[7] * m0;

		m0 = r0[1];                 /* now back substitute row 0 */
		s  = 1.0f/r0[0];
		r0[4] = s * (r0[4] - r1[4] * m0), r0[5] = s * (r0[5] - r1[5] * m0),
		r0[6] = s * (r0[6] - r1[6] * m0), r0[7] = s * (r0[7] - r1[7] * m0);

		MAT(dst,0,0) = r0[4]; MAT(dst,0,1) = r0[5],
		MAT(dst,0,2) = r0[6]; MAT(dst,0,3) = r0[7],
		MAT(dst,1,0) = r1[4]; MAT(dst,1,1) = r1[5],
		MAT(dst,1,2) = r1[6]; MAT(dst,1,3) = r1[7],
		MAT(dst,2,0) = r2[4]; MAT(dst,2,1) = r2[5],
		MAT(dst,2,2) = r2[6]; MAT(dst,2,3) = r2[7],
		MAT(dst,3,0) = r3[4]; MAT(dst,3,1) = r3[5],
		MAT(dst,3,2) = r3[6]; MAT(dst,3,3) = r3[7]; 

		return true;

		#undef MAT
		#undef SWAP_ROWS
	}

	void	x3ds_instance::display()
	{
		assert(m_def != NULL);

		if (m_def->m_file == NULL)
		{
			return;
		}

		//update_material();

		// save GL state
		glPushAttrib (GL_ALL_ATTRIB_BITS);	
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();

		// apply gameSWF matrix

		rect bound;
		m_def->get_bound(&bound);

		matrix m = get_parent()->get_world_matrix();
		m.transform(&bound);

		// get viewport size
		GLint vp[4]; 
		glGetIntegerv(GL_VIEWPORT, vp); 
		int vp_width = vp[2];
		int vp_height = vp[3];

		bound.twips_to_pixels();
		int w = (int) (bound.m_x_max - bound.m_x_min);
		int h = (int) (bound.m_y_max - bound.m_y_min);
		int x = (int) bound.m_x_min;
		int y = (int) bound.m_y_min;
		glViewport(x, vp_height - y - h, w, h);

		// set 3D params

		//glClear(GL_DEPTH_BUFFER_BIT);
		glDepthFunc(GL_LEQUAL);
		glEnable(GL_DEPTH_TEST);
		glCullFace(GL_BACK);
		glEnable(GL_CULL_FACE);

		GLfloat factor = 4.0f;
		glPolygonOffset(factor, 0.0f);

//		glEnable(GL_POLYGON_SMOOTH);
//		glEnable(GL_BLEND);
//		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_BLEND);

		update_light();
		generate_shadow();

		// set GL matrix to identity
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		assert(m_camera);

		Lib3dsNode* cnode = lib3ds_file_node_by_name(m_def->m_file, m_camera->name, LIB3DS_CAMERA_NODE);
		Lib3dsNode* tnode = lib3ds_file_node_by_name(m_def->m_file, m_camera->name, LIB3DS_TARGET_NODE);

		float* target = tnode ? tnode->data.target.pos : m_camera->target;

		float	view_angle;
		float roll;
		float* camera_pos;
		if (cnode)
		{
			view_angle = cnode->data.camera.fov;
			roll = cnode->data.camera.roll;
			camera_pos = cnode->data.camera.pos;
		}
		else
		{
			view_angle = m_camera->fov;
			roll = m_camera->roll;
			camera_pos = m_camera->position;
		}

		float ffar = m_camera->far_range;
		float nnear = m_camera->near_range <= 0 ? ffar * .001f : m_camera->near_range;
		float aspect = (float) w / h;

		gluPerspective(view_angle, aspect, nnear, ffar);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glRotatef(-90., 1.0, 0., 0.);

		// apply camera matrix
		Lib3dsMatrix cmatrix;
		lib3ds_matrix_camera(cmatrix, camera_pos, target, roll);
		glMultMatrixf(&cmatrix[0][0]);

		// Set texture matrix for shadow mapping (for active texture unit)
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, m_shadow_id);
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		glTranslatef(0.5f, 0.5f, 0.5f);
		glScalef(0.5f, 0.5f, 0.5f);
		glMultMatrixf(m_light_projection);
		glMultMatrixf(m_light_modelview);
		GLfloat camera_modelview[16], temp_matrix[16];
		glGetFloatv(GL_MODELVIEW_MATRIX, camera_modelview);
		invert_matrix(temp_matrix, camera_modelview);
		glMultMatrixf(temp_matrix);
		glMatrixMode(GL_MODELVIEW);
		glActiveTexture(GL_TEXTURE0);
		glClear(GL_DEPTH_BUFFER_BIT);

		// apply light
		set_light();

		// draw 3D model

		glUseProgram(m_program_id);
		GLint uniform_loc = glGetUniformLocation(m_program_id, "textureMap");
		if (uniform_loc != -1)
		{
			glUniform1i(uniform_loc, 0);
		}
		uniform_loc = glGetUniformLocation(m_program_id, "bumpMap");
		if (uniform_loc != -1)
		{
			glUniform1i(uniform_loc, 1);
		}
		uniform_loc = glGetUniformLocation(m_program_id, "shadowMap");
		if (uniform_loc != -1)
		{
			glUniform1i(uniform_loc, 2);
		}

		for (Lib3dsNode* p = m_def->m_file->nodes; p != 0; p = p->next)
		{
			render_node(p, true);
		}

		glUseProgram(0);

#ifdef _DEBUG
    if (glGetError() != GL_NO_ERROR)
		{
			fprintf(stderr, "GL Error!\n");
		}
#endif

		// restore openGL state
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
		glPopAttrib();
	}

	void x3ds_instance::set_material(Lib3dsMaterial* mat, x3ds_object *obj)
	{
		obj->m_tex_percent = 0.0f;
		obj->m_bump_percent = 0.0f;
		if (mat)
		{
			glMaterialfv(GL_FRONT, GL_AMBIENT, mat->ambient);
			glMaterialfv(GL_FRONT, GL_DIFFUSE, mat->diffuse);
			glMaterialfv(GL_FRONT, GL_SPECULAR, mat->specular);
			glMaterialf(GL_FRONT, GL_SHININESS, pow(2.0, 10.0 * mat->shininess));

			x3ds_texture *tex = NULL;
			if (mat->texture1_map.name)
			{
				if (m_def->m_texture.get(mat->texture1_map.name, &tex))
				{
					glBindTexture(GL_TEXTURE_2D, tex->m_tex_id);
					obj->m_tex_percent = mat->texture1_map.percent;
				}
			}
			if (mat->bump_map.name)
			{
				if (m_def->m_bump.get(mat->bump_map.name, &tex))
				{
					glActiveTexture(GL_TEXTURE1);
					glBindTexture(GL_TEXTURE_2D, tex->m_tex_id);
					glActiveTexture(GL_TEXTURE0);
					obj->m_bump_percent = mat->bump_map.percent;
				}
			}
		}
		else
		{
			// no material, assume the default
			static const Lib3dsRgba a = {0.2f, 0.2f, 0.2f, 1.0f};
			static const Lib3dsRgba d = {0.8f, 0.8f, 0.8f, 1.0f};
			static const Lib3dsRgba s = {0.0f, 0.0f, 0.0f, 1.0f};
			glMaterialfv(GL_FRONT, GL_AMBIENT, a);
			glMaterialfv(GL_FRONT, GL_DIFFUSE, d);
			glMaterialfv(GL_FRONT, GL_SPECULAR, s);
			glMaterialf(GL_FRONT, GL_SHININESS, pow(2.0, 10.0 * 0.5));
		}
	}

	void x3ds_instance::bind_material(Lib3dsMaterial* mat, float U, float V)
	{
		if (mat)
		{
			x3ds_texture *tex = NULL;
			if (m_def->m_texture.get(mat->texture1_map.name, &tex))
			{
				float s = U * tex->m_scale_x;
				float t = tex->m_scale_y * (1 - V);
				glTexCoord2f(s, t);
			}
			else if (m_def->m_bump.get(mat->bump_map.name, &tex))
			{
				float s = U * tex->m_scale_x;
				float t = tex->m_scale_y * (1 - V);
				glTexCoord2f(s, t);
			}
		}
	}

	void x3ds_instance::create_mesh_glist(Lib3dsMesh* mesh)
	{
		Lib3dsVector* normalL = (Lib3dsVector*) malloc(3 * sizeof(Lib3dsVector) * mesh->faces);
		Lib3dsMatrix m;
		lib3ds_matrix_copy(m, mesh->matrix);
		lib3ds_matrix_inv(m);
		glMultMatrixf(&m[0][0]);

		lib3ds_mesh_calculate_normals(mesh, normalL);

		glBegin(GL_TRIANGLES);
		for (Uint32 p = 0; p < mesh->faces; ++p)
		{
			Lib3dsFace* f = &mesh->faceL[p];
			
			glNormal3fv(f->normal);
			for (int i = 0; i < 3; ++i)
			{
				glNormal3fv(normalL[3 * p + i]);
				glVertex3fv(mesh->pointL[f->points[i]].pos);
			}
		}
		glEnd();

		free(normalL);
	}

	void x3ds_instance::create_mesh_flist(Lib3dsMesh* mesh, x3ds_object *obj)
	{
		Lib3dsVector* normalL = (Lib3dsVector*) malloc(3 * sizeof(Lib3dsVector) * mesh->faces);
		Lib3dsMatrix m;
		lib3ds_matrix_copy(m, mesh->matrix);
		lib3ds_matrix_inv(m);
		glMultMatrixf(&m[0][0]);

		lib3ds_mesh_calculate_normals(mesh, normalL);

		Lib3dsFace* f = &mesh->faceL[0];
		Lib3dsMaterial* mat = f->material[0] ? 
			lib3ds_file_material_by_name(m_def->m_file, f->material) : NULL;		
		
		set_material(mat, obj);

		glBegin(GL_TRIANGLES);
		for (Uint32 p = 0; p < mesh->faces; ++p)
		{
			Lib3dsFace* f = &mesh->faceL[p];

			glNormal3fv(f->normal);
			for (int i = 0; i < 3; ++i)
			{
				glNormal3fv(normalL[3 * p + i]);
				if (mesh->texelL)
				{
					bind_material(mat, mesh->texelL[f->points[i]][0], mesh->texelL[f->points[i]][1]);
				}
				glVertex3fv(mesh->pointL[f->points[i]].pos);
			}
		}
		glEnd();

		free(normalL);
	}

	void x3ds_instance::render_node(Lib3dsNode* node, bool full)
	{
		for (Lib3dsNode* p = node->childs; p != 0; p = p->next)
		{
			render_node(p, full);
		}

		if (node->type == LIB3DS_OBJECT_NODE)
		{
			if (strcmp(node->name, "$$$DUMMY") == 0)
			{
				return;
			}

			Lib3dsMesh* mesh = lib3ds_file_mesh_by_name(m_def->m_file, node->data.object.morph);
			if (mesh == NULL)
			{
				mesh = lib3ds_file_mesh_by_name(m_def->m_file, node->name);
			}
			assert(mesh);
			
			if (!&mesh->faceL[0])
			{
//				log_error("lib3ds: mesh has no faces\n");
				return;
			}

			x3ds_object *obj;
			if (!node->user.p)	// generate x3ds_object
			{
				obj = new x3ds_object;
				node->user.p = obj;

				obj->m_glist_id = glGenLists(1);
				glNewList(obj->m_glist_id, GL_COMPILE);
					create_mesh_glist(mesh);
				glEndList();

				obj->m_flist_id = glGenLists(1);
				glNewList(obj->m_flist_id, GL_COMPILE);
					create_mesh_flist(mesh, obj);
				glEndList();
			}
			else
			{
				obj = (x3ds_object *)node->user.p;
			}

			// exec mesh list
			glPushMatrix();
			Lib3dsObjectData* d = &node->data.object;
			glMultMatrixf(&node->matrix[0][0]);
			glTranslatef( - d->pivot[0], - d->pivot[1], - d->pivot[2]);

			if (full)
			{
				set_shader_args(obj->m_tex_percent, obj->m_bump_percent);
				glCallList(obj->m_flist_id);
			}
			else
			{
				glCallList(obj->m_glist_id);
			}
			glPopMatrix();
		}
	}

	bool	x3ds_instance::on_event(const event_id& id)
	// Dispatch event handler(s), if any.
	{
		// Keep m_as_environment alive during any method calls!
		smart_ptr<as_object>	this_ptr(this);

		bool called = false;

		// Check for member function.
		const tu_stringi&	method_name = id.get_function_name().to_tu_stringi();
		if (method_name.length() > 0)
		{
			as_value	method;
			if (get_member(method_name, &method))
			{
				as_environment env;
				gameswf::call_method(method, &env, this, 0, env.get_top_index());
				called = true;
			}
		}
		return called;
	}

#if 0
	// useless stuff ?
	void	x3ds_instance::update_material()
	{
		GLint aux_buffers;
		glGetIntegerv(GL_AUX_BUFFERS, &aux_buffers);
		if (aux_buffers < 2)
		{
			static int n = 0;
			if (n < 1)
			{
				n++;
				log_error("Your video card does not have AUX buffers, can't snapshot movieclips\n");
			}
			return;
		}

		glPushAttrib(GL_ALL_ATTRIB_BITS);

		// update textures
		for (stringi_hash<smart_ptr <bitmap_info> >::iterator it = 
			m_material.begin(); it != m_material.end(); ++it)
		{
			as_value target = m_map[it->first];
			character* ch = find_target(target);
			if (ch)
			{
				if (ch->get_parent() == NULL)
				{
					log_error("Can't snapshot _root movieclip, material '%s'\n", it->first.c_str());
					continue;
				}

				GLint draw_buffer;
				glGetIntegerv(GL_DRAW_BUFFER, &draw_buffer);

				glDrawBuffer(GL_AUX1);
				glReadBuffer(GL_AUX1);

				// save "ch" matrix
				matrix ch_matrix = ch->get_matrix();

				float ch_width = TWIPS_TO_PIXELS(ch->get_width());
				float ch_height = TWIPS_TO_PIXELS(ch->get_height());

				// get viewport size
        GLint vp[4]; 
        glGetIntegerv(GL_VIEWPORT, vp); 
				int vp_width = vp[2];
				int vp_height = vp[3];

				// get texture size
				int	tw = 1; while (tw < ch_width) { tw <<= 1; }
				int	th = 1; while (th < ch_height) { th <<= 1; }

				// texture size must be <= viewport size
				if (tw > vp_width)
				{
					tw = vp_width;
				}
				if (th > vp_height)
				{
					th = vp_height;
				}

				ch->set_member("_width", tw);
				ch->set_member("_height", th);

				rect bound;
				ch->get_bound(&bound);

				// parent world matrix moves point(0,0) to "pzero"
				matrix mparent = ch->get_parent()->get_world_matrix();
				point pzero;
				mparent.transform_by_inverse(&pzero, point(0, 0));

				// after transformation of "ch" matrix left-top corner is in point(bound.m_x_min, bound.m_y_min),
				// therefore we need to move point(bound.m_x_min, bound.m_y_min) to point(pzero.m_x, pzero.m_y)
				// that "ch" movieclip's left-top corner will be in point(0,0) of screen
				matrix m = ch->get_matrix();
				float xt = (pzero.m_x - bound.m_x_min) / m.get_x_scale();
				float yt = (pzero.m_y - bound.m_y_min) / m.get_y_scale();

				// move "ch" to left-bottom corner (as point of origin of OpenGL is in left-bottom)
				// later glCopyTexImage2D will copy snapshot of "ch" into texture
				yt += PIXELS_TO_TWIPS(vp_height - th) / m.get_y_scale();
				m.concatenate_translation(xt, yt);

				ch->set_matrix(m);

				glClearColor(1, 1, 1, 1);
				glClear(GL_COLOR_BUFFER_BIT);

				ch->display();

				// restore "ch" matrix
				ch->set_matrix(ch_matrix);

				smart_ptr<bitmap_info> bi = it->second;
				if (bi->m_texture_id == 0)
				{
					glGenTextures(1, (GLuint*) &bi->m_texture_id);
					bi->m_original_height = (int) ch_height;
					bi->m_original_width = (int) ch_width;
				}
				glBindTexture(GL_TEXTURE_2D, bi->m_texture_id);
				glEnable(GL_TEXTURE_2D);
				glDisable(GL_TEXTURE_GEN_S);
				glDisable(GL_TEXTURE_GEN_T);				

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);	// GL_NEAREST ?
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

				glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0,0, tw, th, 0);

				glDisable(GL_TEXTURE_2D);

				glDrawBuffer(draw_buffer);
				glReadBuffer(draw_buffer);
			}
		}
		glPopAttrib();
	}

#endif

} // end of namespace gameswf

#endif
