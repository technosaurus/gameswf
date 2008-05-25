// gameswf_abc.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// do_abc tag reader

#ifndef GAMESWF_ABC_H
#define GAMESWF_ABC_H

#include "gameswf/gameswf_types.h"

namespace gameswf
{

	struct abc_def;

	struct multiname
	{
		enum kind
		{
			CONSTANT_UNDEFINED = 0,
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

		int m_kind;
		int m_flags;
		int m_ns;
		int m_ns_set;
		int m_name;

		multiname() :
			m_kind(CONSTANT_UNDEFINED),
			m_flags(0),
			m_ns(0),
			m_name(0)
		{
		}

		inline bool is_qname() const
		{
			return m_kind == CONSTANT_QName;
		}
	};

	struct namespac
	{
		enum kind
		{
			CONSTANT_Undefined = 0,
			CONSTANT_Namespace = 0x08,
			CONSTANT_PackageNamespace = 0x16,
			CONSTANT_PackageInternalNs = 0x17,
			CONSTANT_ProtectedNamespace = 0x18,
			CONSTANT_ExplicitNamespace = 0x19,
			CONSTANT_StaticProtectedNs = 0x1A,
			CONSTANT_PrivateNs = 0x05
		};

		kind m_kind;
		int	m_name;

		namespac() :
			m_kind(CONSTANT_Undefined),
			m_name(0)
		{
		}

	};

	struct option_detail
	{
		int m_value;
		Uint8 m_kind;
	};

	struct method_info : public ref_counted
	{
		enum flags
		{
			NEED_ARGUMENTS = 0x01,
			NEED_ACTIVATION = 0x02,
			NEED_REST = 0x04,
			HAS_OPTIONAL = 0x08,
			SET_DXNS = 0x40,
			HAS_PARAM_NAMES = 0x80
		};

		int m_return_type;
		array<int> m_param_type;
		int m_name;
		Uint8 m_flags;
		array<option_detail> m_options;
//		param_info m_param_names;

		void	read(stream* in, abc_def* abc);
	};

	struct metadata_info : public ref_counted
	{
		void	read(stream* in, abc_def* abc);
	};


	//
	// instance_info
	//

	struct traits_info : public ref_counted
	{
		enum kind
		{
			Trait_Slot = 0,
			Trait_Method = 1,
			Trait_Getter = 2,
			Trait_Setter = 3,
			Trait_Class = 4,
			Trait_Function = 5,
			Trait_Const = 6
		};

		enum attr
		{
			ATTR_Final = 0x1,
			ATTR_Override = 0x2,
			ATTR_Metadata = 0x4
		};

		int m_name;
		Uint8 m_kind;
		Uint8 m_attr;

		// data
		union
		{
			struct
			{
				int m_slot_id;
				int m_type_name;
				int m_vindex;
				Uint8 m_vkind;
			} trait_slot;

			struct
			{
				int m_slot_id;
				int m_classi;
			} trait_class;

			struct
			{
				int m_slot_id;
				int m_function;
			} trait_function;
			
			struct
			{
				int m_disp_id;
				int m_method;
			} trait_method;

		};


		array<int> m_metadata;

		void	read(stream* in, abc_def* abc);
	};

	struct instance_info : public ref_counted
	{

		enum flags
		{
			CONSTANT_ClassSealed = 0x01,
			CONSTANT_ClassFinal = 0x02,
			CONSTANT_ClassInterface = 0x04,
			CONSTANT_ClassProtectedNs = 0x08
		};

		int m_name;
		int m_super_name;
		Uint8 m_flags;
		int m_protectedNs;
		array<int> m_interface;
		int m_iinit;
		array< smart_ptr<traits_info> > m_trait;

		instance_info() :
			m_name(0),
			m_super_name(0),
			m_flags(0),
			m_protectedNs(0),
			m_iinit(0)
		{
		}

		void	read(stream* in, abc_def* abc);
	};

	struct class_info : public ref_counted
	{
		int m_cinit;
		array< smart_ptr<traits_info> > m_trait;

		void	read(stream* in, abc_def* abc);
	};

	struct script_info : public ref_counted
	{
		int m_init;
		array< smart_ptr<traits_info> > m_trait;

		void	read(stream* in, abc_def* abc);
	};

	struct exceptiin_info : public ref_counted
	{
		int m_from;
		int m_to;
		int m_target;
		int m_exc_type;
		int m_var_name;

		void	read(stream* in, abc_def* abc);
	};

	struct body_info : public ref_counted
	{
		int m_method;
		int m_max_stack;
		int m_local_count;
		int m_init_scope_depth;
		int m_max_scope_depth;
		array<Uint8> m_code;
		array< smart_ptr<exceptiin_info> > m_exception;
		array< smart_ptr<traits_info> > m_trait;

		void	read(stream* in, abc_def* abc);
	};

	struct abc_def : public ref_counted
	{
		// constant pool
		array<int> m_integer;
		array<Uint32> m_uinteger;
		array<double> m_double;
		array<tu_string> m_string;
		array<namespac> m_namespace;
		array< array<int> > m_ns_set;
		array<multiname> m_multiname;

		array< smart_ptr<method_info> > m_method;
		array< smart_ptr<metadata_info> > m_metadata;
		array< smart_ptr<instance_info> > m_instance;
		array< smart_ptr<class_info> > m_class;
		array< smart_ptr<script_info> > m_script;
		array< smart_ptr<body_info> > m_body;

		inline const char* get_string(int index) const
		{
			return m_string[index].c_str(); 
		}

		inline const char* get_namespace(int index) const
		{
			return get_string(m_namespace[index].m_name); 
		}

		inline const char* get_multiname(int index) const
		{
			return get_string(m_multiname[index].m_name); 
		}

		abc_def(player* player);
		virtual ~abc_def();

		void	read(stream* in);
		void	read_cpool(stream* in);

	};
}


#endif // GAMESWF_ABC_H
