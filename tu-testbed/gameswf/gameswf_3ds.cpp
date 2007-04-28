// gameswf_3ds.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// lib3ds interface

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
	static const Lib3dsRgba a={0.2, 0.2, 0.2, 1.0};
	static const Lib3dsRgba d={0.8, 0.8, 0.8, 1.0};
	static const Lib3dsRgba s={0.0, 0.0, 0.0, 1.0};

	x3ds_definition::x3ds_definition(const char* url) : m_file(NULL)
	{
		m_file = lib3ds_file_load(url);
		if (m_file == NULL)
		{
			log_error("can't load '%s'\n", url);
			return;
		}

		lib3ds_file_bounding_box(m_file, m_bmin, m_bmax);
		m_sx = m_bmax[0] - m_bmin[0];
		m_sy = m_bmax[1] - m_bmin[1];
		m_sz = m_bmax[2] - m_bmin[2];

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


	void	x3ds_definition::display(character* ch)
	{

		if (m_file == NULL)
		{
			return;
		}

		// save GL state
		glPushAttrib (GL_ALL_ATTRIB_BITS);	
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();

		// set GL matrix to identity
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		// aply gameswf matrix
		matrix m = ch->get_world_matrix();

		float	mat[16];
		memset(&mat[0], 0, sizeof(mat));
		mat[0] = m.m_[0][0];
		mat[1] = m.m_[1][0];
		mat[4] = m.m_[0][1];
		mat[5] = m.m_[1][1];
		mat[10] = 1;
		float w = 1024 * 20;	// hack, twips
		float h = 768 * 20;	// hack, twips
		mat[12] = -1.0f + 2.0f * m.m_[0][2] / w - m_sx;
		mat[13] = 1.0f  - 2.0f * m.m_[1][2] / h - m_sy;
		mat[15] = 1;

		glMultMatrixf(mat);

		glRotatef(-90., 1.0, 0., 0.);


		//	  glClearColor(0, 0, 0, 1.0);
		glShadeModel(GL_SMOOTH);
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);
		glDisable(GL_LIGHT1);
		//  	glDepthFunc(GL_LEQUAL);
		//	  glEnable(GL_DEPTH_TEST);
		//  	glEnable(GL_CULL_FACE);
		//	  glCullFace(GL_BACK);
		//  	glDisable(GL_NORMALIZE);
		//	  glPolygonOffset(1.0, 2);

		glEnable(GL_POLYGON_SMOOTH);
		glClear(GL_DEPTH_BUFFER_BIT);
		//		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, m_file->ambient);

		// apply camera matrix
		x3ds_instance* inst = (x3ds_instance*) ch;
		Lib3dsMatrix mc;
		lib3ds_matrix_camera(mc, inst->m_camera->position, inst->m_camera->target, inst->m_camera->roll);
		glMultMatrixf(&mc[0][0]);

		// draw 3D object
		for (Lib3dsNode* p = m_file->nodes; p != 0; p = p->next)
		{
			render_node(p);
		}

		// restote state
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();

		glPopAttrib();
	}

	void x3ds_definition::render_node(Lib3dsNode* node)
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

			Lib3dsMesh* mesh = lib3ds_file_mesh_by_name(m_file, node->data.object.morph);
			if (mesh == NULL)
			{
				mesh = lib3ds_file_mesh_by_name(m_file, node->name);
			}
			assert(mesh);

			if (mesh->user.d == 0)
			{
				mesh->user.d = glGenLists(1);
				glNewList(mesh->user.d, GL_COMPILE);
				{
					Lib3dsVector* normalL = (Lib3dsVector*) malloc(3 * sizeof(Lib3dsVector) * mesh->faces);
					Lib3dsMaterial* oldmat = (Lib3dsMaterial*) -1;
					{
						Lib3dsMatrix M;
						lib3ds_matrix_copy(M, mesh->matrix);
						lib3ds_matrix_inv(M);
						glMultMatrixf(&M[0][0]);
					}
					lib3ds_mesh_calculate_normals(mesh, normalL);

					for (unsigned p = 0; p < mesh->faces; ++p)
					{
						Lib3dsFace* f = &mesh->faceL[p];
						Lib3dsMaterial* mat = 0;

						int tex_mode = 0;

						if (f->material[0]) 
						{
							mat = lib3ds_file_material_by_name(m_file, f->material);
						}

						if (mat != oldmat)
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

								s = pow(2, 10.0 * mat->shininess);
								if (s > 128.0)
								{
									s=128.0;
								}
								glMaterialf(GL_FRONT, GL_SHININESS, s);
							}
							else
							{
								glMaterialfv(GL_FRONT, GL_AMBIENT, a);
								glMaterialfv(GL_FRONT, GL_DIFFUSE, d);
								glMaterialfv(GL_FRONT, GL_SPECULAR, s);
							}
							oldmat = mat;
						}

						glBegin(GL_TRIANGLES);
						glNormal3fv(f->normal);
						for (int i = 0; i < 3; ++i)
						{
							glNormal3fv(normalL[3*p+i]);
							glVertex3fv(mesh->pointL[f->points[i]].pos);
						}
						glEnd();
					}
					free(normalL);
				}
				glEndList();
			}

			if (mesh->user.d)
			{
				glPushMatrix();
				Lib3dsObjectData* d = &node->data.object;
				glMultMatrixf(&node->matrix[0][0]);
				glTranslatef( - d->pivot[0], - d->pivot[1], - d->pivot[2]);
				glCallList(mesh->user.d);
				glPopMatrix();
			}
		}
	}

	character* x3ds_definition::create_character_instance(character* parent, int id)
	{
		character* ch = new x3ds_instance(this, parent, id);
		return ch;
	}

	Lib3dsCamera* x3ds_definition::create_camera()
	{
		if (m_file->cameras == NULL)
		{
			// Add camera
			Lib3dsCamera *camera = lib3ds_camera_new("Camera_X");
			camera->target[0] = m_cx;
			camera->target[1] = m_cy;
			camera->target[2] = m_cz;
			memcpy(camera->position, camera->target, sizeof(camera->position));
			camera->position[0] = m_bmax[0] + 1.5 * fmax(m_sy, m_sz);
			camera->near_range = ( camera->position[0] - m_bmax[0] ) * .5;
			camera->far_range = ( camera->position[0] - m_bmin[0] ) * 2;
			lib3ds_file_insert_camera(m_file, camera);
			return camera;
		}

		assert(0);
		return NULL;

	}

	//
	// x3ds_instance
	//

	x3ds_instance::x3ds_instance(x3ds_definition* def, character* parent, int id)	:
		character(parent, id),
		m_def(def),
		m_current_frame(0.0f)
	{
		assert(m_def != NULL);
		m_camera = m_def->create_camera();
	}

	x3ds_instance::~x3ds_instance()
	{
		if (m_camera)
		{
//			lib3ds_camera_free(m_camera);
		}
	}

	void	x3ds_instance::display()
	{
		m_def->display(this);
	}

	void	x3ds_instance::advance(float delta_time)
	{
		m_current_frame += 1.0f;
		if (m_current_frame > m_def->m_file->frames)
		{
			m_current_frame = 0.0f;
		}
		lib3ds_file_eval(m_def->m_file, m_current_frame);
	}

	bool	x3ds_instance::get_member(const tu_stringi& name, as_value* val)
	{
		// first try standart properties
		if (character::get_member(name, val))
		{
			return true;
		}
		return false;
	}

	bool	x3ds_instance::set_member(const tu_stringi& name, const as_value& val)
	{
		// first try standart properties
		if (character::set_member(name, val))
		{
			return true;
		}

		if (name == "test")
		{
			m_camera->roll += 0.1;
			return true;
		}

		log_error("error: x3ds_instance::set_member('%s', '%s') not implemented\n", name.c_str(), val.to_string());
		return false;

	}

} // end of namespace gameswf

#endif
