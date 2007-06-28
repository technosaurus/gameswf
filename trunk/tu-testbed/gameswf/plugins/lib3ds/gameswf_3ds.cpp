// gameswf_3ds.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Lib3ds plugin implementation for gameswf library

// TODO: the using movieclip as texture for 3D model

#include "base/tu_config.h"

#if TU_CONFIG_LINK_TO_LIB3DS == 1

#include <lib3ds/file.h>
#include <lib3ds/camera.h>
#include <lib3ds/mesh.h>
#include <lib3ds/node.h>
#include <lib3ds/material.h>
#include <lib3ds/matrix.h>
#include <lib3ds/vector.h>
#include <lib3ds/light.h>

#include "gameswf_3ds.h"

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

		lib3ds_file_bounding_box(m_file, m_bmin, m_bmax);
		m_sx = m_bmax[0] - m_bmin[0];
		m_sy = m_bmax[1] - m_bmin[1];
		m_sz = m_bmax[2] - m_bmin[2];

		m_size = fmax(m_sx, m_sy); 
		m_size = fmax(m_size, m_sz);

		// used in create_camera()
		m_cx = (m_bmin[0] + m_bmax[0]) / 2;
		m_cy = (m_bmin[1] + m_bmax[1]) / 2;
		m_cz = (m_bmin[2] + m_bmax[2]) / 2;

		lib3ds_file_eval(m_file, 0.);
	}

	x3ds_definition::~x3ds_definition()
	{
		if (m_file)
		{
			lib3ds_file_free(m_file);
		}
	}

	character* x3ds_definition::create_character_instance(character* parent, int id)
	{
		character* ch = new x3ds_instance(this, parent, id);
		return ch;
	}

	Lib3dsCamera* x3ds_definition::create_camera()
	{
		Lib3dsCamera* camera = NULL;
		if (m_file->cameras == NULL)
		{
			// Add camera
			camera = lib3ds_camera_new("Camera_ISO");
			camera->target[0] = m_cx;
			camera->target[1] = m_cy;
			camera->target[2] = m_cz;
			memcpy(camera->position, camera->target, sizeof(camera->position));
			camera->position[0] = m_bmax[0] + 0.75f * m_size;
			camera->position[1] = m_bmin[1] - 0.75f * m_size;
			camera->position[2] = m_bmax[2] + 0.75f * m_size;
			camera->near_range = ( camera->position[0] - m_bmax[0] ) * 0.5f;
			camera->far_range = ( camera->position[0] - m_bmin[0] ) * 3.0f;
			lib3ds_file_insert_camera(m_file, camera);
		}
		else
		{
			// take the first camera
			camera = m_file->cameras;
		}

		// No lights in the file?  Add one. 
		if (m_file->lights == NULL)
		{
			Lib3dsLight* light = lib3ds_light_new("light0");
			light->spot_light = 0;
			light->see_cone = 0;
			light->color[0] = light->color[1] = light->color[2] = .6f;
			light->position[0] = m_cx + m_size * .75f;
			light->position[1] = m_cy - m_size * 1.f;
			light->position[2] = m_cz + m_size * 1.5f;
			light->position[3] = 0.;
			light->outer_range = 100;
			light->inner_range = 10;
			light->multiplier = 1;
			lib3ds_file_insert_light(m_file, light);
		}

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

		return camera;
	}

	void x3ds_definition::remove_camera(Lib3dsCamera* camera)
	{
		lib3ds_file_remove_camera(m_file, camera);
	}


	//
	// x3ds_instance
	//

	x3ds_instance::x3ds_instance(x3ds_definition* def, character* parent, int id)	:
		character(parent, id),
		m_def(def),
		m_current_frame(0.0f)
	{
		m_mesh_list  = glGenLists(1);
		assert(m_mesh_list);

		assert(m_def != NULL);
		m_camera = m_def->create_camera();

		glEnable(GL_TEXTURE_2D);
		glGenTextures(1, (GLuint*)&m_texture_id);
		glBindTexture(GL_TEXTURE_2D, m_texture_id);
	
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);	// GL_NEAREST ?
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		glDisable(GL_TEXTURE_2D);

		lib3ds_matrix_identity(m_matrix);

		// aply gameswf matrix
