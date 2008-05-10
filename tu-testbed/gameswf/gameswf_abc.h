// gameswf_abc.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// ActionScript 3.0 virtual machine implementaion

#ifndef GAMESWF_ABC_H
#define GAMESWF_ABC_H


#include "gameswf/gameswf_impl.h"


namespace gameswf
{
	struct movie_definition_sub;

	// AVM2 parser
	struct abc_def : public character_def
	{
		abc_def(player* player);
		virtual ~abc_def();
		void	read(stream* in, movie_definition_sub* m);

	};
}


#endif // GAMESWF_ABC_H
