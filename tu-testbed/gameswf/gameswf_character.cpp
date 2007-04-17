// gameswf_character.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some implementation code for the gameswf SWF player library.


#include "gameswf_character.h"

namespace gameswf
{

	character::character(character* parent, int id)
		:
		m_id(id),
		m_parent(parent),
		m_depth(-1),
		m_ratio(0.0f),
		m_clip_depth(0),
		m_visible(true),
		m_display_callback(NULL),
		m_display_callback_user_ptr(NULL)
	{
		// loadMovieClip() requires that the following will be commented out
		// assert((parent == NULL && m_id == -1)	|| (parent != NULL && m_id >= 0));
	}

	bool	character::get_member(const tu_stringi& name, as_value* val)
		// Set *val to the value of the named member and
		// return true, if we have the named member.
		// Otherwise leave *val alone and return false.
	{
		as_standard_member	std_member = get_standard_member(name);
		switch (std_member)
		{
		default:
		case M_INVALID_MEMBER:
			break;
		case M_X:
			//if (name == "_x")
			{
				matrix	m = get_matrix();
				val->set_double(TWIPS_TO_PIXELS(m.m_[0][2]));
				return true;
			}
		case M_Y:
			//else if (name == "_y")
			{
				matrix	m = get_matrix();
				val->set_double(TWIPS_TO_PIXELS(m.m_[1][2]));
				return true;
			}
		case M_XSCALE:
			//else if (name == "_xscale")
			{
				matrix m = get_matrix();	// @@ or get_world_matrix()?  Test this.
				float xscale = m.get_x_scale();
				val->set_double(xscale * 100);		// result in percent
				return true;
			}
		case M_YSCALE:
			//else if (name == "_yscale")
			{
				matrix m = get_matrix();	// @@ or get_world_matrix()?  Test this.
				float yscale = m.get_y_scale();
				val->set_double(yscale * 100);		// result in percent
				return true;
			}
		case M_CURRENTFRAME:
			//else if (name == "_currentframe")
			{
				int n = get_current_frame();
				if (n >= 0)
				{
					val->set_int(n + 1);
				}
				else
				{
					val->set_undefined();
				}
				return true;
			}
		case M_TOTALFRAMES:
			//else if (name == "_totalframes")
			{
				// number of frames.  Read only.
				int n = get_frame_count();
				if (n >= 0)
				{
					val->set_int(n);
				}
				else
				{
					val->set_undefined();
				}
				return true;
			}
		case M_ALPHA:
			//else if (name == "_alpha")
			{
				// Alpha units are in percent.
				val->set_double(get_cxform().m_[3][0] * 100.f);
				return true;
			}
		case M_VISIBLE:
			//else if (name == "_visible")
			{
				val->set_bool(get_visible());
				return true;
			}
		case M_WIDTH:
			//else if (name == "_width")
			{
				matrix	m = get_world_matrix();
				rect	transformed_rect;

				// @@ not sure about this...
				rect	source_rect;
				source_rect.m_x_min = 0;
				source_rect.m_y_min = 0;
				source_rect.m_x_max = (float) get_width();
				source_rect.m_y_max = (float) get_height();

				transformed_rect.enclose_transformed_rect(get_world_matrix(), source_rect);
				val->set_double(TWIPS_TO_PIXELS(transformed_rect.width()));
				return true;
			}
		case M_HEIGHT:
			//else if (name == "_height")
			{
				rect	transformed_rect;

				// @@ not sure about this...
				rect	source_rect;
				source_rect.m_x_min = 0;
				source_rect.m_y_min = 0;
				source_rect.m_x_max = (float) get_width();
				source_rect.m_y_max = (float) get_height();

				transformed_rect.enclose_transformed_rect(get_world_matrix(), source_rect);
				val->set_double(TWIPS_TO_PIXELS(transformed_rect.height()));
				return true;
			}
		case M_ROTATION:
			//else if (name == "_rotation")
			{
				// Verified against Macromedia player using samples/test_rotation.swf
				float	angle = get_matrix().get_rotation();

				// Result is CLOCKWISE DEGREES, [-180,180]
				angle *= 180.0f / float(M_PI);

				val->set_double(angle);
				return true;
			}
		case M_TARGET:
			//else if (name == "_target")
			{
				// Full path to this object; e.g. "/_level0/sprite1/sprite2/ourSprite"
				character* parent = get_parent();
				if (parent == NULL)	// root
				{
					val->set_tu_string("/");
					return true;
				}

				as_value target;
				parent->get_member("_target", &target);

				// if s != "/"(root) add "/"
				tu_string s = target.to_tu_string();
				s += s == "/" ? "" : "/";

				// add own name
				if (get_name().length() == 0)
				{
					s += "noname";
				}
				else
				{
					s += get_name();
				}

				val->set_tu_string(s);
				return true;
			}
		case M_FRAMESLOADED:
			//else if (name == "_framesloaded")
			{
				int n = get_frame_count();
				if (n >= 0)
				{
					val->set_int(n);
				}
				else
				{
					val->set_undefined();
				}
				return true;
			}
		case M_NAME:
			//else if (name == "_name")
			{
				val->set_tu_string(get_name());
				return true;
			}
		case M_DROPTARGET:
			//else if (name == "_droptarget")
			{
				// Absolute path in slash syntax where we were last dropped (?)
				// @@ TODO
				val->set_string("/_root");
				return true;
			}
		case M_URL:
			//else if (name == "_url")
			{
				// our URL.
				val->set_string("gameswf");
				return true;
			}
		case M_HIGHQUALITY:
			//else if (name == "_highquality")
			{
				// Whether we're in high quality mode or not.
				val->set_bool(true);
				return true;
			}
		case M_FOCUSRECT:
			//else if (name == "_focusrect")
			{
				// Is a yellow rectangle visible around a focused movie clip (?)
				val->set_bool(false);
				return true;
			}
		case M_SOUNDBUFTIME:
			//else if (name == "_soundbuftime")
			{
				// Number of seconds before sound starts to stream.
				val->set_double(0.0);
				return true;
			}
		case M_XMOUSE:
			//else if (name == "_xmouse")
			{
				// Local coord of mouse IN PIXELS.
				int	x, y, buttons;
				get_mouse_state(&x, &y, &buttons);

				matrix	m = get_world_matrix();

				point	a(PIXELS_TO_TWIPS(x), PIXELS_TO_TWIPS(y));
				point	b;

				m.transform_by_inverse(&b, a);

				val->set_double(TWIPS_TO_PIXELS(b.m_x));
				return true;
			}
		case M_YMOUSE:
			//else if (name == "_ymouse")
			{
				// Local coord of mouse IN PIXELS.
				int	x, y, buttons;
				get_mouse_state(&x, &y, &buttons);

				matrix	m = get_world_matrix();

				point	a(PIXELS_TO_TWIPS(x), PIXELS_TO_TWIPS(y));
				point	b;

				m.transform_by_inverse(&b, a);

				val->set_double(TWIPS_TO_PIXELS(b.m_y));
				return true;
			}
		case M_PARENT:
			{
				val->set_as_object_interface(static_cast<as_object_interface*>(m_parent));
				return true;
			}
		}	// end switch

		return false;
	}

