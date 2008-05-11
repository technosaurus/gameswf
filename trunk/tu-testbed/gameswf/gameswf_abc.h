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

	struct constant_pool
	{
		array<int> m_integer;
		array<Uint32> m_uinteger;
		array<double> m_double;
		array<tu_string> m_string;

		void	read(stream* in);

	};

	struct abc_def : public character_def
	{
		constant_pool m_cpool;

		abc_def(player* player);
		virtual ~abc_def();
		void	read(stream* in, movie_definition_sub* m);

	};
}


#endif // GAMESWF_ABC_H
