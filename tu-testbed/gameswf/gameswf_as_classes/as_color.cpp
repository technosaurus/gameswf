// as_color.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "../gameswf_as_classes/as_color.h"

namespace gameswf
{
	void	as_global_color_ctor(const fn_call& fn)
	{
		if (fn.nargs == 1)
		{
			smart_ptr<as_color>	obj = new as_color;

			assert(fn.arg(0).get_type() == as_value::OBJECT);
			obj->m_target = fn.arg(0).to_object()->cast_to_character();
			assert(obj->m_target != NULL);
			fn.result->set_as_object_interface(obj.get_ptr());
		}
	}


	void	as_color_getRGB(const fn_call& fn)
	{
		as_color* obj = (as_color*) fn.this_ptr;
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
			as_color* obj = (as_color*) fn.this_ptr;
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

	as_color::as_color()
	{
		set_member("getRGB", &as_color_getRGB);
		set_member("setRGB", &as_color_setRGB);
//		set_member("getTransform", &as_array_tostring);
//		set_member("setTransform", &as_array_tostring);
	}
};
