// quickie 2D visualization of Perspective Shadow Map sampling density


#include "SDL.h"
#include <stdlib.h>
#include <string.h>
#include "engine/ogl.h"
#include "engine/utility.h"
#include "engine/container.h"
#include "engine/geometry.h"


struct controls
{
	// Mouse position.
	float	m_mouse_x, m_mouse_y;

	// Mouse buttons.
	bool	m_mouse_left;
	bool	m_mouse_right;
	bool	m_mouse_left_click;
	bool	m_mouse_right_click;

	// Keys
	bool	m_alt;
	bool	m_ctl;
	bool	m_shift;

	controls()
		:
		m_mouse_x(0),
		m_mouse_y(0),
		m_mouse_left(false),
		m_mouse_right(false),
		m_mouse_left_click(false),
		m_mouse_right_click(false),
		m_alt(false),
		m_ctl(false),
		m_shift(false)
	{
	}
};


void	draw_circle(float x, float y, float radius)
// Draw a circle at the given spot w/ given radius.
{
	int	divisions = (int) floorf(radius);
	if (divisions < 4)
	{
		divisions = 4;
	}

	glBegin(GL_LINE_STRIP);
	for (int i = 0; i < divisions; i++)
	{
		float	angle = 2.0f * float(M_PI) * (i / float(divisions));
		glVertex3f(x + cosf(angle) * radius, y + sinf(angle) * radius, 1);
	}
	glVertex3f(x + radius, y, 1);
	glEnd();
}


void	draw_filled_circle(float x, float y, float radius)
// Draw a filled circle at the given spot w/ given radius.
{
	int	divisions = (int) floorf(radius);
	if (divisions < 4)
	{
		divisions = 4;
	}

	glBegin(GL_POLYGON);
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


struct occluder
// Basically a circular object, to stand in for a shadow occluder.
{
	vec3	m_position;
	float	m_radius;

	occluder()
		:
		m_position(vec3::zero),
		m_radius(1.0f)
	{
	}

	occluder(const vec3& pos, float radius)
		:
		m_position(pos),
		m_radius(radius)
	{
	}

	void	draw()
	{
		glColor3f(0, 0, 1);
		draw_filled_circle(m_position.x, m_position.y, m_radius);
	}

	bool	hit(const vec3& v)
	{
		return (v - m_position).magnitude() <= m_radius;
	}
};



static float	s_viewer_y = -250;
static float	s_view_tan = 1.0f;
static float	s_near_plane_distance = 10;
static float	s_far_plane_distance = 500;
static vec3	s_light_direction(1, 0, 0);
static array<occluder>	s_occluders;


vec3	frustum_point(int i)
// Returns the i'th frustum vertex (i in [0,3]).
// Goes ccw around the boundary.
{
	assert(i >= 0 && i < 4);

	float	d = i < 2 ? s_near_plane_distance : s_far_plane_distance;
	float	w = d * s_view_tan / 2;
	if (i == 0 || i == 3)
	{
		w = -w;
	}

	return vec3(w, s_viewer_y + d, 0);
}


void	draw_frustum()
// Draw a schematic top-view of a view frustum.
{
	glColor3f(1, 1, 1);
	glBegin(GL_LINE_STRIP);
// 	glVertex3f(-s_near_plane_distance * s_view_tan / 2, s_viewer_y + s_near_plane_distance, 1);
// 	glVertex3f( s_near_plane_distance * s_view_tan / 2, s_viewer_y + s_near_plane_distance, 1);
// 	glVertex3f( s_far_plane_distance * s_view_tan / 2, s_viewer_y + s_far_plane_distance, 1);
// 	glVertex3f(-s_far_plane_distance * s_view_tan / 2, s_viewer_y + s_far_plane_distance, 1);
// 	glVertex3f(-s_near_plane_distance * s_view_tan / 2, s_viewer_y + s_near_plane_distance, 1);
	glVertex3fv(frustum_point(0));
	glVertex3fv(frustum_point(1));
	glVertex3fv(frustum_point(2));
	glVertex3fv(frustum_point(3));
	glVertex3fv(frustum_point(0));
	glEnd();
}


vec3	frustum_max_projection(const vec3& axis)
// Return the point in the frustum whose projection onto the given
// axis is maximum.
{
	float	max = -1000000.f;
	vec3	max_point(vec3::zero);
	for (int i = 0; i < 4; i++)
	{
		vec3	v = frustum_point(i);
		float	proj = v * axis;
		if (proj > max)
		{
			max = proj;
			max_point = v;
		}
	}

	return max_point;
}


void	draw_occluders()
// Draw all occluders.
{
	for (int i = 0; i < s_occluders.size(); i++)
	{
		s_occluders[i].draw();
	}
}


vec3	occluders_max_projection(const vec3& axis)
// Returns the point on all occluders whose projection onto the given
// axis is maximum.
{
	float	max = -1000000.f;
	vec3	max_point(vec3::zero);
	for (int i = 0; i < s_occluders.size(); i++)
	{
		vec3	v = s_occluders[i].m_position;
		float	proj = v * axis + s_occluders[i].m_radius;
		if (proj > max)
		{
			max = proj;
			max_point = v + axis * s_occluders[i].m_radius;
		}
	}

	return max_point;
}


vec3	to_persp(const vec3& v)
// Transform v from normal space into perspective space.
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
// Transform v from perspective space to normal space.
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
	// Direction perpendicular to the light.
	vec3	light_right(s_light_direction.y, -s_light_direction.x, 0);

	// Find the boundary of our light ray projections.
	vec3	extreme_0 = occluders_max_projection(light_right);
	vec3	extreme_1 = occluders_max_projection(-light_right);

	vec3	extreme_0_prime = to_persp(extreme_0);
	vec3	extreme_1_prime = to_persp(extreme_1);


	// Draw a bunch of rays.  Light-buffer ray sampling is linear
	// in *perspective* space, which means it'll be something else
	// in world-space.
 	int	ray_count = fclamp(30 * density, 0, 1000);
	ray_count = ray_count * 2 + 1;	// make it odd
 	for (int i = 0; i < ray_count; i++)
 	{
		float	f = 0.0f;
		if (ray_count > 1)
		{
			f = (float(i) / (ray_count - 1));
		}

		vec3	start = from_persp(extreme_0_prime * (1-f) + extreme_1_prime * f);

		vec3	v0 = start - s_light_direction * 100000.0f;
		vec3	v1 = start + s_light_direction * 100000.0f;
		glColor3f(1, 1, 0);
		draw_segment(v0, v1);
	}
}


