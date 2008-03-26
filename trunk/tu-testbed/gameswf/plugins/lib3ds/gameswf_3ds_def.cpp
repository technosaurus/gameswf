// gameswf_3ds_def.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Lib3ds plugin implementation for gameswf library

#include "base/tu_config.h"

#if TU_CONFIG_LINK_TO_LIB3DS == 1

#include "base/tu_opengl_includes.h"

#include <lib3ds/file.h>
#include <lib3ds/camera.h>
#include <lib3ds/mesh.h>
#include <lib3ds/node.h>
#include <lib3ds/material.h>
#include <lib3ds/matrix.h>
#include <lib3ds/vector.h>
#include <lib3ds/light.h>

#include "gameswf_3ds_def.h"
#include "gameswf_3ds_inst.h"

extern PFNGLACTIVETEXTUREPROC glActiveTexture;

namespace gameswf
{

	x3ds_definition::x3ds_definition(const char* url) : m_file(NULL)
	{
		m_file = lib3ds_file_load(url);
		if (m_file == NULL)
		{
			static int n = 0;
			if (n < 1)
			{
				n++;
				log_error("can't load '%s'\n", url);
			}
			return;
		}

		// load textures
	  for (Lib3dsMaterial* p = m_file->materials; p != 0; p = p->next)
		{
			load_texture(p->texture1_map.name);
			load_bump(p->bump_map.name);
		}

		ensure_camera();
		ensure_lights();
		ensure_nodes();

		lib3ds_file_eval(m_file, 0.);
	}

	x3ds_definition::~x3ds_definition()
	{
		x3ds_texture *tex;
		for (Lib3dsMaterial* p = m_file->materials; p != 0; p = p->next)
		{
			if (m_texture.get(p->texture1_map.name, &tex))
			{
				free(tex);
			}
			if (m_bump.get(p->bump_map.name, &tex))
			{
				free(tex);
			}
		}

		if (m_file)
		{
			lib3ds_file_free(m_file);
		}
	}

