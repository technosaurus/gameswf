// quickie 2D visualization of Perspective Shadow Map sampling density


#include "SDL.h"
#include <stdlib.h>
#include <string.h>
#include "engine/ogl.h"
#include "engine/utility.h"
#include "engine/container.h"
#include "engine/geometry.h"



void	draw_circle(float x, float y, float radius)
// Draw a circle at the given spot w/ given radius.
{
	int	divisions = (int) floorf(radius);
	if (divisions < 4)
	{
		divisions = 4;
	}

	glColor3f(1, 1, 1);
	glBegin(GL_LINE_STRIP);
	for (int i = 0; i < divisions; i++)
	{
		float	angle = 2.0f * float(M_PI) * (i / float(divisions));
		glVertex3f(x + cosf(angle) * radius, y + sinf(angle) * radius, 1);
	}
	glVertex3f(x + radius, y, 1);
	glEnd();
}


void	draw_square(float x0, float y0, float x1, float y1, float xc, float yc, float threshold)
{

	// Draw this square.
	glColor3f(1, 1, 0);
	glBegin(GL_LINE_STRIP);
	glVertex3f(x0, y0, 1);
	glVertex3f(x1, y0, 1);
	glVertex3f(x1, y1, 1);
	glVertex3f(x0, y1, 1);
	glVertex3f(x0, y0, 1);
	glEnd();

	if (x1 - x0 > 10)
	{
		// Decide if we should split.

		float	sx = (x0 + x1) / 2;
		float	sy = (y0 + y1) / 2;

		float	dx = sx - xc;
		float	dy = sy - yc;
		float	dist = sqrtf(dx * dx + dy * dy);

		if (dist < threshold)
		{
			// Subdivide.
			float	half_x = (x1 - x0) / 2;
			float	half_y = (y1 - y0) / 2;
			for (int j = 0; j < 2; j++)
			{
				for (int i = 0; i < 2; i++)
				{
					draw_square(x0 + i * half_x, y0 + j * half_y,	
						    x0 + i * half_x + half_x, y0 + j * half_y + half_y,
						    xc, yc,
						    threshold / 2);
				}
			}

			return;
		}
		else
		{
			// Draw a little circle at our center.
			draw_circle(sx, sy, threshold / 16.0f);
		}
	}
}


void	draw_segment(const vec3& a, const vec3& b)
// Draw a line segment between the two points.
{
	glBegin(GL_LINE_STRIP);
	glVertex2f(a.x, a.y);
	glVertex2f(b.x, b.y);
	glEnd();
}


static float	s_viewer_y = -250;
static float	s_view_width = 10;
static float	s_near_plane_distance = 10;
static float	s_far_plane_distance = 500;
static vec3	s_light_direction(1, 0, 0);


void	draw_frustum()
// Draw a schematic top-view of a view frustum.
{
	glColor3f(1, 1, 1);
	glBegin(GL_LINE_STRIP);
	glVertex3f(-s_view_width / 2, s_viewer_y + s_near_plane_distance, 1);
	glVertex3f( s_view_width / 2, s_viewer_y + s_near_plane_distance, 1);
	glVertex3f( s_view_width / 2 * s_far_plane_distance / s_near_plane_distance, s_viewer_y + s_far_plane_distance, 1);
	glVertex3f(-s_view_width / 2 * s_far_plane_distance / s_near_plane_distance, s_viewer_y + s_far_plane_distance, 1);
	glVertex3f(-s_view_width / 2, s_viewer_y + s_near_plane_distance, 1);
	glEnd();
}


vec3	to_persp(const vec3& v)
// Transform v from normal space into post-perspective space.
{
	float	vy = v.y - s_viewer_y;
	float	one_over_vy;
	if (fabs(vy) < 0.000001f)
	{
		one_over_vy = 1000000.0f;	// "big"
	}
	else
	{
		one_over_vy = 1.0f / vy;
	}

	return vec3(v.x * one_over_vy, one_over_vy, 0);
}


vec3	from_persp(const vec3& v)
// Transform v from post-perspective space to normal space.
{
	float	vy = v.y;
	float	one_over_vy;
	if (fabs(vy) < 0.000001f)
	{
		one_over_vy = 1000000.0f;	// "big"
	}
	else
	{
		one_over_vy = 1.0f / vy;
	}

	return vec3(v.x * one_over_vy, one_over_vy + s_viewer_y, 0);
}


