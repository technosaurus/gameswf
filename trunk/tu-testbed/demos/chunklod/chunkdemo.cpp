// chunkdemo.cpp	-- tu@tulrich.com Copyright 2001 by Thatcher Ulrich

// Demo program to show chunked LOD rendering.


#include <windows.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "chunklod.h"
#include "cull.h"
#include "utility.h"


static int	window_width = 1024;
static int	window_height = 768;
static float	horizontal_fov_degrees = 90.f;


vec3	viewer_dir(1, 0, 0);
vec3	viewer_up(0, 1, 0);
float	viewer_theta = 0;
float	viewer_phi = 0;
float	viewer_height = 0;
vec3	viewer_pos(512, 100, 512);


static plane_info	frustum_plane[6];
static plane_info	transformed_frustum_plane[6];


int	speed = 5;
//bool	pin_to_ground = false;
bool	move_forward = false;
bool	wireframe_mode = false;
render_options	render_opt;

float	max_pixel_error = 5.0f;

int	triangle_counter = 0;





void	apply_matrix(const matrix& m)
// Applies the given matrix to the current GL matrix.
{
	float	mat[16];

	// Copy to the 4x4 layout.
	for (int col = 0; col < 4; col++) {
		for (int row = 0; row < 3; row++) {
			mat[col * 4 + row] = m.GetColumn(col).get(row);
		}
		if (col < 3) {
			mat[col * 4 + 3] = 0;
		} else {
			mat[col * 4 + 3] = 1;
		}
	}

	// Apply to the current OpenGL matrix.
	glMultMatrixf(mat);
}


void	setup_projection_matrix(int w, int h, float horizontal_fov_degrees)
// Set up viewport & projection matrix.
// w and h are the window dimensions in pixels.
// horizontal_fov is in degrees.
{
	glViewport(0, 0, w, h);

	//
	// Set up projection matrix.
	//
	float	nearz = 4.0;
	float	farz = 80000;

	float	aspect_ratio = float(h) / float(w);
	float	horizontal_fov = horizontal_fov_degrees * M_PI / 180.f;
	float	vertical_fov = atan(tan(horizontal_fov / 2.f) * aspect_ratio) * 2;
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	float	m[16];
	int	i;
	for (i = 0; i < 16; i++) m[i] = 0;
	m[0] = -1.0 / tan(vertical_fov / 2.f);
	m[5] = -m[0] / aspect_ratio;
	m[10] = (farz + nearz) / (farz - nearz);
	m[11] = 1;
	m[14] = - 2 * farz * nearz / (farz - nearz);
	
	glMultMatrixf(m);

	glMatrixMode(GL_MODELVIEW);

//#define TEST_FRUSTUM 1	// define to shrink the frustum, to see culling at work...
#if TEST_FRUSTUM
	// Compute values for the frustum planes.  Normals point inwards towards the visible volume.
	frustum_plane[0].set(0, 0, 1, nearz * 10);	// near.
	frustum_plane[1].set(0, 0, -1, -farz * 0.1);	// far.
	frustum_plane[2].set(-cos(horizontal_fov/2), 0, sin(horizontal_fov/4), 0);	// left.
	frustum_plane[3].set(cos(horizontal_fov/2), 0, sin(horizontal_fov/4), 0);	// right.
	frustum_plane[4].set(0, -cos(vertical_fov/2), sin(vertical_fov/4), 0);	// top.
	frustum_plane[5].set(0, cos(vertical_fov/2), sin(vertical_fov/4), 0);	// bottom.
#else
	// Compute values for the frustum planes.  Normals point inwards towards the visible volume.
	frustum_plane[0].set(0, 0, 1, nearz);	// near.
	frustum_plane[1].set(0, 0, -1, -farz);	// far.
	frustum_plane[2].set(-cos(horizontal_fov/2), 0, sin(horizontal_fov/2), 0);	// left.
	frustum_plane[3].set(cos(horizontal_fov/2), 0, sin(horizontal_fov/2), 0);	// right.
	frustum_plane[4].set(0, -cos(vertical_fov/2), sin(vertical_fov/2), 0);	// top.
	frustum_plane[5].set(0, cos(vertical_fov/2), sin(vertical_fov/2), 0);	// bottom.
#endif
}