void	draw_stuff(const controls& c, float density)
{
	float	mx = c.m_mouse_x - 500.0f;
	float	my = 500.0f - c.m_mouse_y;
	vec3	mouse_pos(mx, my, 0);

	if (c.m_mouse_right)
	{
		// Re-compute light direction.
		s_light_direction = mouse_pos;
		s_light_direction.normalize();
	}

	if (c.m_mouse_left_click)
	{
		// Add or delete an occluder.

		// If we're on an occluder, then delete it.
		bool	deleted = false;
		for (int i = 0; i < s_occluders.size(); i++)
		{
			if (s_occluders[i].hit(mouse_pos))
			{
				// Remove this guy.
				s_occluders[i] = s_occluders.back();
				s_occluders.resize(s_occluders.size() - 1);
				deleted = true;
				break;
			}
		}

		if (!deleted)
		{
			// If we didn't delete, then the user want to
			// add an occluder.
			s_occluders.push_back(occluder(mouse_pos, 20));
		}
	}

	draw_light_rays(density);
	draw_occluders();

	// Draw a line at the "near clip plane".
	glColor3f(0, 0, 1);
	draw_segment(vec3(-1000, s_viewer_y, 0), vec3(1000, s_viewer_y, 0));

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


	// Add a couple occluders.
	s_occluders.push_back(occluder(vec3(0,0,0), 20));
	s_occluders.push_back(occluder(vec3(100,50,0), 20));
	s_occluders.push_back(occluder(vec3(-100,50,0), 20));

	// Mouse state.
	controls	c;
//	int	mouse_x = 0;
//	int	mouse_y = 0;
//	int	mouse_buttons = 0;

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
		c.m_mouse_left_click = false;
		c.m_mouse_right_click = false;
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
				else if (key == SDLK_LCTRL || key == SDLK_RCTRL)
				{
					c.m_ctl = true;
				}
				else if (key == SDLK_LSHIFT || key == SDLK_RSHIFT)
				{
					c.m_shift = true;
				}
				else if (key == SDLK_LALT || key == SDLK_RALT)
				{
					c.m_alt = true;
				}
				break;
			}

			case SDL_KEYUP:
			{
				int	key = event.key.keysym.sym;

				if (key == SDLK_LCTRL || key == SDLK_RCTRL)
				{
					c.m_ctl = false;
				}
				else if (key == SDLK_LSHIFT || key == SDLK_RSHIFT)
				{
					c.m_shift = false;
				}
				else if (key == SDLK_LALT || key == SDLK_RALT)
				{
					c.m_alt = false;
				}
				break;
			}

			case SDL_MOUSEMOTION:
				c.m_mouse_x = float((int) (event.motion.x));
				c.m_mouse_y = float((int) (event.motion.y));
				break;

			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
			{
				int	mask = 1 << (event.button.button);
				bool	down = (event.button.state == SDL_PRESSED);
				bool	clicked = (event.type == SDL_MOUSEBUTTONDOWN);
				if (event.button.button == 1)
				{
					c.m_mouse_left = down;
					c.m_mouse_left_click = clicked;
				}
				if (event.button.button == 3)
				{
					c.m_mouse_right = down;
					c.m_mouse_right_click = clicked;
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

		draw_stuff(c, density);

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
