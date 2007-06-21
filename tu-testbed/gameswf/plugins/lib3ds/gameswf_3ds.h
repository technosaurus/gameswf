// gameswf_3ds.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Lib3ds plugin implementation for gameswf library

#ifndef GAMESWF_3DS_H
#define GAMESWF_3DS_H

#if TU_CONFIG_LINK_TO_LIB3DS == 1

#include "../../gameswf_impl.h"
#include "base/tu_opengl_includes.h"

#include <lib3ds/types.h>

namespace gameswf
{

	struct x3ds_definition : public character_def
	{
		x3ds_definition(const char* url);
		~x3ds_definition();

		virtual character* create_character_instance(character* parent, int id);

		Lib3dsCamera* create_camera();
		void remove_camera(Lib3dsCamera* camera);

		Lib3dsFile* m_file;
		float	m_sx, m_sy, m_sz; // bounding box dimensions

	private:

		Lib3dsVector m_bmin, m_bmax;
		float m_size;
		float	m_cx, m_cy, m_cz; // bounding box center
		int m_lightList;
	};

	struct x3ds_instance : public character
	{
		x3ds_instance(x3ds_definition* def,	character* parent, int id);
		~x3ds_instance();

		virtual void	display();
		virtual void	advance(float delta_time);
		virtual bool	get_member(const tu_stringi& name, as_value* val);
		virtual bool	set_member(const tu_stringi& name, const as_value& val);
		virtual x3ds_instance* cast_to_3ds() { return this; }

		void	apply_matrix(float* target, float* camera_pos);

		Lib3dsCamera* m_camera;
		int m_texture_id;

		// textures required for 3D model
		string_hash< smart_ptr<bitmap_info> > m_texture;

		private:
		
		smart_ptr<x3ds_definition>	m_def;
		Lib3dsFloat m_current_frame;
		Lib3dsMatrix m_matrix;
		GLuint m_mesh_list;

		// binds texture to triangle (from mesh)
		void bind_material(Lib3dsMaterial* mat, float U, float V);

		// enables texture if there is material & loaded image
		void set_material(Lib3dsMaterial* mat);

		// load image & create texture or get bitmap_info pointer to loaded texture
		bitmap_info* get_texture(const char* finame);

		void create_mesh_list(Lib3dsMesh* mesh);
		void render_node(Lib3dsNode* node);
	};

}	// end namespace gameswf

#else

#endif


#endif
