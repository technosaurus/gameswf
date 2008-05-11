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

	struct multiname
	{
		enum kind
		{
			CONSTANT_QName = 0x07,
			CONSTANT_QNameA = 0x0D,
			CONSTANT_RTQName = 0x0F,
			CONSTANT_RTQNameA = 0x10,
			CONSTANT_RTQNameL = 0x11,
			CONSTANT_RTQNameLA = 0x12,
			CONSTANT_Multiname = 0x09,
			CONSTANT_MultinameA = 0x0E,
			CONSTANT_MultinameL = 0x1B,
			CONSTANT_MultinameLA = 0x1C
		};

	};

	struct namespac
	{
		enum kind
		{
			CONSTANT_Namespace = 0x08,
			CONSTANT_PackageNamespace = 0x16,
			CONSTANT_PackageInternalNs = 0x17,
			CONSTANT_ProtectedNamespace = 0x18,
			CONSTANT_ExplicitNamespace = 0x19,
			CONSTANT_StaticProtectedNs = 0x1A,
			CONSTANT_PrivateNs = 0x05
		};

		int m_kind;
		int	m_name;

		namespac() :
			m_kind(0),
			m_name(0)
		{
		}

		void set_kind(int kind)
		{
			assert(kind == 0x08 || kind == 0x16 || kind == 0x17 || kind == 0x18 ||
				kind == 0x19 || kind == 0x1A || kind == 0x05);
			m_kind = kind;
		}

	};

	struct constant_pool
	{
		array<int> m_integer;
		array<Uint32> m_uinteger;
		array<double> m_double;
		array<tu_string> m_string;
		array<namespac> m_namespace;
		array< array<int> > m_ns_set;
		array<multiname> m_multiname;

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
