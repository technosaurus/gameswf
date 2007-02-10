// gameswf_video_impl.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// video implementation

#include "gameswf_video_impl.h"
#include "gameswf_video_base.h"
#include "gameswf_netstream.h"
#include "gameswf_stream.h"

namespace gameswf {

void video_stream_definition::read(stream* in, int tag, movie_definition* m)
{
	// Character ID has been read already 

	assert(tag == 60 ||	tag == 61);

	if (tag == 60)
	{

		Uint16 numframes = in->read_u16();
		m_frames.resize(numframes);

		m_width = in->read_u16();
		m_height = in->read_u16();
		Uint8 reserved_flags = in->read_uint(5);
		m_deblocking_flags = in->read_uint(2);
		m_smoothing_flags = in->read_uint(1) ? true : false;

		m_codec_id = in->read_u8();
	}
	else
	if (tag == 61)
	{
		Uint16 n = in->read_u16();
		m_frames[n] = NULL;
	}

}

character* video_stream_definition::create_character_instance(movie* parent, int id)
{
	character* ch = new video_stream_instance(this, parent, id);
	return ch;
}

	void attach_video(const fn_call& fn)
	{
		assert(dynamic_cast<video_stream_instance*>(fn.this_ptr));
		video_stream_instance* video = static_cast<video_stream_instance*>(fn.this_ptr);

		if (fn.nargs != 1)
		{
			log_error("attachVideo needs 1 arg\n");
			return;
		}

		//		if (dynamic_cast<netstream_as_object*>(fn.arg(0).to_object()))
		if (video)
		{
			video->m_ns = static_cast<netstream_as_object*>(fn.arg(0).to_object());
		}
	}

	video_stream_instance::video_stream_instance(video_stream_definition* def, movie* parent, int id)
		:
	character(parent, id),
		m_def(def),
		m_video_source(NULL),
		m_ns(NULL)
	{
		assert(m_def);
	}

	video_stream_instance::~video_stream_instance()
	{
	}

	void video_stream_instance::display()
	{
		if (m_ns)
		{
			netstream_as_object* nso = static_cast<netstream_as_object*>(m_ns);
			YUV_video* v = nso->obj.get_video();
			if (v)
			{
				if (v->video_in_place())
				{
					// Uint32 t = SDL_GetTicks();

					rect bounds;
					bounds.m_x_min = 0.0f;
					bounds.m_y_min = 0.0f;
					bounds.m_x_max = PIXELS_TO_TWIPS(m_def->m_width);
					bounds.m_y_max = PIXELS_TO_TWIPS(m_def->m_height);
	
					cxform cx = get_world_cxform();
					gameswf::rgba color = cx.transform(gameswf::rgba());

					matrix m = get_world_matrix();
					v->display(&m, &bounds, color);

					// printf("video time %d\n", SDL_GetTicks() - t);
				}
			}
		}
	}

	void video_stream_instance::advance(float delta_time)
	{
	}

	void	video_stream_instance::set_member(const tu_stringi& name, const as_value& val)
	{
		if (name == "_alpha")
		{
			// Set alpha modulate, in percent.
			cxform	cx = get_cxform();
			cx.m_[3][0] = float(val.to_number()) / 100.f;
 			set_cxform(cx);
			return;
		}
		if (name == "_visible")
		{
			m_visible = val.to_bool();
			return;
		}

		log_error("error: video_stream_instance::set_member('%s', '%s') not implemented yet\n",
			name.c_str(),
			val.to_string());
	}

	bool video_stream_instance::get_member(const tu_stringi& name, as_value* val)
	{
		if (name == "attachVideo")
		{
			val->set_as_c_function_ptr(attach_video);
			return true;
		}
		else
		if (name == "_alpha")
		{
			// Alpha units are in percent.
			val->set_double(get_cxform().m_[3][0] * 100.f);
			return true;
		}
		else if (name == "_visible")
		{
			val->set_bool(m_visible);
			return true;
		}
		return false;
	}

} // end of namespace gameswf

