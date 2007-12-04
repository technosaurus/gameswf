// gameswf_3ds.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Lib3ds plugin implementation for gameswf library

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
	void	as_map_material(const fn_call& fn)
	{
		assert(fn.this_ptr);
		x3ds_instance* x3ds = fn.this_ptr->cast_to_3ds();
		if (x3ds && fn.nargs == 2)
		{
			x3ds->m_map[fn.arg(0).to_tu_string()] = fn.arg(1);
		}
	}

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

		lib3ds_file_bounding_box_of_objects(m_file, LIB3DS_TRUE, LIB3DS_FALSE, LIB3DS_FALSE, m_bmin, m_bmax);
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

		// create empty bitmaps for movieclip's snapshots
	  for (Lib3dsMaterial* p = m_def->m_file->materials; p != 0; p = p->next)
		{
			if (p->texture1_map.name)
			{
				m_material[p->texture1_map.name] = new bitmap_info();
			}
	  }

		m_mesh_list  = glGenLists(1);
		assert(m_mesh_list);

		assert(m_def != NULL);
		m_camera = m_def->create_camera();
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
		// TODO handle 3D character properties like _x, _y, _z, _zoom, ...
		if (name == "mapMaterial")
		{
			val->set_as_c_function_ptr(as_map_material);
			return true;
		}
		return character::get_member(name, val);
	}

	bool	x3ds_instance::set_member(const tu_stringi& name, const as_value& val)
	{
		// TODO handle 3D character properties like _x, _y, _z, _zoom, ...
		return character::set_member(name, val);
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

	void	x3ds_instance::display()
	{
		assert(m_def != NULL);

		if (m_def->m_file == NULL)
		{
			return;
		}

		Uint32 t = SDL_GetTicks();

		//update_material();

		// save GL state
		glPushAttrib (GL_ALL_ATTRIB_BITS);	
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();

		glClear(GL_DEPTH_BUFFER_BIT);

		// set 3D params
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

		// apply light
		{
			glLightModelfv(GL_LIGHT_MODEL_AMBIENT, m_def->m_file->ambient);
			glShadeModel(GL_SMOOTH);
			int light = GL_LIGHT0;
			for (Lib3dsLight* p = m_def->m_file->lights; p != 0; p = p->next)
			{
				assert(light <= GL_LIGHT7);

				static GLfloat glfLightAmbient[] = {0.0, 0.0, 0.0, 1.0};
				static GLfloat glfLightDiffuse[] = {1.0, 1.0, 1.0, 0.0};
				static GLfloat glfLightSpecular[] = {1.0f, 1.0f, 1.0f, 1.0f};

		//		glLightf (light, GL_SPOT_EXPONENT, 128.0f);
		//		glLightfv(light, GL_AMBIENT, glfLightAmbient);
		//		glLightfv(light, GL_SPECULAR, glfLightSpecular);
		//		glLightfv(light, GL_DIFFUSE, p->color);
				glLightfv(light, GL_POSITION, p->position);
				glEnable(light);

				light++;
			}
			glEnable(GL_LIGHTING);
		}


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

		//printf("3ds display time: %d\n", SDL_GetTicks() - t);


	}

	void x3ds_instance::bind_texture(const char* infile)
	{
		glDisable(GL_TEXTURE_2D);

		if (infile == NULL)
		{
			return;
		}

		smart_ptr<bitmap_info> bi = NULL;
		m_material.get(infile, &bi);
		if (bi == NULL || bi->m_texture_id == 0)
		{
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
				bi = get_render_handler()->create_bitmap_info_rgb(im);
				delete im;
				bi->layout_image(bi->m_suspended_image);
				delete bi->m_suspended_image;
				bi->m_suspended_image = NULL;
				m_material[infile] = bi;
			}
			else
			{
				static int n = 0;
				if (n < 1)
				{
					n++;
					log_error("can't load jpeg file %s\n", url.c_str());
				}
				return;
			}
		}

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, bi->m_texture_id);
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	}

	void x3ds_instance::set_material(Lib3dsMaterial* mat)
	{
		if (mat)
		{
//			if (mat->two_sided)
//			{
//				glDisable(GL_CULL_FACE);
//			}
//			else
//			{
//				glEnable(GL_CULL_FACE);
//			}
			glDisable(GL_CULL_FACE);

			glMaterialfv(GL_FRONT, GL_AMBIENT, mat->ambient);
			glMaterialfv(GL_FRONT, GL_DIFFUSE, mat->diffuse);
			glMaterialfv(GL_FRONT, GL_SPECULAR, mat->specular);
			glMaterialf(GL_FRONT, GL_SHININESS, pow(2, 10.0 * mat->shininess));

			bind_texture(mat->texture1_map.name);
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
      glMaterialf(GL_FRONT, GL_SHININESS, pow(2, 10.0 * 0.5));
		}
	}

	void x3ds_instance::bind_material(Lib3dsMaterial* mat, float U, float V)
	{
		if (mat)
		{
			smart_ptr<bitmap_info> bi = NULL;
			if (m_material.get(mat->texture1_map.name, &bi))
			{
				// get texture size
				int	tw = 1; while (tw < bi->m_original_width) { tw <<= 1; }
				int	th = 1; while (th < bi->m_original_height) { th <<= 1; }

				float scale_x = (float) bi->m_original_width / tw;
				float scale_y = (float) bi->m_original_height / th;

				float s = scale_y * (1 - V);
				float t = U * scale_x;
				glTexCoord2f(s, t);
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
				if (mesh->texelL)
				{
					bind_material(mat, mesh->texelL[f->points[i]][1], mesh->texelL[f->points[i]][0]);
				}
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