void	clear()
// Clear the drawing surface and initialize viewing parameters.
{
	glClearDepth(1.0f);                                                     // Depth Buffer Setup
	glEnable(GL_DEPTH_TEST);                                                // Enables Depth Testing
	glDepthFunc(GL_LEQUAL);
	glDrawBuffer(GL_BACK);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Set up the projection and view matrices.

	// Projection matrix.
	glMatrixMode(GL_PROJECTION);
	setup_projection_matrix(window_width, window_height, horizontal_fov_degrees);

	// View matrix.
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	matrix	view_matrix;
	view_matrix.View(viewer_dir, viewer_up, viewer_pos);
	apply_matrix(view_matrix);

	// Transform the frustum planes from view coords into world coords.
	int	i;
	for (i = 0; i < 6; i++) {
		// Rotate the plane from view coords into world coords.  I'm pretty sure this is not a slick
		// way to do this :)
		plane_info&	tp = transformed_frustum_plane[i];
		plane_info&	p = frustum_plane[i];
		view_matrix.ApplyInverseRotation(&tp.normal, p.normal);
		vec3	v;
		view_matrix.ApplyInverse(&v, p.normal * p.d);
		tp.d = v * tp.normal;
	}

	// Enable z-buffer.
	glDepthFunc(GL_LEQUAL);	// smaller z gets priority

	// Set the wireframe state.
	if (wireframe_mode) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	} else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	// Set up automatic texture-coordinate generation.
	// Basically we're just stretching the current texture
	// over the entire model.
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	float	p[4] = { 0, 0, 0, 0 };
	float	slope = 1.0 / 1024;
	float	intercept = 1.0;
	p[0] = slope;
	glTexGenfv(GL_S, GL_OBJECT_PLANE, p);
	p[0] = 0;

	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	p[2] = slope;
	glTexGenfv(GL_T, GL_OBJECT_PLANE, p);

	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
}


struct cull_info {
	bool	culled;	// true when the volume is not visible
	Uint8	active_planes;	// one bit per frustum plane

	cull_info(bool c = false, Uint8 a = 0x3f) : culled(c), active_planes(a) {}
};


//
// Usage description.
//

void	print_usage()
// Prints controls summary.
{
	printf(
		"chunkdemo, by Thatcher Ulrich <tu@tulrich.com> May-July 2001\n"
		"This program and associated source code is in the public domain.\n"
		"Commercial and non-commercial use encouraged.  I make no warrantees;\n"
		"use at your own risk.\n"
		"\n"
		"The 'crater' sample data courtesy John Ratcliff is not in the public\n"
		"domain!  See http://ratcliff.flipcode.com for details.\n"
		"\n"
		"Controls:\n"
		"  'q' - quit\n"
		"  'w' - toggle wireframe\n"
		"  'b' - toggle rendering of geometry & bounding boxes\n"
		"  'm' - toggle vertex morphing\n"
		"  'f' - toggle viewpoint forward motion\n"
		"  '[' - decrease max pixel error\n"
		"  ']' - increase max pixel error\n"
		"  ',' - decrease viewpoint speed\n"
		"  '.' - increase viewpoint speed\n"
		"\n"
		);
}



//
// Input handling.
//


void	mouse_motion_handler(float dx, float dy, int state)
// Handles mouse motion.
{
	bool	left_button = (state & 1) ? true : false;
	bool	right_button = (state & 4) ? true : false;

	float	f_speed = (1 << speed);

	if (left_button && right_button) {
		// Translate in the view plane.
		vec3	right = viewer_dir.cross(viewer_up);
		viewer_pos += right * dx / 3 * speed + viewer_up * -dy / 3 * speed;
		
	} else if (right_button) {
		// Rotate the viewer.
		viewer_theta += -dx / 100;
		while (viewer_theta < 0) viewer_theta += 2 * M_PI;
		while (viewer_theta >= 2 * M_PI) viewer_theta -= 2 * M_PI;

		viewer_phi += -dy / 100;
		const float	plimit = M_PI / 2;
		if (viewer_phi > plimit) viewer_phi = plimit;
		if (viewer_phi < -plimit) viewer_phi = -plimit;

		viewer_dir = vec3(1, 0, 0);
		viewer_up = vec3(0, 1, 0);
		
		viewer_dir = Geometry::Rotate(viewer_theta, viewer_up, viewer_dir);
		vec3	right = viewer_dir.cross(viewer_up);
		viewer_dir = Geometry::Rotate(viewer_phi, right, viewer_dir);
		viewer_up = Geometry::Rotate(viewer_phi, right, viewer_up);

	} else if (left_button) {
		// Translate the viewer.
		vec3	right = viewer_dir.cross(viewer_up);
		viewer_pos += right * dx / 3 * speed + viewer_dir * -dy / 3 * speed;
	}
}


