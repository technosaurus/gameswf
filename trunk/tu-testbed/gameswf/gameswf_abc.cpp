// gameswf_abc.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// do_abc tag reader/parser

#include "gameswf/gameswf_abc.h"
#include "gameswf/gameswf_stream.h"
#include "gameswf/gameswf_disasm.h"

namespace gameswf
{

	//	method_info
	//	{
	//		u30 param_count
	//		u30 return_type
	//		u30 param_type[param_count]
	//		u30 name
	//		u8 flags
	//		option_info options
	//		param_info param_names
	//	}
	void method_info::read(stream* in, abc_def* abc)
	{
		int param_count = in->read_vu30();
		
		// The return_type field is an index into the multiname
		m_return_type = in->read_vu30();


		m_param_type.resize(param_count);
		for (int i = 0; i < param_count; i++)
		{
			m_param_type[i] = in->read_vu30();
		}

		m_name = in->read_vu30();
		m_flags = in->read_u8();

		if (m_flags & HAS_OPTIONAL)
		{
			assert(0 && "todo");
//		option_info options
		}

		if (m_flags & HAS_PARAM_NAMES)
		{
			assert(0 && "todo");
//		param_info param_names
		}
	}

	void metadata_info::read(stream* in, abc_def* abc)
	{
		assert(0&&"todo");
	}

	//	traits_info
	//	{
	//		u30 name
	//		u8 kind
	//		u8 data[]
	//		u30 metadata_count
	//		u30 metadata[metadata_count]
	//	}
	void traits_info::read(stream* in, abc_def* abc)
	{
		// The value can not be zero, and the multiname entry specified must be a QName.
		m_name = in->read_vu30();
		assert(m_name != 0 && abc->m_multiname[m_name].is_qname());
		IF_VERBOSE_PARSE(log_msg("    traits: name='%s'\n",	abc->get_multiname(m_name)));
		
		Uint8 b = in->read_u8();
		m_kind = b & 0x0F;
		m_attr = b >> 4;

		switch (m_kind)
		{
			case Trait_Slot :
			case Trait_Const :

				trait_slot.m_slot_id = in->read_vu30();

				trait_slot.m_type_name = in->read_vu30();
				assert(trait_slot.m_type_name < abc->m_multiname.size());

				trait_slot.m_vindex = in->read_vu30();

				if (trait_slot.m_vindex != 0)
				{
					// This field exists only when vindex is non-zero.
					trait_slot.m_vkind = in->read_u8();
				}
				break;

			case Trait_Class :

				trait_class.m_slot_id = in->read_vu30();

				trait_class.m_classi = in->read_vu30();
				assert(trait_class.m_classi < abc->m_class.size());
				break;

			case Trait_Function :

				trait_function.m_slot_id = in->read_vu30();

				trait_function.m_function = in->read_vu30();
				assert(trait_function.m_function < abc->m_method.size());
				break;

			case Trait_Method :
			case Trait_Getter :
			case Trait_Setter :

				trait_method.m_disp_id = in->read_vu30();

				trait_method.m_method = in->read_vu30();
				assert(trait_method.m_method < abc->m_method.size());
				break;

			default:
				assert(false && "invalid kind of traits");
				break;
		}

		if (m_attr & ATTR_Metadata)
		{
			assert(0 && "test");
			int n = in->read_vu30();
			m_metadata.resize(n);
			for (int i = 0; i < n; i++)
			{
				m_metadata[i] = in->read_vu30();
			}
		}
	}

