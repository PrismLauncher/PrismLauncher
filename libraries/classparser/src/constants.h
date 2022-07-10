#pragma once
#include "errors.h"
#include "membuffer.h"
#include <sstream>

namespace java
{
enum class constant_type_t : uint8_t
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
    // FIXME: missing some constant types, see https://docs.oracle.com/javase/specs/jvms/se7/html/jvms-4.html#jvms-4.4
};

struct ref_type_t
{
    /**
        * Class reference:
        * an index within the constant pool to a UTF-8 string containing
        * the fully qualified class name (in internal format)
        * Used for j_class, j_fieldref, j_methodref and j_interface_methodref
        */
    uint16_t class_idx;
    // used for j_fieldref, j_methodref and j_interface_methodref
    uint16_t name_and_type_idx;
};

struct name_and_type_t
{
    uint16_t name_index;
    uint16_t descriptor_index;
};

class constant
{
public:
    constant_type_t type = constant_type_t::j_hole;

    constant(util::membuffer &buf)
    {
        buf.read(type);

        // load data depending on type
        switch (type)
        {
        case constant_type_t::j_float:
            buf.read_be(data.int_data);
            break;
        case constant_type_t::j_int:
            buf.read_be(data.int_data); // same as float data really
            break;
        case constant_type_t::j_double:
            buf.read_be(data.long_data);
            break;
        case constant_type_t::j_long:
            buf.read_be(data.long_data); // same as double
            break;
        case constant_type_t::j_class:
            buf.read_be(data.ref_type.class_idx);
            break;
        case constant_type_t::j_fieldref:
        case constant_type_t::j_methodref:
        case constant_type_t::j_interface_methodref:
            buf.read_be(data.ref_type.class_idx);
            buf.read_be(data.ref_type.name_and_type_idx);
            break;
        case constant_type_t::j_string:
            buf.read_be(data.index);
            break;
        case constant_type_t::j_string_data:
            // HACK HACK: for now, we call these UTF-8 and do no further processing.
            // Later, we should do some decoding. It's really modified UTF-8
            // * U+0000 is represented as 0xC0,0x80 invalid character
            // * any single zero byte ends the string
            // * characters above U+10000 are encoded like in CESU-8
            buf.read_jstr(str_data);
            break;
        case constant_type_t::j_nameandtype:
            buf.read_be(data.name_and_type.name_index);
            buf.read_be(data.name_and_type.descriptor_index);
            break;
        default:
            // invalid constant type!
            throw classfile_exception();
        }
    }
    constant(int)
    {
    }

    std::string toString()
    {
        std::ostringstream ss;
        switch (type)
        {
        case constant_type_t::j_hole:
            ss << "Fake legacy entry";
            break;
        case constant_type_t::j_float:
            ss << "Float: " << data.float_data;
            break;
        case constant_type_t::j_double:
            ss << "Double: " << data.double_data;
            break;
        case constant_type_t::j_int:
            ss << "Int: " << data.int_data;
            break;
        case constant_type_t::j_long:
            ss << "Long: " << data.long_data;
            break;
        case constant_type_t::j_string_data:
            ss << "StrData: " << str_data;
            break;
        case constant_type_t::j_string:
            ss << "Str: " << data.index;
            break;
        case constant_type_t::j_fieldref:
            ss << "FieldRef: " << data.ref_type.class_idx << " " << data.ref_type.name_and_type_idx;
            break;
        case constant_type_t::j_methodref:
            ss << "MethodRef: " << data.ref_type.class_idx << " " << data.ref_type.name_and_type_idx;
            break;
        case constant_type_t::j_interface_methodref:
            ss << "IfMethodRef: " << data.ref_type.class_idx << " " << data.ref_type.name_and_type_idx;
            break;
        case constant_type_t::j_class:
            ss << "Class: " << data.ref_type.class_idx;
            break;
        case constant_type_t::j_nameandtype:
            ss << "NameAndType: " << data.name_and_type.name_index << " "
               << data.name_and_type.descriptor_index;
            break;
        default:
            ss << "Invalid entry (" << int(type) << ")";
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
        ref_type_t ref_type;
        name_and_type_t name_and_type;
    } data = {0};
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
        // FIXME: @SANITY this should check for the end of buffer.
        uint16_t length = 0;
        buf.read_be(length);
        length--;
        const constant *last_constant = nullptr;
        while (length)
        {
            const constant &cnst = constant(buf);
            constants.push_back(cnst);
            last_constant = &constants[constants.size() - 1];
            if (last_constant->type == constant_type_t::j_double ||
                last_constant->type == constant_type_t::j_long)
            {
                // push in a fake constant to preserve indexing
                constants.push_back(constant(0));
                length -= 2;
            }
            else
            {
                length--;
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
            throw classfile_exception();
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
