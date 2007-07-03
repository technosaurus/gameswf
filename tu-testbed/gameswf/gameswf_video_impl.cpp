// gameswf_video_impl.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// video implementation

#include "gameswf/gameswf_video_impl.h"
#include "gameswf/gameswf_video_base.h"
#include "gameswf/gameswf_stream.h"
#include "gameswf/gameswf_as_classes/as_netstream.h"

namespace gameswf
{
	void video_stream_definition::get_bound(rect& bound)
	{
		bound.m_x_min = 0;
		bound.m_y_min = 0;
		bound.m_x_max = PIXELS_TO_TWIPS(m_width);
		bound.m_y_max = PIXELS_TO_TWIPS(m_height);
	}

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
		else if (tag == 61)
		{
			Uint16 n = in->read_u16();
			m_frames[n] = NULL;
		}
	
	}
	
	character* video_stream_definition::create_character_instance(character* parent, int id)
	{
		character* ch = new video_stream_instance(this, parent, id);
		return ch;
	}

	void attach_video(const fn_call& fn)
	{
		video_stream_instance* video = fn.this_ptr->cast_to_video_stream_instance();
		assert(video);

		if (fn.nargs != 1)
		{
			log_error("attachVideo needs 1 arg\n");
			return;
		}

		// fn.arg(0) may be null
		video->attach_netstream((as_netstream*) fn.arg(0).to_object());
	}

	video_stream_instance::video_stream_instance(video_stream_definition* def, character* parent, int id)
	:
		character(parent, id),
		m_def(def)
	{
		assert(m_def != NULL);
	}

	video_stream_instance::~video_stream_instance()
	{
	}

	void video_stream_instance::display()
	{
		if (m_ns != NULL)	// is attached video ?
		{
			YUV_video* video_frame = m_ns->get_video();
			if (video_frame != NULL)
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
				video_frame->display(&m, &bounds, color);

				// printf("video time %d\n", SDL_GetTicks() - t);
			}
		}
	}

	bool video_stream_instance::get_member(const tu_stringi& name, as_value* val)
	{
		// first try standart members
		if (character::get_member(name, val))
		{
			return true;
		}

		if (name == "attachVideo")
		{
			val->set_as_c_function_ptr(attach_video);
			return true;
		}
		return false;
	}


} // end of namespace gameswf

