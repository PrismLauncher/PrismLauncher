#pragma once
#include "errors.h"
#include <sstream>

namespace java
{
class constant
{
public:
	enum type_t : uint8_t
	{
		j_hole = 0, // HACK: this is a hole in the array, because java is crazy
		j_string_data = 1,
		j_int = 3,
		j_float = 4,
		j_long = 5,
		j_double = 6,
		j_class = 7,
		j_string = 8,
		j_fieldref = 9,
		j_methodref = 10,
		j_interface_methodref = 11,
		j_nameandtype = 12
	} type;

	constant(util::membuffer &buf)
	{
		buf.read(type);
		// invalid constant type!
		if (type > j_nameandtype || type == (type_t)0 || type == (type_t)2)
			throw new classfile_exception();

		// load data depending on type
		switch (type)
		{
		case j_float:
		case j_int:
			buf.read_be(int_data); // same as float data really
			break;
		case j_double:
		case j_long:
			buf.read_be(long_data); // same as double
			break;
		case j_class:
			buf.read_be(ref_type.class_idx);
			break;
		case j_fieldref:
		case j_methodref:
		case j_interface_methodref:
			buf.read_be(ref_type.class_idx);
			buf.read_be(ref_type.name_and_type_idx);
			break;
		case j_string:
			buf.read_be(index);
			break;
		case j_string_data:
			// HACK HACK: for now, we call these UTF-8 and do no further processing.
			// Later, we should do some decoding. It's really modified UTF-8
			// * U+0000 is represented as 0xC0,0x80 invalid character
			// * any single zero byte ends the string
			// * characters above U+10000 are encoded like in CESU-8
			buf.read_jstr(str_data);
			break;
		case j_nameandtype:
			buf.read_be(name_and_type.name_index);
			buf.read_be(name_and_type.descriptor_index);
			break;
		}
	}

	constant(int fake)
	{
		type = j_hole;
	}

	std::string toString()
	{
		std::ostringstream ss;
		switch (type)
		{
		case j_hole:
			ss << "Fake legacy entry";
			break;
		case j_float:
			ss << "Float: " << float_data;
			break;
		case j_double:
			ss << "Double: " << double_data;
			break;
		case j_int:
			ss << "Int: " << int_data;
			break;
		case j_long:
			ss << "Long: " << long_data;
			break;
		case j_string_data:
			ss << "StrData: " << str_data;
			break;
		case j_string:
			ss << "Str: " << index;
			break;
		case j_fieldref:
			ss << "FieldRef: " << ref_type.class_idx << " " << ref_type.name_and_type_idx;
			break;
		case j_methodref:
			ss << "MethodRef: " << ref_type.class_idx << " " << ref_type.name_and_type_idx;
			break;
		case j_interface_methodref:
			ss << "IfMethodRef: " << ref_type.class_idx << " " << ref_type.name_and_type_idx;
			break;
		case j_class:
			ss << "Class: " << ref_type.class_idx;
			break;
		case j_nameandtype:
			ss << "NameAndType: " << name_and_type.name_index << " "
			   << name_and_type.descriptor_index;
			break;
		}
		return ss.str();
	}

	std::string str_data; /** String data in 'modified utf-8'.*/
	// store everything here.
	union
	{
		int32_t int_data;
		int64_t long_data;
		float float_data;
		double double_data;
		uint16_t index;
		struct
		{
			/**
			 * Class reference:
			 *   an index within the constant pool to a UTF-8 string containing
			 *   the fully qualified class name (in internal format)
			 * Used for j_class, j_fieldref, j_methodref and j_interface_methodref
			 */
			uint16_t class_idx;
			// used for j_fieldref, j_methodref and j_interface_methodref
			uint16_t name_and_type_idx;
		} ref_type;
		struct
		{
			uint16_t name_index;
			uint16_t descriptor_index;
		} name_and_type;
	};
};

/**
 * A helper class that represents the custom container used in Java class file for storage of
 * constants
 */
class constant_pool
{
public:
	/**
	 * Create a pool of constants
	 */
	constant_pool()
	{
	}
	/**
	 * Load a java constant pool
	 */
	void load(util::membuffer &buf)
	{
		uint16_t length = 0;
		buf.read_be(length);
		length--;
		uint16_t index = 1;
		const constant *last_constant = nullptr;
		while (length)
		{
			const constant &cnst = constant(buf);
			constants.push_back(cnst);
			last_constant = &constants[constants.size() - 1];
			if (last_constant->type == constant::j_double ||
				last_constant->type == constant::j_long)
			{
				// push in a fake constant to preserve indexing
				constants.push_back(constant(0));
				length -= 2;
				index += 2;
			}
			else
			{
				length--;
				index++;
			}
		}
	}
	typedef std::vector<java::constant> container_type;
	/**
	 * Access constants based on jar file index numbers (index of the first element is 1)
	 */
	java::constant &operator[](std::size_t constant_index)
	{
		if (constant_index == 0 || constant_index > constants.size())
		{
			throw new classfile_exception();
		}
		return constants[constant_index - 1];
	}
	;
	container_type::const_iterator begin() const
	{
		return constants.begin();
	}
	;
	container_type::const_iterator end() const
	{
		return constants.end();
	}

private:
	container_type constants;
};
}