	//	instance_info
	//		{
	//		u30 name
	//		u30 super_name
	//		u8 flags
	//		u30 protectedNs
	//		u30 intrf_count
	//		u30 interface[intrf_count]
	//		u30 iinit
	//		u30 trait_count
	//		traits_info trait[trait_count]
	//	}
	void instance_info::read(stream* in, abc_def* abc)
	{
		m_name = in->read_vu30();
		m_super_name = in->read_vu30();

		m_flags = in->read_u8();
		if (m_flags & CONSTANT_ClassProtectedNs)
		{
			m_protectedNs = in->read_vu30();
		}

		int i, n;
		n = in->read_vu30();
		m_interface.resize(n);
		for (i = 0; i < n; i++)
		{
			m_interface[i] = in->read_vu30();
		}

		m_iinit = in->read_vu30();

		IF_VERBOSE_PARSE(log_msg("  name='%s', supername='%s', ns='%s'\n",
			abc->get_multiname(m_name), abc->get_multiname(m_super_name),
				abc->get_namespace(m_protectedNs)));

		n = in->read_vu30();
		m_trait.resize(n);
		for (i = 0; i < n; i++)
		{
			traits_info* trait = new traits_info();
			trait->read(in, abc);
			m_trait[i] = trait;
		}
	}

	void class_info::read(stream* in, abc_def* abc)
	{
		// This is an index into the method array of the abcFile
		m_cinit = in->read_vu30();
		assert(m_cinit < abc->m_method.size());

		int n = in->read_vu30();
		m_trait.resize(n);
		for (int i = 0; i < n; i++)
		{
			traits_info* trait = new traits_info();
			trait->read(in, abc);
			m_trait[i] = trait;
		}
	}

	void script_info::read(stream* in, abc_def* abc)
	{
		// The init field is an index into the method array of the abcFile
		m_init = in->read_vu30();
		assert(m_init < abc->m_method.size());

		int n = in->read_vu30();
		m_trait.resize(n);
		for (int i = 0; i < n; i++)
		{
			traits_info* trait = new traits_info();
			trait->read(in, abc);
			m_trait[i] = trait;
		}
	}

	// exception_info
	// {
	//		u30 from
	//		u30 to
	//		u30 target
	//		u30 exc_type
	//		u30 var_name
	// }
	void exceptiin_info::read(stream* in, abc_def* abc)
	{
		m_from = in->read_vu30();
		m_to = in->read_vu30();
		m_target = in->read_vu30();
		m_exc_type = in->read_vu30();
		m_var_name = in->read_vu30();
	}

	//	method_body_info
	//	{
	//		u30 method
	//		u30 max_stack
	//		u30 local_count
	//		u30 init_scope_depth
	//		u30 max_scope_depth
	//		u30 code_length
	//		u8 code[code_length]
	//		u30 exception_count
	//		exception_info exception[exception_count]
	//		u30 trait_count
	//		traits_info trait[trait_count]
	//	}
	void body_info::read(stream* in, abc_def* abc)
	{
		m_method = in->read_vu30();

		m_max_stack = in->read_vu30();
		m_local_count = in->read_vu30();
		m_init_scope_depth = in->read_vu30();
		m_max_scope_depth = in->read_vu30();

		int i, n;
		n = in->read_vu30();	// code_length
		m_code.resize(n);
		for (i = 0; i < n; i++)
		{
			m_code[i] = in->read_u8();
		}

		n = in->read_vu30();	// exception_count
		m_exception.resize(n);
		for (i = 0; i < n; i++)
		{
			exceptiin_info* e = new exceptiin_info();
			e->read(in, abc);
			m_exception[i] = e;
		}

		n = in->read_vu30();	// trait_count
		m_trait.resize(n);
		for (int i = 0; i < n; i++)
		{
			traits_info* trait = new traits_info();
			trait->read(in, abc);
			m_trait[i] = trait;
		}
	}


	abc_def::abc_def(player* player)
	{
	}

	abc_def::~abc_def()
	{
	}

