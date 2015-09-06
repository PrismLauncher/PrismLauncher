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
#ifndef PRIMITIVE_DETAIL_H_INCLUDED
#define PRIMITIVE_DETAIL_H_INCLUDED

#include <type_traits>

///@cond
namespace nbt
{

namespace detail
{
    ///Meta-struct that holds the tag_type value for a specific primitive type
    template<class T> struct get_primitive_type
    { static_assert(sizeof(T) != sizeof(T), "Invalid type paramter for tag_primitive, can only use types that NBT uses"); };

    template<> struct get_primitive_type<int8_t>  : public std::integral_constant<tag_type, tag_type::Byte> {};
    template<> struct get_primitive_type<int16_t> : public std::integral_constant<tag_type, tag_type::Short> {};
    template<> struct get_primitive_type<int32_t> : public std::integral_constant<tag_type, tag_type::Int> {};
    template<> struct get_primitive_type<int64_t> : public std::integral_constant<tag_type, tag_type::Long> {};
    template<> struct get_primitive_type<float>   : public std::integral_constant<tag_type, tag_type::Float> {};
    template<> struct get_primitive_type<double>  : public std::integral_constant<tag_type, tag_type::Double> {};
}

}
///@endcond

#endif // PRIMITIVE_DETAIL_H_INCLUDED
