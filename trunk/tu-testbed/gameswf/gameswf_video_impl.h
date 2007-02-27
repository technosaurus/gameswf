// gameswf_video_impl.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.


#ifndef GAMESWF_VIDEO_H
#define GAMESWF_VIDEO_H

#include "gameswf_impl.h"

namespace gameswf
{

	struct video_stream_definition : public character_def
	{

		//	video_stream_definition();
		//	virtual ~video_stream_definition();


		character* create_character_instance(movie* parent, int id);
		void	read(stream* in, int tag, movie_definition* m);
		const rect&	get_bound() const	{
			return m_unused_rect;
		}

		Uint16 m_width;
		Uint16 m_height;

	private:

		//	uint8_t reserved_flags;
		Uint8 m_deblocking_flags;
		bool m_smoothing_flags;

		// 0: extern file
		// 2: H.263
		// 3: screen video (Flash 7+ only)
		// 4: VP6
		Uint8 m_codec_id;
		array<void*>	m_frames;
		rect m_unused_rect;
	};

	class video_stream_instance : public character
	{

	public:

		//	const as_object* m_source;
		video_stream_definition*	m_def;

		// m_video_source - A Camera object that is capturing video data or a NetStream object.
		// To drop the connection to the Video object, pass null for source.
		as_object* m_video_source;

		video_stream_instance(video_stream_definition* def,	movie* parent, int id);

		~video_stream_instance();

//		virtual void	advance(float delta_time);
		void	display();

		//	void set_source(const as_object* source)
		//	{
		//		m_source = source;
		//	}

		//
		// ActionScript overrides
		//

		virtual void set_member(const tu_stringi& name, const as_value& val);
		virtual bool get_member(const tu_stringi& name, as_value* val);

		as_object* m_ns;

	};


}	// end namespace gameswf


#endif // GAMESWF_VIDEO_H