void	process_events()
// Check and respond to any events in the event queue.
{
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_KEYDOWN: {
			char*	keyname = SDL_GetKeyName(event.key.keysym.sym);
//			printf("The %s key was pressed!\n", keyname);
			
			if (strcmp(keyname, "q") == 0) {
				// Quit on 'q'.
				exit(0);
			}
			if (strcmp(keyname, "w") == 0) {
				// Toggle wireframe mode.
				wireframe_mode = !wireframe_mode;
			}
			if (strcmp(keyname, "b") == 0) {
				// Cycle through rendering of "geo", "geo + boxes", "boxes"
				if ( render_opt.show_geometry ) {
					if ( render_opt.show_box ) {
						render_opt.show_geometry = false;
					} else {
						render_opt.show_box = true;
					}
				} else {
					render_opt.show_box = false;
					render_opt.show_geometry = true;
				}
			}
			if (strcmp(keyname, "m") == 0) {
				// Toggle rendering of vertex morphing.
				render_opt.morph = !render_opt.morph;
			}
			if (strcmp(keyname, "f") == 0) {
				// Toggle viewpoint motion.
				move_forward = ! move_forward;
			}
			if (strcmp(keyname, "[") == 0) {
				// Decrease max pixel error.
				max_pixel_error -= 0.5;
				if (max_pixel_error < 0.5) {
					max_pixel_error = 0.5;
				}
				printf("new max_pixel_error = %f\n", max_pixel_error);
			}
			if (strcmp(keyname, "]") == 0) {
				// Decrease max pixel error.
				max_pixel_error += 0.5;
				printf("new max_pixel_error = %f\n", max_pixel_error);
			}
			if (strcmp(keyname, ".") == 0) {
				// Speed up.
				speed++;
				if (speed > 10) speed = 10;
			}
			if (strcmp(keyname, ",") == 0) {
				// Slow down.
				speed--;
				if (speed < 1) speed = 1;
			}
			break;
		}

		case SDL_MOUSEMOTION: {
			mouse_motion_handler(event.motion.xrel, event.motion.yrel, event.motion.state);
			break;
		}

		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP: {
//			mouse_button_handler(event.button.button, event.button.state == SDL_PRESSED ? true : false);
			break;
		}

		case SDL_QUIT:
			exit(0);
		}
	}
}


#undef main
extern "C" int	main(int argc, char *argv[])
{
	// Print out program info.
	print_usage();

	// Initialize the SDL subsystems we're using.
	if (SDL_Init(SDL_INIT_VIDEO /* | SDL_INIT_JOYSTICK | SDL_INIT_CDROM | SDL_INIT_AUDIO*/))
	{
		fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
		exit(1);
	}
	atexit(SDL_Quit);

	const SDL_VideoInfo* info = NULL;
	info = SDL_GetVideoInfo();
	if (!info) {
		fprintf(stderr, "SDL_GetVideoInfo() failed.");
		exit(1);
	}

	int	bpp = info->vfmt->BitsPerPixel;
	int	flags = SDL_OPENGL | (0 ? SDL_FULLSCREEN : 0);

	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 5 );
	SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 5 );
	SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 5 );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

	// Set the video mode.
	if (SDL_SetVideoMode(window_width, window_height, bpp, flags) == 0) {
		fprintf(stderr, "SDL_SetVideoMode() failed.");
		exit(1);
	}

	// Load a texture.
	SDL_Surface*	texture = IMG_Load("crater/crater.jpg");
	if (texture) {
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 1);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texture->w, texture->h, 0, GL_RGB, GL_UNSIGNED_BYTE, texture->pixels);
		gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, texture->w, texture->h, GL_RGB, GL_UNSIGNED_BYTE, texture->pixels);

		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);	// GL_MODULATE

		SDL_FreeSurface(texture);

		glBindTexture(GL_TEXTURE_2D, 1);
		glEnable(GL_TEXTURE_2D);
	}

	// Load our chunked model.
	SDL_RWops*	in = SDL_RWFromFile("crater/crater.chu", "rb");
	if (in == NULL) {
		printf("Can't open 'crater/crater.chu'\n");
		exit(1);
	}
	lod_chunk_tree*	model = new lod_chunk_tree(in);

	// Main loop.
	Uint32	last_ticks = SDL_GetTicks();
	for (;;) {
		Uint32	ticks = SDL_GetTicks();
		float	delta_t = ( ticks - last_ticks ) / 1000.f;
		last_ticks = ticks;

		process_events();

		if ( move_forward ) {
			// Drift the viewpoint forward.
			viewer_pos += viewer_dir * delta_t * (1 << speed) * 0.50f;
		}

		model->set_parameters(max_pixel_error, window_width, horizontal_fov_degrees);
		model->update(viewer_pos);

		clear();

		model->render(transformed_frustum_plane, render_opt);

		SDL_GL_SwapBuffers();
	}

	return 0;
}