	bool	character::set_member(const tu_stringi& name, const as_value& val)
	{
		as_standard_member	std_member = get_standard_member(name);
		switch (std_member)
		{
		default:
		case M_INVALID_MEMBER:
			break;
		case M_X:
			//if (name == "_x")
			{
				matrix	m = get_matrix();
				m.m_[0][2] = (float) PIXELS_TO_TWIPS(val.to_number());
				set_matrix(m);
				return true;
			}
		case M_Y:
			//else if (name == "_y")
			{
				matrix	m = get_matrix();
				m.m_[1][2] = (float) PIXELS_TO_TWIPS(val.to_number());
				set_matrix(m);
				return true;
			}
		case M_XSCALE:
			//else if (name == "_xscale")
			{
				matrix	m = get_matrix();

				// Decompose matrix and insert the desired value.
				float	x_scale = (float) val.to_number() / 100.f;	// input is in percent
				float	y_scale = m.get_y_scale();
				float	rotation = m.get_rotation();
				m.set_scale_rotation(x_scale, y_scale, rotation);

				set_matrix(m);
				return true;
			}
		case M_YSCALE:
			//else if (name == "_yscale")
			{
				matrix	m = get_matrix();

				// Decompose matrix and insert the desired value.
				float	x_scale = m.get_x_scale();
				float	y_scale = (float) val.to_number() / 100.f;	// input is in percent
				float	rotation = m.get_rotation();
				m.set_scale_rotation(x_scale, y_scale, rotation);

				set_matrix(m);
				return true;
			}
		case M_ALPHA:
			//else if (name == "_alpha")
			{
				// Set alpha modulate, in percent.
				cxform	cx = get_cxform();
				cx.m_[3][0] = float(val.to_number()) / 100.f;
				set_cxform(cx);
				return true;
			}
		case M_VISIBLE:
			//else if (name == "_visible")
			{
				set_visible(val.to_bool());
				return true;
			}
		case M_WIDTH:
			//else if (name == "_width")
			{
				// @@ tulrich: is parameter in world-coords or local-coords?
				matrix	m = get_matrix();
				m.m_[0][0] = float(PIXELS_TO_TWIPS(val.to_number()));
				float w = get_width();
				if (fabsf(w) > 1e-6f)
				{
					m.m_[0][0] /= w;
				}
				set_matrix(m);
				return true;
			}
		case M_HEIGHT:
			//else if (name == "_height")
			{
				// @@ tulrich: is parameter in world-coords or local-coords?
				matrix	m = get_matrix();
				m.m_[1][1] = float(PIXELS_TO_TWIPS(val.to_number()));
				float h = get_height();
				if (fabsf(h) > 1e-6f)
				{
					m.m_[1][1] /= h;
				}
				set_matrix(m);
				return true;
			}
		case M_ROTATION:
			//else if (name == "_rotation")
			{
				matrix	m = get_matrix();

				// Decompose matrix and insert the desired value.
				float	x_scale = m.get_x_scale();
				float	y_scale = m.get_y_scale();
				float	rotation = (float) val.to_number() * float(M_PI) / 180.f;	// input is in degrees
				m.set_scale_rotation(x_scale, y_scale, rotation);

				set_matrix(m);
				return true;
			}
		case M_NAME:
			//else if (name == "_name")
			{
				set_name(val.to_string());
				return true;
			}
		case M_HIGHQUALITY:
			//else if (name == "_highquality")
			{
				// @@ global { 0, 1, 2 }
				//				// Whether we're in high quality mode or not.
				//				val->set(true);
				return true;
			}
		case M_FOCUSRECT:
			//else if (name == "_focusrect")
			{
				//				// Is a yellow rectangle visible around a focused movie clip (?)
				//				val->set(false);
				return true;
			}
		case M_SOUNDBUFTIME:
			//else if (name == "_soundbuftime")
			{
				// @@ global
				//				// Number of seconds before sound starts to stream.
				//				val->set(0.0);
				return true;
			}
		}	// end switch

		return false;
	}


}
