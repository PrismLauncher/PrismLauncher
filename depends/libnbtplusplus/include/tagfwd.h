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
/** @file
 * @brief Provides forward declarations for all tag classes
 */
#ifndef TAGFWD_H_INCLUDED
#define TAGFWD_H_INCLUDED
#include <cstdint>

namespace nbt
{

class tag;

template<class T> class tag_primitive;
typedef tag_primitive<int8_t> tag_byte;
typedef tag_primitive<int16_t> tag_short;
typedef tag_primitive<int32_t> tag_int;
typedef tag_primitive<int64_t> tag_long;
typedef tag_primitive<float> tag_float;
typedef tag_primitive<double> tag_double;

class tag_string;

template<class T> class tag_array;
typedef tag_array<int8_t> tag_byte_array;
typedef tag_array<int32_t> tag_int_array;

class tag_list;
class tag_compound;

}

#endif // TAGFWD_H_INCLUDED
