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
#ifndef JSON_FORMATTER_H_INCLUDED
#define JSON_FORMATTER_H_INCLUDED

#include "tagfwd.h"
#include <ostream>

#include "nbt++_export.h"

namespace nbt
{
namespace text
{

/**
 * @brief Prints tags in a JSON-like syntax into a stream
 *
 * @todo Make it configurable and able to produce actual standard-conformant JSON
 */
class NBT___EXPORT json_formatter
{
public:
    json_formatter() {};
    void print(std::ostream& os, const tag& t) const;
};

}
}

#endif // JSON_FORMATTER_H_INCLUDED