/*		matrix m = get_world_matrix();
		float	mat[16];
		memset(&mat[0], 0, sizeof(mat));
		mat[0] = m.m_[0][0];
		mat[1] = m.m_[1][0];
		mat[4] = m.m_[0][1];
		mat[5] = m.m_[1][1];
		mat[10] = 1;
		float w = 1024 * 20;	// hack, twips
		float h = 768 * 20;	// hack, twips
		mat[12] = -1.0f + 2.0f * m.m_[0][2] / w - m_def->m_sx;
		mat[13] = 1.0f  - 2.0f * m.m_[1][2] / h - m_def->m_sy;
		mat[15] = 1;
	//	glMultMatrixf(mat);*/
		
	}

	x3ds_instance::~x3ds_instance()
	{
		glDeleteLists(m_mesh_list, 1);
		glDeleteTextures(1, (GLuint*) &m_texture_id);
		m_def->remove_camera(m_camera);
	}

	void	x3ds_instance::advance(float delta_time)
	{
//		lib3ds_matrix_rotate_x(m_matrix, 0.01f);
//		lib3ds_matrix_rotate_y(m_matrix, 0.01f);
//		lib3ds_matrix_rotate_z(m_matrix, 0.01f);

		m_current_frame = fmod(m_current_frame + 1.0f, (float) m_def->m_file->frames);
		lib3ds_file_eval(m_def->m_file, m_current_frame);
	}

	bool	x3ds_instance::get_member(const tu_stringi& name, as_value* val)
	{
		// TODO handle character properties like _x, _y, _z, _zoom, ...
		log_error("error: x3ds_instance::get_member('%s') is't implemented yet\n", name.c_str());
		return false;
	}

	bool	x3ds_instance::set_member(const tu_stringi& name, const as_value& val)
	{
		// TODO handle character properties like _x, _y, _z, _zoom, ...
		log_error("error: x3ds_instance::set_member('%s', '%s') is't implemented yet\n", name.c_str(), val.to_string());
		return false;
	}

	void	x3ds_instance::apply_matrix(float* target, float* camera_pos)
	{
		Lib3dsVector v;
		lib3ds_vector_sub(v, target, camera_pos);
		float dist = lib3ds_vector_length(v);

		glTranslatef(0., dist, 0.);
		glMultMatrixf(&m_matrix[0][0]);
		glTranslatef(0., -dist, 0.);
	}

	void	x3ds_instance::display()
	{
		assert(m_def != NULL);

		if (m_def->m_file == NULL)
		{
			return;
		}

		// save GL state
		glPushAttrib (GL_ALL_ATTRIB_BITS);	
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();

		glClear(GL_DEPTH_BUFFER_BIT);

		// set 3D params
		glShadeModel(GL_SMOOTH);
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);
		glDisable(GL_LIGHT1);
		glDepthFunc(GL_LEQUAL);
		glEnable(GL_DEPTH_TEST);
		glCullFace(GL_BACK);

