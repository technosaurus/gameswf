// gameswf_abc.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// ActionScript 3.0 virtual machine implementaion


#include "gameswf/gameswf_abc.h"
#include "gameswf/gameswf_stream.h"

namespace gameswf
{

	//
	// read constant pool
	//
	//	cpool_info
	//	{
	//		u30 int_count
	//		s32 integer[int_count]
	//		u30 uint_count
	//		u32 uinteger[uint_count]
	//		u30 double_count
	//		d64 double[double_count]
	//		u30 string_count
	//		string_info string[string_count]
	//		u30 namespace_count
	//		namespace_info namespace[namespace_count]
	//		u30 ns_set_count
	//		ns_set_info ns_set[ns_set_count]
	//		u30 multiname_count
	//		multiname_info multiname[multiname_count]
	//	}
	void constant_pool::read(stream* in)
	{
		int n;

		// integer pool
		n = in->read_vu30();
		if (n > 0)
		{
			m_integer.resize(n);
			m_integer[0] = 0;	// default value
			for (int i = 1; i < n; i++)
			{
				m_integer[i] = in->read_vs32();
				printf("integer: %d\n", m_integer[i]);
			}
		}

		// uinteger pool
		n = in->read_vu30();
		if (n > 0)
		{
			m_uinteger.resize(n);
			m_uinteger[0] = 0;	// default value
			for (int i = 1; i < n; i++)
			{
				m_uinteger[i] = in->read_vu32();
				printf("uinteger: %d\n", m_uinteger[i]);
			}
		}

		// double pool
		n = in->read_vu30();
		if (n > 0)
		{
			m_double.resize(n);
			m_double[0] = 0;	// default value
			for (int i = 1; i < n; i++)
			{
				m_double[i] = in->read_double();
				printf("double: %f\n", m_double[i]);
			}
		}

		// string pool
		n = in->read_vu30();
		if (n > 0)
		{
			m_string.resize(n);
			m_string[0] = "";	// default value
			for (int i = 1; i < n; i++)
			{
				int len = in->read_vs32();
				in->read_string_with_length(len, &m_string[i]);
				printf("string: '%s'\n", m_string[i].c_str());
			}
		}
		
		return;

		// namespace pool
		n = in->read_vu30();
		if (n > 0)
		{
			m_string.resize(n);
			m_string[0] = "";	// default value
			for (int i = 1; i < n; i++)
			{
				int len = in->read_vs32();
				in->read_string_with_length(len, &m_string[i]);
				printf("%s\n", m_string[i].c_str());
			}
		}

	}

	abc_def::abc_def(player* player) :
		character_def(player)
	{
	}

	abc_def::~abc_def()
	{
	}

	//	abcFile
	//	{
	//		u16 minor_version
	//		u16 major_version
	//		cpool_info constant_pool
	//		u30 method_count
	//		method_info method[method_count]
	//		u30 metadata_count
	//		metadata_info metadata[metadata_count]
	//		u30 class_count
	//		instance_info instance[class_count]
	//		class_info class[class_count]
	//		u30 script_count
	//		script_info script[script_count]
	//		u30 method_body_count
	//		method_body_info method_body[method_body_count]
	//	}

	//TODO
	void	abc_def::read(stream* in, movie_definition_sub* m)
	{

		Uint16 minor_version = in->read_u16();
		Uint16 major_version = in->read_u16();
		assert(minor_version == 16 && major_version == 46);

		// read constant pool
		m_cpool.read(in);


//		assert(false && "todo");
	}

};	// end namespace gameswf


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
