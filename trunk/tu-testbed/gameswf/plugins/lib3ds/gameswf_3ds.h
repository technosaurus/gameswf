// gameswf_3ds.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Lib3ds plugin implementation for gameswf library

#ifndef GAMESWF_3DS_H
#define GAMESWF_3DS_H

#if TU_CONFIG_LINK_TO_LIB3DS == 1

#include "gameswf/gameswf_impl.h"
#include "base/tu_opengl_includes.h"

#include <lib3ds/types.h>

namespace gameswf
{

	struct x3ds_definition : public character_def
	{
		Lib3dsFile* m_file;
		float	m_sx, m_sy, m_sz; // bounding box dimensions
		Lib3dsVector m_bmin, m_bmax;
		float m_size;
		float	m_cx, m_cy, m_cz; // bounding box center
		int m_lightList;
		rect	m_rect;

		x3ds_definition(const char* url);
		~x3ds_definition();

		virtual character* create_character_instance(character* parent, int id);
		virtual void get_bound(rect* bound)
		{
			*bound = m_rect;
		}

		void set_bound(const rect& bound)
		{
			m_rect = bound;
		}

		Lib3dsCamera* create_camera();
		void remove_camera(Lib3dsCamera* camera);
	};

	struct x3ds_instance : public character
	{
		Lib3dsCamera* m_camera;

		// textures required for 3D model
		// <texture name, bitmap> m_material
		stringi_hash< smart_ptr<bitmap_info> > m_material;

		// <material, movieclip> map
		stringi_hash<as_value> m_map;
		
		smart_ptr<x3ds_definition>	m_def;
		Lib3dsFloat m_current_frame;
		hash<Lib3dsNode*, GLuint> m_mesh_list;

		play_state	m_play_state;
		stringi_hash<as_value>	m_variables;

		x3ds_instance(x3ds_definition* def,	character* parent, int id);
		~x3ds_instance();

		virtual character_def* get_character_def() { return m_def.get_ptr();	}
		virtual void	display();
		void	set_light();
		virtual void	advance(float delta_time);
		virtual bool	get_member(const tu_stringi& name, as_value* val);
		virtual bool	set_member(const tu_stringi& name, const as_value& val);
		virtual x3ds_instance* cast_to_3ds() { return this; }
		virtual bool	on_event(const event_id& id);

		// binds texture to triangle (from mesh)
		void bind_material(Lib3dsMaterial* mat, float U, float V);

		// enables texture if there is material & loaded image
		void set_material(Lib3dsMaterial* mat);

		// load image & create texture or get bitmap_info pointer to loaded texture
		void bind_texture(const char* finame);

		void create_mesh_list(Lib3dsMesh* mesh);
		void render_node(Lib3dsNode* node);

//		void	update_material();
	};

}	// end namespace gameswf

#else

#endif


#endif
