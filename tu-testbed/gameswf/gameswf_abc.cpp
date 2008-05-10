// gameswf_abc.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// ActionScript 3.0 virtual machine implementaion


#include "gameswf/gameswf_abc.h"

namespace gameswf
{
	abc_def::abc_def(player* player) :
		character_def(player)
	{
	}

	abc_def::~abc_def()
	{
	}

	void	abc_def::read(stream* in, movie_definition_sub* m)
	{
		//TODO
		assert(false && "todo");
	}

};	// end namespace gameswf


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