	//	abcFile
	//	{
	//		u16 minor_version
	//		u16 major_version
	//		cpool_info cpool_info
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
	void	abc_def::read(stream* in)
	{
		int eof = in->get_tag_end_position();
		int i, n;

		Uint16 minor_version = in->read_u16();
		Uint16 major_version = in->read_u16();
		assert(minor_version == 16 && major_version == 46);

		// read constant pool
		read_cpool(in);
		assert(in->get_position() < eof);

		// read method_info
		n = in->read_vu30();
		m_method.resize(n);
		IF_VERBOSE_PARSE(log_msg("method_info count: %d\n", n));
		for (i = 0; i < n; i++)
		{
			method_info* info = new method_info();
			info->read(in, this);
			m_method[i] = info;
			IF_VERBOSE_PARSE(log_msg("method_info[%d]: name='%s', type='%s', params=%d\n",
				i, get_string(info->m_name), get_multiname(info->m_return_type),
				info->m_param_type.size()));
		}

		assert(in->get_position() < eof);

		// read metadata_info
		n = in->read_vu30();
		m_metadata.resize(n);
		IF_VERBOSE_PARSE(log_msg("metadata_info count: %d\n", n));
		for (i = 0; i < n; i++)
		{
			assert(0 && "test");
			metadata_info* info = new metadata_info();
			info->read(in, this);
			m_metadata[i] = info;
		}

		assert(in->get_position() < eof);

		// read instance_info & class_info
		n = in->read_vu30();
		m_instance.resize(n);
		IF_VERBOSE_PARSE(log_msg("instance_info count: %d\n", n));
		for (i = 0; i < n; i++)
		{
			IF_VERBOSE_PARSE(log_msg("instance_info[%d]:\n", i));

			instance_info* info = new instance_info();
			info->read(in, this);
			m_instance[i] = info;
		}

		assert(in->get_position() < eof);

		// class_info
		m_class.resize(n);
		IF_VERBOSE_PARSE(log_msg("class_info count: %d\n", n));
		for (i = 0; i < n; i++)
		{
			IF_VERBOSE_PARSE(log_msg("class_info[%d]\n", i));
			class_info* info = new class_info();
			info->read(in, this);
			m_class[i] = info;
		}

		assert(in->get_position() < eof);

		// read script_info
		n = in->read_vu30();
		m_script.resize(n);
		IF_VERBOSE_PARSE(log_msg("script_info count: %d\n", n));
		for (i = 0; i < n; i++)
		{
			IF_VERBOSE_PARSE(log_msg("script_info[%d]\n", i));
			script_info* info = new script_info();
			info->read(in, this);
			m_script[i] = info;
		}

		assert(in->get_position() < eof);

		// read body_info
		n = in->read_vu30();
		m_body.resize(n);
		for (i = 0; i < n; i++)
		{
			IF_VERBOSE_PARSE(log_msg("body_info[%d]\n", i));
			body_info* info = new body_info();
			info->read(in, this);
			m_body[i] = info;

			printf("method	%i\n",info->m_method);
			log_disasm_avm2(info->m_code, *this);
		}

		assert(in->get_position() == eof);

	}

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
	void abc_def::read_cpool(stream* in)
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
				IF_VERBOSE_PARSE(log_msg("cpool_info: integer[%d]=%d\n", i, m_integer[i]));
			}
		}
		else
		{
			IF_VERBOSE_PARSE(log_msg("cpool_info: no integer pool\n"));
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
				IF_VERBOSE_PARSE(log_msg("cpool_info: uinteger[%d]=%d\n", i, m_uinteger[i]));
			}
		}
		else
		{
			IF_VERBOSE_PARSE(log_msg("cpool_info: no uinteger pool\n"));
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
				IF_VERBOSE_PARSE(log_msg("cpool_info: double[%d]=%f\n", i, m_double[i]));
			}
		}
		else
		{
			IF_VERBOSE_PARSE(log_msg("cpool_info: no double pool\n"));
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
				IF_VERBOSE_PARSE(log_msg("cpool_info: string[%d]='%s'\n", i, m_string[i].c_str()));
			}
		}
		else
		{
			IF_VERBOSE_PARSE(log_msg("cpool_info: no string pool\n"));
		}
		
		// namespace pool
		n = in->read_vu30();
		if (n > 0)
		{
			m_namespace.resize(n);
			namespac ns;
			m_namespace[0] = ns;	// default value

			for (int i = 1; i < n; i++)
			{
				ns.m_kind = static_cast<namespac::kind>(in->read_u8());
				ns.m_name = in->read_vu30();
				m_namespace[i] = ns;

				// User-defined namespaces have kind CONSTANT_Namespace or
				// CONSTANT_ExplicitNamespace and a non-empty name. 
				// System namespaces have empty names and one of the other kinds
				switch (ns.m_kind)
				{
					case namespac::CONSTANT_Namespace:
					case namespac::CONSTANT_ExplicitNamespace:
				//		assert(*get_string(ns.m_name) != 0);
						break;
					case namespac::CONSTANT_PackageNamespace:
					case namespac::CONSTANT_PackageInternalNs:
					case namespac::CONSTANT_ProtectedNamespace:
					case namespac::CONSTANT_StaticProtectedNs:
					case namespac::CONSTANT_PrivateNs:
				//		assert(*get_string(ns.m_name) == 0);
						break;
					default:
						assert(0);
				}
				IF_VERBOSE_PARSE(log_msg("cpool_info: namespace[%d]='%s', kind=0x%02X\n", 
					i, get_string(ns.m_name), ns.m_kind));
			}
		}
		else
		{
			IF_VERBOSE_PARSE(log_msg("cpool_info: no namespace pool\n"));
		}

		// namespace sets pool
		n = in->read_vu30();
		if (n > 0)
		{
			m_ns_set.resize(n);
			array<int> ns;
			m_ns_set[0] = ns;	// default value
			for (int i = 1; i < n; i++)
			{
				int count = in->read_vu30();
				ns.resize(count);
				for (int j = 0; j < count; j++)
				{
					ns[j] = in->read_vu30();
				}
				m_ns_set[i] = ns;
			}
		}
		else
		{
			IF_VERBOSE_PARSE(log_msg("cpool_info: no namespace sets\n"));
		}

		// multiname pool
		n = in->read_vu30();
		if (n > 0)
		{
			m_multiname.resize(n);
			multiname mn;
			m_multiname[0] = mn;	// default value
			for (int i = 1; i < n; i++)
			{
				Uint8 kind = in->read_u8();
				mn.m_kind = kind;
				switch (kind)
				{
					case multiname::CONSTANT_QName:
					case multiname::CONSTANT_QNameA:
					{
						mn.m_ns = in->read_vu30();
						mn.m_name = in->read_vu30();
						IF_VERBOSE_PARSE(log_msg("cpool_info: multiname[%d]='%s', ns='%s'\n", 
							i, get_string(mn.m_name), get_namespace(mn.m_ns)));
						break;
					}

					case multiname::CONSTANT_RTQName:
						assert(0&&"todo");
						break;

					case multiname::CONSTANT_RTQNameA:
						assert(0&&"todo");
						break;

					case multiname::CONSTANT_RTQNameL:
						assert(0&&"todo");
						break;

					case multiname::CONSTANT_RTQNameLA:
						assert(0&&"todo");
						break;

					case multiname::CONSTANT_Multiname:
					case multiname::CONSTANT_MultinameA:
						mn.m_ns_set = in->read_vu30();
						mn.m_name = in->read_vu30();
						IF_VERBOSE_PARSE(log_msg("cpool_info: multiname[%d]='%s', ns_set='%s'\n", 
							i, get_string(mn.m_name), "todo"));
						break;

					case multiname::CONSTANT_MultinameL:
						assert(0&&"todo");
						break;

					case multiname::CONSTANT_MultinameLA:
						assert(0&&"todo");
						break;

					default:
						assert(0);

				}
				m_multiname[i] = mn;
			}
		}
		else
		{
			IF_VERBOSE_PARSE(log_msg("cpool_info: no multiname pool\n"));
		}


	}

};	// end namespace gameswf


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
