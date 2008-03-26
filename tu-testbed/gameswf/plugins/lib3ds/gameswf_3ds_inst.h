// gameswf_3ds.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Lib3ds plugin implementation for gameswf library

#ifndef GAMESWF_3DS_INST_H
#define GAMESWF_3DS_INST_H

#if TU_CONFIG_LINK_TO_LIB3DS == 1

#include <SDL_opengl.h>
#include "gameswf/gameswf_impl.h"
#include "base/tu_opengl_includes.h"
#include "gameswf_3ds_def.h"

#include <lib3ds/types.h>

#define SHADOW_SIZE 1024
#define MAX_INFO_LOG_SIZE 2048

namespace gameswf
{

	struct x3ds_object
	{
		GLfloat	m_tex_percent;
		GLfloat m_bump_percent;
		GLuint	m_glist_id;
		GLuint	m_flist_id;
	};

	struct x3ds_instance : public character
	{
		// Unique id of a gameswf resource
		enum { m_class_id = AS_PLUGIN_3DS };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return character::is(class_id);
		}

		Lib3dsCamera* m_camera;

		// <material, movieclip> map
		stringi_hash<as_value> m_map;
		
		smart_ptr<x3ds_definition>	m_def;
		Lib3dsFloat m_current_frame;

		play_state	m_play_state;
		stringi_hash<as_value>	m_variables;

		GLuint m_shadow_id, m_shadow_fb;
		GLuint m_program_id, m_vshader_id, m_fshader_id;
		GLboolean m_needs_validation;
		GLfloat m_light_pos[3];
		GLfloat	m_light_dir[3];
		GLfloat m_center[3];
		GLfloat	m_radius;
		GLfloat m_light_projection[16];
		GLfloat	m_light_modelview[16];

		x3ds_instance(x3ds_definition* def,	character* parent, int id);
		~x3ds_instance();

		virtual character_def* get_character_def() { return m_def.get_ptr();	}
		virtual void	display();
		void	update_light();
		void	set_light();
		bool	upload_shaders(GLuint &program_id, GLuint &vshader_id, GLuint &fshader_id);
		void	validate_shader();
		void	set_shader_args(GLfloat tex_p, GLfloat bump_p);
		void	generate_shadow();
		bool	invert_matrix(float dst[16], float src[16]);
		void	goto_frame(float frame);
		virtual void	advance(float delta_time);
		virtual bool	get_member(const tu_stringi& name, as_value* val);
		virtual bool	set_member(const tu_stringi& name, const as_value& val);
		virtual bool	on_event(const event_id& id);

		// binds texture to triangle (from mesh)
		void bind_material(Lib3dsMaterial* mat, float U, float V);

		// enables texture if there is material & loaded image
		void set_material(Lib3dsMaterial* mat, x3ds_object *obj);

		void create_mesh_glist(Lib3dsMesh* mesh);
		void create_mesh_flist(Lib3dsMesh* mesh, x3ds_object *obj);
		void render_node(Lib3dsNode* node, bool full);
		void clear_obj(Lib3dsNode* nodes);

//		void	update_material();
	};

}	// end namespace gameswf

#else

#endif


#endif
