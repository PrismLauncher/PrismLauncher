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
#ifndef VALUE_INITIALIZER_H_INCLUDED
#define VALUE_INITIALIZER_H_INCLUDED

#include "value.h"

#include "nbt++_export.h"

namespace nbt
{

/**
 * @brief Helper class for implicitly constructing value objects
 *
 * This type is a subclass of @ref value. However the only difference to value
 * is that this class has additional constructors which allow implicit
 * conversion of various types to value objects. These constructors are not
 * part of the value class itself because implicit conversions like this
 * (especially from @c tag&& to @c value) can cause problems and ambiguities
 * in some cases.
 *
 * value_initializer is especially useful as function parameter type, it will
 * allow convenient conversion of various values to tags on function call.
 *
 * As value_initializer objects are in no way different than value objects,
 * they can just be converted to value after construction.
 */
class NBT___EXPORT value_initializer : public value
{
public:
    value_initializer(std::unique_ptr<tag>&& t) noexcept: value(std::move(t)) {}
    value_initializer(std::nullptr_t) noexcept          : value(nullptr) {}
    value_initializer(value&& val) noexcept             : value(std::move(val)) {}
    value_initializer(tag&& t)                          : value(std::move(t)) {}

    value_initializer(int8_t val);
    value_initializer(int16_t val);
    value_initializer(int32_t val);
    value_initializer(int64_t val);
    value_initializer(float val);
    value_initializer(double val);
    value_initializer(const std::string& str);
    value_initializer(std::string&& str);
    value_initializer(const char* str);
};

}

#endif // VALUE_INITIALIZER_H_INCLUDED
