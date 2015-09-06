/*
 * libnbt++ - A library for the Minecraft Named Binary Tag format.
 * Copyright (C) 2013, 2015  ljfa-ag
 *
 * This file is part of libnbt++.
 *
 * libnbt++ is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libnbt++ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libnbt++.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "tag.h"
#include "nbt_tags.h"
#include "text/json_formatter.h"
#include <limits>
#include <ostream>
#include <stdexcept>
#include <typeinfo>

namespace nbt
{

static_assert(std::numeric_limits<float>::is_iec559 && std::numeric_limits<double>::is_iec559,
    "The floating point values for NBT must conform to IEC 559/IEEE 754");

bool is_valid_type(int type, bool allow_end)
{
    return (allow_end ? 0 : 1) <= type && type <= 11;
}

std::unique_ptr<tag> tag::clone() &&
{
    return std::move(*this).move_clone();
}

std::unique_ptr<tag> tag::create(tag_type type)
{
    switch(type)
    {
    case tag_type::Byte:        return make_unique<tag_byte>();
    case tag_type::Short:       return make_unique<tag_short>();
    case tag_type::Int:         return make_unique<tag_int>();
    case tag_type::Long:        return make_unique<tag_long>();
    case tag_type::Float:       return make_unique<tag_float>();
    case tag_type::Double:      return make_unique<tag_double>();
    case tag_type::Byte_Array:  return make_unique<tag_byte_array>();
    case tag_type::String:      return make_unique<tag_string>();
    case tag_type::List:        return make_unique<tag_list>();
    case tag_type::Compound:    return make_unique<tag_compound>();
    case tag_type::Int_Array:   return make_unique<tag_int_array>();

    default: throw std::invalid_argument("Invalid tag type");
    }
}

bool operator==(const tag& lhs, const tag& rhs)
{
    if(typeid(lhs) != typeid(rhs))
        return false;
    return lhs.equals(rhs);
}

bool operator!=(const tag& lhs, const tag& rhs)
{
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& os, tag_type tt)
{
    switch(tt)
    {
    case tag_type::End:         return os << "end";
    case tag_type::Byte:        return os << "byte";
    case tag_type::Short:       return os << "short";
    case tag_type::Int:         return os << "int";
    case tag_type::Long:        return os << "long";
    case tag_type::Float:       return os << "float";
    case tag_type::Double:      return os << "double";
    case tag_type::Byte_Array:  return os << "byte_array";
    case tag_type::String:      return os << "string";
    case tag_type::List:        return os << "list";
    case tag_type::Compound:    return os << "compound";
    case tag_type::Int_Array:   return os << "int_array";
    case tag_type::Null:        return os << "null";

    default:                    return os << "invalid";
    }
}

std::ostream& operator<<(std::ostream& os, const tag& t)
{
    static const text::json_formatter formatter;
    formatter.print(os, t);
    return os;
}

}
