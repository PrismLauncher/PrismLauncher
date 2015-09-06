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
#include "tag_string.h"
#include "io/stream_reader.h"
#include "io/stream_writer.h"

namespace nbt
{

void tag_string::read_payload(io::stream_reader& reader)
{
    try
    {
        value = reader.read_string();
    }
    catch(io::input_error& ex)
    {
        throw io::input_error("Error reading tag_string");
    }
}

void tag_string::write_payload(io::stream_writer& writer) const
{
    writer.write_string(value);
}

}
