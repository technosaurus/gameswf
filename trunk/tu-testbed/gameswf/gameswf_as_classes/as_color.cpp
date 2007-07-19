// as_color.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// The Color class lets you set the RGB color value and color transform
// of movie clips and retrieve those values once they have been set. 

#include "gameswf/gameswf_as_classes/as_color.h"

namespace gameswf
{
	void	as_global_color_ctor(const fn_call& fn)
	{
		if (fn.nargs == 1)
		{
			smart_ptr<as_color>	obj = new as_color;

			assert(fn.arg(0).get_type() == as_value::OBJECT);
			assert(fn.arg(0).to_object());
			obj->m_target = fn.arg(0).to_object()->cast_to_character();
			assert(obj->m_target != NULL);
			fn.result->set_as_object_interface(obj.get_ptr());
		}
	}


	void	as_color_getRGB(const fn_call& fn)
	{
		assert(fn.this_ptr);
		as_color* obj = fn.this_ptr->cast_to_as_color();
		assert(obj);
		assert(obj->m_target != NULL);

		cxform	cx = obj->m_target->get_cxform();
		Uint8 r = (Uint8) ceil(cx.m_[0][0] * 255.0f);
		Uint8 g = (Uint8) ceil(cx.m_[1][0] * 255.0f);
		Uint8 b = (Uint8) ceil(cx.m_[2][0] * 255.0f);
		fn.result->set_int(r << 16 | g << 8 | b);
	}

	void	as_color_setRGB(const fn_call& fn)
	{
		if (fn.nargs == 1)
		{
			assert(fn.this_ptr);
			as_color* obj = fn.this_ptr->cast_to_as_color();
			assert(obj);
			assert(obj->m_target != NULL);

			cxform	cx = obj->m_target->get_cxform();
			int rgb = (int) fn.arg(0).to_number();
			Uint8 r = (rgb >> 16) & 0xFF;
			Uint8 g = (rgb >> 8) & 0xFF;
			Uint8 b = rgb & 0xFF;
			cx.m_[0][0] = float(r) / 255.0f;
			cx.m_[1][0] = float(g) / 255.0f;
			cx.m_[2][0] = float(b) / 255.0f;
			obj->m_target->set_cxform(cx);
		}		
	}

	void	as_color_gettransform(const fn_call& fn)
	{
		assert(fn.this_ptr);
		as_color* obj = fn.this_ptr->cast_to_as_color();
		assert(obj);
		assert(obj->m_target != NULL);

		cxform	cx = obj->m_target->get_cxform();
		Uint8 r = (Uint8) ceil(cx.m_[0][0] * 255.0f);
		Uint8 g = (Uint8) ceil(cx.m_[1][0] * 255.0f);
		Uint8 b = (Uint8) ceil(cx.m_[2][0] * 255.0f);
		Uint8 a = (Uint8) ceil(cx.m_[3][0] * 255.0f);

		as_object* tobj = new as_object();
		tobj->set_member("ra", r / 255.0f * 100.0f);	// percent (-100..100)
		tobj->set_member("rb", r);	// value	(-255..255)
		tobj->set_member("ga", g / 255.0f * 100.0f);	// percent (-100..100)
		tobj->set_member("gb", g);	// value	(-255..255)
		tobj->set_member("ba", b / 255.0f * 100.0f);	// percent (-100..100)
		tobj->set_member("bb", b);	// value	(-255..255)
		tobj->set_member("aa", a / 255.0f * 100.0f);	// percent (-100..100)
		tobj->set_member("ab", a);	// value	(-255..255)
		fn.result->set_as_object_interface(tobj);
	}

	void	as_color_settransform(const fn_call& fn)
	{
		if (fn.nargs == 1)
		{
			assert(fn.this_ptr);
			as_color* obj = fn.this_ptr->cast_to_as_color();
			assert(obj);
			assert(obj->m_target != NULL);

			assert(fn.arg(0).to_object());
			as_object* tobj = fn.arg(0).to_object()->cast_to_as_object();
			if (tobj)
			{
				cxform	cx = obj->m_target->get_cxform();
				as_value v;

				if (tobj->get_member("ra", &v))
				{
					cx.m_[0][0] *= float(v.to_number()) / 100.0f;
				}
				else
				if (tobj->get_member("rb", &v))
				{
					cx.m_[0][0] = float(v.to_number()) / 255.0f;
				}

				if (tobj->get_member("ga", &v))
				{
					cx.m_[1][0] *= float(v.to_number()) / 100.0f;
				}
				else
				if (tobj->get_member("gb", &v))
				{
					cx.m_[1][0] = float(v.to_number()) / 255.0f;
				}

				if (tobj->get_member("ba", &v))
				{
					cx.m_[2][0] *= float(v.to_number()) / 100.0f;
				}
				else
				if (tobj->get_member("bb", &v))
				{
					cx.m_[2][0] = float(v.to_number()) / 255.0f;
				}

				if (tobj->get_member("aa", &v))
				{
					cx.m_[3][0] *= float(v.to_number()) / 100.0f;
				}
				else
				if (tobj->get_member("ab", &v))
				{
					cx.m_[3][0] = float(v.to_number()) / 255.0f;
				}

				obj->m_target->set_cxform(cx);
			}
		}		
	}

	as_color::as_color()
	{
		set_member("getRGB", &as_color_getRGB);
		set_member("setRGB", &as_color_setRGB);
		set_member("getTransform", &as_color_gettransform);
		set_member("setTransform", &as_color_settransform);
	}
};
