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
#include "io/stream_writer.h"
#include <sstream>

namespace nbt
{
namespace io
{

void write_tag(const std::string& key, const tag& t, std::ostream& os, endian::endian e)
{
    stream_writer(os, e).write_tag(key, t);
}

void stream_writer::write_tag(const std::string& key, const tag& t)
{
    write_type(t.get_type());
    write_string(key);
    write_payload(t);
}

void stream_writer::write_string(const std::string& str)
{
    if(str.size() > max_string_len)
    {
        os.setstate(std::ios::failbit);
        std::ostringstream sstr;
        sstr << "String is too long for NBT (" << str.size() << " > " << max_string_len << ")";
        throw std::length_error(sstr.str());
    }
    write_num(static_cast<uint16_t>(str.size()));
    os.write(str.data(), str.size());
}

}
}