void	draw_light_rays(float density)
// Show post-perspective light rays in ordinary space.
{
	// Show the light direction.
	glColor3f(1, 1, 0);
	draw_segment(vec3::zero, s_light_direction * 100.0f);

	vec3	right(s_light_direction.y, -s_light_direction.x, 0);

	// Show light "right".
	draw_segment(vec3::zero, right * 50.0f);

	// Draw a bunch of rays.

	

// 	int	ray_count = fclamp(100 * density, 1, 1000);
// 	for (int i = 0; i < ray_count; i++)
// 	{
// 		float	f = 0.5f;
// 		if (ray_count > 1)
// 		{
// 			f = float(i) / (ray_count - 1);
// 		}

// 		vec3	start = light_persp + right_persp * f * something;

// 		for (...)
// 		{
// 			vec3	point_persp = start + light_dir_persp * t;
// 			vec3	point = from_persp(point_persp);

// 			// plot point
// 		}
// 	}
}


void	draw_stuff(float mouse_x, float mouse_y, float density)
{
	// Compute light direction.
	s_light_direction = vec3(mouse_x, mouse_y, 0);
	s_light_direction.normalize();

	// Draw a line at the "near clip plane".
	glColor3f(0, 0, 1);
	glBegin(GL_LINE_STRIP);
	glVertex2f(-1000, s_viewer_y);
	glVertex2f(1000, s_viewer_y);
	glEnd();

	draw_light_rays(density);
	draw_frustum();
}



#undef main	// SDL wackiness
int	main(int argc, char *argv[])
{
	// Initialize the SDL subsystems we're using.
	if (SDL_Init(SDL_INIT_VIDEO /* | SDL_INIT_JOYSTICK | SDL_INIT_CDROM | SDL_INIT_AUDIO*/))
	{
		fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
		exit(1);
	}
	atexit(SDL_Quit);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	int	width = 1000;
	int	height = 1000;

	// Set the video mode.
	if (SDL_SetVideoMode(width, height, 16 /* 32 */, SDL_OPENGL) == 0)
	{
		fprintf(stderr, "SDL_SetVideoMode() failed.");
		exit(1);
	}

	ogl::open();

	// Turn on alpha blending.
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glMatrixMode(GL_PROJECTION);
	glOrtho(0, 1000, 0, 1000, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(500, 500, 0);

	float	density = 1.0f;

	// Mouse state.
	int	mouse_x = 0;
	int	mouse_y = 0;
	int	mouse_buttons = 0;

	bool	paused = false;
	float	speed_scale = 1.0f;
	Uint32	last_ticks = SDL_GetTicks();
	for (;;)
	{
		Uint32	ticks = SDL_GetTicks();
		int	delta_ticks = ticks - last_ticks;
		float	delta_t = delta_ticks / 1000.f;
		last_ticks = ticks;

		if (paused == true)
		{
			delta_t = 0.0f;
		}

		// Handle input.
		SDL_Event	event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
			case SDL_KEYDOWN:
			{
				int	key = event.key.keysym.sym;

				if (key == SDLK_q || key == SDLK_ESCAPE)
				{
					exit(0);
				}
				else if (key == SDLK_k)
				{
					density = fclamp(density + 0.1f, 0.1f, 3.0f);
					printf("density = %f\n", density);
				}
				else if (key == SDLK_j)
				{
					density = fclamp(density - 0.1f, 0.1f, 3.0f);
					printf("density = %f\n", density);
				}
				break;
			}

			case SDL_MOUSEMOTION:
				mouse_x = (int) (event.motion.x);
				mouse_y = (int) (event.motion.y);
				break;

			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
			{
				int	mask = 1 << (event.button.button);
				if (event.button.state == SDL_PRESSED)
				{
					mouse_buttons |= mask;
				}
				else
				{
					mouse_buttons &= ~mask;
				}
				break;
			}

			case SDL_QUIT:
				exit(0);
				break;

			default:
				break;
			}
		}

		glDisable(GL_DEPTH_TEST);	// Disable depth testing.
		glDrawBuffer(GL_BACK);
		glClear(GL_COLOR_BUFFER_BIT);

		draw_stuff(float(mouse_x) - 500, 500.0f - float(mouse_y), density);

		SDL_GL_SwapBuffers();

		SDL_Delay(10);
	}

	return 0;
}


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