//		glEnable(GL_POLYGON_SMOOTH);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

		float top = tan(view_angle * 0.5f) * nnear;
		float bottom = -top;
		float aspect = 1.3333f;	// 4/3 == width /height
		float left = aspect* bottom;
		float right = aspect * top;

		//	gluPerspective(fov, aspect, nnear, ffar) ==> glFrustum(left, right, bottom, top, nnear, ffar);
		//	fov * 0.5 = arctan ((top-bottom)*0.5 / near)
		//	Since bottom == -top for the symmetrical projection that gluPerspective() produces, then:
		//	top = tan(fov * 0.5) * near
		//	bottom = -top
		//	Note: fov must be in radians for the above formulae to work with the C math library. 
		//	If you have comnputer your fov in degrees (as in the call to gluPerspective()), 
		//	then calculate top as follows:
		//	top = tan(fov*3.14159/360.0) * near
		//	The left and right parameters are simply functions of the top, bottom, and aspect:
		//	left = aspect * bottom
		//	right = aspect * top
		glFrustum(left, right, bottom, top, nnear, ffar);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glRotatef(-90., 1.0, 0., 0.);

		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, m_def->m_file->ambient);

		// apply gameswf matrix
		apply_matrix(target, camera_pos);

		// apply camera matrix
		Lib3dsMatrix cmatrix;
		lib3ds_matrix_camera(cmatrix, camera_pos, target, roll);
		glMultMatrixf(&cmatrix[0][0]);

		// draw 3D model
		for (Lib3dsNode* p = m_def->m_file->nodes; p != 0; p = p->next)
		{
			render_node(p);
		}

		// restore openGL state
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
		glPopAttrib();

	}

	bitmap_info* x3ds_instance::get_texture(const char* infile)
	{
		if (infile == NULL)
		{
			return NULL;
		}

		// is path relative ?
		tu_string url = get_workdir();
		if (strstr(infile, ":") || infile[0] == '/')
		{
			url = "";
		}
		url += infile;

		// is already loaded ?
		smart_ptr<bitmap_info> bi = NULL;
		if (m_texture.get(url, &bi))
		{
			return bi.get_ptr();
		}

		// load image & create texture
		image::rgb* im = image::read_jpeg(url.c_str());
		if (im)
		{
			bi = get_render_handler()->create_bitmap_info_rgb(im);
			delete im;
			bi->layout_image(bi->m_suspended_image);
			delete bi->m_suspended_image;
			bi->m_suspended_image = NULL;
			m_texture.add(url, bi);
			return bi.get_ptr();
		}

		static int n = 0;
		if (n < 1)
		{
			n++;
			log_error("can't load jpeg file %s\n", url.c_str());
		}
		return NULL;
	}

	void x3ds_instance::set_material(Lib3dsMaterial* mat)
	{
		if (mat)
		{
			float s;
			if (mat->two_sided)
			{
				glDisable(GL_CULL_FACE);
			}
			else
			{
				glEnable(GL_CULL_FACE);
			}

			glMaterialfv(GL_FRONT, GL_AMBIENT, mat->ambient);
			glMaterialfv(GL_FRONT, GL_DIFFUSE, mat->diffuse);
			glMaterialfv(GL_FRONT, GL_SPECULAR, mat->specular);

			s = pow(2.0f, 10.0f * mat->shininess);
			if (s > 128.0f)
			{
				s = 128.0f;
			}
			glMaterialf(GL_FRONT, GL_SHININESS, s);

			// try to bind texture
			bitmap_info* bi = get_texture(mat->texture1_map.name);
			if (bi)
			{
				glEnable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, bi->m_texture_id);
				glDisable(GL_TEXTURE_GEN_S);
				glDisable(GL_TEXTURE_GEN_T);
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
		}
	}

	void x3ds_instance::bind_material(Lib3dsMaterial* mat, float U, float V)
	{
		if (mat)
		{
			bitmap_info* bi = get_texture(mat->texture1_map.name);
			if (bi)
			{

				// get texture size
				int	tw = 1; while (tw < bi->m_original_width) { tw <<= 1; }
				int	th = 1; while (th < bi->m_original_height) { th <<= 1; }

				float scale_x = (float) bi->m_original_width / (float) tw;
				float scale_y = (float) bi->m_original_height / (float) th;
				float x = U * scale_x;
				float y = scale_y - V * scale_y;
				glTexCoord2f(x, y);
			}
		}
	}

	void x3ds_instance::create_mesh_list(Lib3dsMesh* mesh)
	{
		Lib3dsVector* normalL = (Lib3dsVector*) malloc(3 * sizeof(Lib3dsVector) * mesh->faces);
		{
			Lib3dsMatrix m;
			lib3ds_matrix_copy(m, mesh->matrix);
			lib3ds_matrix_inv(m);
			glMultMatrixf(&m[0][0]);
		}

		lib3ds_mesh_calculate_normals(mesh, normalL);

		Lib3dsMaterial* oldmat = NULL;
		for (Uint32 p = 0; p < mesh->faces; ++p)
		{
			Lib3dsFace* f = &mesh->faceL[p];
			Lib3dsMaterial* mat = f->material[0] ? 
				lib3ds_file_material_by_name(m_def->m_file, f->material) : NULL;
			set_material(mat);

			glBegin(GL_TRIANGLES);
			glNormal3fv(f->normal);
			for (int i = 0; i < 3; ++i)
			{
				glNormal3fv(normalL[3 * p + i]);
				bind_material(mat, mesh->texelL[f->points[i]][1], mesh->texelL[f->points[i]][0]);
				glVertex3fv(mesh->pointL[f->points[i]].pos);
			}
			glEnd();
			glDisable(GL_TEXTURE_2D);
		}

		free(normalL);
	}

	void x3ds_instance::render_node(Lib3dsNode* node)
	{
		for (Lib3dsNode* p = node->childs; p != 0; p = p->next)
		{
			render_node(p);
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
			
			// build mesh list
			glNewList(m_mesh_list, GL_COMPILE);
			create_mesh_list(mesh);
			glEndList();

			// exec mesh list
			glPushMatrix();
			Lib3dsObjectData* d = &node->data.object;
			glMultMatrixf(&node->matrix[0][0]);
			glTranslatef( - d->pivot[0], - d->pivot[1], - d->pivot[2]);
			glCallList(m_mesh_list);
			glPopMatrix();
		}
	}

} // end of namespace gameswf

#endif
