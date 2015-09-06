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
#include "value_initializer.h"
#include "nbt_tags.h"

namespace nbt
{

value_initializer::value_initializer(int8_t val)            : value(tag_byte(val)) {}
value_initializer::value_initializer(int16_t val)           : value(tag_short(val)) {}
value_initializer::value_initializer(int32_t val)           : value(tag_int(val)) {}
value_initializer::value_initializer(int64_t val)           : value(tag_long(val)) {}
value_initializer::value_initializer(float val)             : value(tag_float(val)) {}
value_initializer::value_initializer(double val)            : value(tag_double(val)) {}
value_initializer::value_initializer(const std::string& str): value(tag_string(str)) {}
value_initializer::value_initializer(std::string&& str)     : value(tag_string(std::move(str))) {}
value_initializer::value_initializer(const char* str)       : value(tag_string(str)) {}

}
