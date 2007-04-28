// gameswf_3ds.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// lib3ds interface

#ifndef GAMESWF_3DS_H
#define GAMESWF_3DS_H

#if TU_CONFIG_LINK_TO_LIB3DS == 1

#include "gameswf_impl.h"
#include "base/tu_opengl_includes.h"

#include <lib3ds/types.h>

namespace gameswf
{

	struct x3ds_definition : public character_def
	{
		x3ds_definition(const char* url);
		~x3ds_definition();

		virtual character* create_character_instance(character* parent, int id);
		virtual void	display(character* inst);

	private:

		Lib3dsFile* m_file;
		float	m_sx, m_sy, m_sz; // bounding box dimensions

		void render_node(Lib3dsNode* node);
	};

	struct x3ds_instance : public character
	{
		x3ds_instance(x3ds_definition* def,	character* parent, int id);
		~x3ds_instance();

		virtual void	display();

		private:
		
		smart_ptr<x3ds_definition>	m_def;

	};

}	// end namespace gameswf

#else

#endif


#endif