	void x3ds_definition::load_texture(const char* infile)
	{
		if (infile == NULL)
		{
			return;
		}			

		if (strlen(infile) == 0)
		{
			return;
		}			

		if (m_texture.get(infile, NULL))
		{
			// loaded already
			return;
		}

		// try to load & create texture from file

		// is path relative ?
		tu_string url = get_workdir();
		if (strstr(infile, ":") || infile[0] == '/')
		{
			url = "";
		}
		url += infile;

		image::rgb* im = image::read_jpeg(url.c_str());
		if (im)
		{
			int width = im->m_width;
			int height = im->m_height;
			int size = im->m_height * im->m_pitch; //width * height * bpp;

			unsigned char* data = new unsigned char[size];
			unsigned char* copy_data = im->m_data; //ilGetData();
			memcpy(data, copy_data, size);
			x3ds_texture *tex = new x3ds_texture;
			glGenTextures(1, &tex->m_tex_id);
			glBindTexture(GL_TEXTURE_2D, tex->m_tex_id);
			GLint	tex_size = 1;
			while (tex_size < width)	{ tex_size <<= 1; }
			while (tex_size < height)	{ tex_size <<= 1; }
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_size, tex_size, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);

			tex->m_scale_x = (GLfloat)width / (GLfloat)tex_size;
			tex->m_scale_y = (GLfloat)height / (GLfloat)tex_size;
			m_texture[infile] = tex;

			delete im;
			delete [] data;
		}
	}

	void x3ds_definition::load_bump(const char* infile)
	{
		if (infile == NULL)
		{
			return;
		}			

		if (strlen(infile) == 0)
		{
			return;
		}			

		if (m_bump.get(infile, NULL))
		{
			// loaded already
			return;
		}

		// try to load & create texture from file

		// is path relative ?
		tu_string url = get_workdir();
		if (strstr(infile, ":") || infile[0] == '/')
		{
			url = "";
		}
		url += infile;

		image::rgb* im = image::read_jpeg(url.c_str());
		if (im)
		{
			int width = im->m_width; //ilGetInteger(IL_IMAGE_WIDTH); 
			int height = im->m_height; //ilGetInteger(IL_IMAGE_HEIGHT);
//			int bpp = ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL);
			int size = im->m_height * im->m_pitch; //width * height * bpp;
			int bpp = 3; //hack

			unsigned char* data = im->m_data; //ilGetData();
			GLfloat *texels = new GLfloat[width * height * 4];
			GLfloat nx, ny, nz, scale, nx1, nx2, ny1, ny2;
			GLint u, v, du1, dv1, du2, dv2;

			for (v = 0; v < height; v++)
			{
				for (u = 0; u < width; u++)
				{
					du1 = (u > 0) ? u - 1 : u; // u - 1
					dv1 = (v > 0) ? v - 1 : v; // v - 1
					du2 = (u < width - 1) ? u + 1 : u; // u + 1
					dv2 = (v < height - 1) ? v + 1 : v; // v + 1
					// NTSC grayscale conversion
					nx1 = 0.299*data[(v*width*bpp)+(du1*bpp)+0]
						+ 0.587*data[(v*width*bpp)+(du1*bpp)+1]
						+ 0.114*data[(v*width*bpp)+(du1*bpp)+2];
					nx2 = 0.299*data[(v*width*bpp)+(du2*bpp)+0]
						+ 0.587*data[(v*width*bpp)+(du2*bpp)+1]
						+ 0.114*data[(v*width*bpp)+(du2*bpp)+2];
					ny1 = 0.299*data[(dv1*width*bpp)+(u*bpp)+0]
						+ 0.587*data[(dv1*width*bpp)+(u*bpp)+1]
						+ 0.114*data[(dv1*width*bpp)+(u*bpp)+2];
					ny2 = 0.299*data[(dv2*width*bpp)+(u*bpp)+0]
						+ 0.587*data[(dv2*width*bpp)+(u*bpp)+1]
						+ 0.114*data[(dv2*width*bpp)+(u*bpp)+2];
					nx = nx1 - nx2;
					ny = ny1 - ny2;
					nz = 1.0f;
		            
					scale = 1.0f / sqrt((nx*nx) + (ny*ny) + (nz*nz));

					texels[(v*width*4)+(u*4)+0] = (nx * scale * 0.5f) + 0.5f;
					texels[(v*width*4)+(u*4)+1] = (ny * scale * 0.5f) + 0.5f;
					texels[(v*width*4)+(u*4)+2] = (nz * scale * 0.5f) + 0.5f;
					texels[(v*width*4)+(u*4)+3] = 1.0f;
				}
			}
			x3ds_texture *tex = new x3ds_texture;
			glGenTextures(1, &tex->m_tex_id);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, tex->m_tex_id);
			GLint	tex_size = 1;
			while (tex_size < width)	{ tex_size <<= 1; }
			while (tex_size < height)	{ tex_size <<= 1; }
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16, tex_size, tex_size, 0, GL_RGBA, GL_FLOAT, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_FLOAT, texels);
			tex->m_scale_x = (GLfloat)width / (GLfloat)tex_size;
			tex->m_scale_y = (GLfloat)height / (GLfloat)tex_size;
			m_bump[infile] = tex;
			glActiveTexture(GL_TEXTURE0);

			delete im;
			delete [] texels;

			//if (glGetError() != GL_NO_ERROR)
			//{
			//	log_error("Load bump GL Error!\n");
			//}

		}
	}

	character* x3ds_definition::create_character_instance(character* parent, int id)
	{
		character* ch = new x3ds_instance(this, parent, id);
		instanciate_registered_class(ch);
		return ch;
	}

	void x3ds_definition::get_bounding_center(Lib3dsVector bmin, Lib3dsVector bmax, Lib3dsVector center, float* size)
	{
		lib3ds_file_bounding_box_of_objects(m_file, LIB3DS_TRUE, LIB3DS_FALSE, LIB3DS_FALSE, bmin, bmax);

		// bounding box dimensions
		float	sx, sy, sz;
		sx = bmax[0] - bmin[0];
		sy = bmax[1] - bmin[1];
		sz = bmax[2] - bmin[2];

		*size = fmax(sx, sy); 
		*size = fmax(*size, sz);

		center[0] = (bmin[0] + bmax[0]) / 2;
		center[1] = (bmin[1] + bmax[1]) / 2;
		center[2] = (bmin[2] + bmax[2]) / 2;
	}

	void x3ds_definition::ensure_camera()
	{
		if (m_file->cameras == NULL)
		{
			// Fabricate camera

			Lib3dsCamera* camera = lib3ds_camera_new("Perspective");

			float size;
			Lib3dsVector bmin, bmax;
			get_bounding_center(bmin, bmax, camera->target, &size);

			memcpy(camera->position, camera->target, sizeof(camera->position));
			camera->position[0] = bmax[0] + 0.75f * size;
			camera->position[1] = bmin[1] - 0.75f * size;
			camera->position[2] = bmax[2] + 0.75f * size;

			// hack
			Lib3dsMatrix m;
			lib3ds_matrix_identity(m);
			lib3ds_matrix_rotate_z(m, 1.6f);	// PI/2
			lib3ds_matrix_rotate_y(m, 0.3f);
			
			lib3ds_vector_transform(camera->position, m, camera->position);

			camera->near_range = (camera->position[0] - bmax[0] ) * 0.5f;
			camera->far_range = (camera->position[0] - bmin[0] ) * 3.0f;
			lib3ds_file_insert_camera(m_file, camera);
		}
	}


	void x3ds_definition::ensure_lights()
	{
		// No lights in the file?  Add one. 
		if (m_file->lights == NULL)
		{
			Lib3dsLight* light = lib3ds_light_new("light0");

			float size;
			Lib3dsVector bmin, bmax;
			get_bounding_center(bmin, bmax, light->position, &size);

			light->spot_light = 0;
			light->see_cone = 0;
			light->color[0] = light->color[1] = light->color[2] = .6f;
			light->position[0] += size * 0.75f;
			light->position[1] -= size * 1.f;
			light->position[2] += size * 1.5f;
			light->position[3] = 0.f;
			light->outer_range = 100;
			light->inner_range = 10;
			light->multiplier = 1;
			lib3ds_file_insert_light(m_file, light);
		}
	}

	void x3ds_definition::ensure_nodes()
	{
		// No nodes?  Fabricate nodes to display all the meshes.
		if (m_file->nodes == NULL)
		{
			Lib3dsMesh *mesh;
			Lib3dsNode *node;
			for (mesh = m_file->meshes; mesh != NULL; mesh = mesh->next)
			{
				node = lib3ds_node_new_object();
				strcpy(node->name, mesh->name);
				node->parent_id = LIB3DS_NO_PARENT;
				node->data.object.scl_track.keyL = lib3ds_lin3_key_new();
				node->data.object.scl_track.keyL->value[0] = 1.;
				node->data.object.scl_track.keyL->value[1] = 1.;
				node->data.object.scl_track.keyL->value[2] = 1.;
				lib3ds_file_insert_node(m_file, node);
			}
		}
	}

	void x3ds_definition::remove_camera(Lib3dsCamera* camera)
	{
		lib3ds_file_remove_camera(m_file, camera);
	}

} // end of namespace gameswf

#endif
