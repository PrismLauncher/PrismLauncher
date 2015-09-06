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
#include "io/stream_reader.h"
#include "make_unique.h"
#include "tag_compound.h"
#include <istream>

namespace nbt
{
namespace io
{

std::pair<std::string, std::unique_ptr<tag_compound>> read_compound(std::istream& is, endian::endian e)
{
    return stream_reader(is, e).read_compound();
}

std::pair<std::string, std::unique_ptr<tag>> read_tag(std::istream& is, endian::endian e)
{
    return stream_reader(is, e).read_tag();
}

stream_reader::stream_reader(std::istream& is, endian::endian e) noexcept:
    is(is), endian(e)
{}

std::istream& stream_reader::get_istr() const
{
    return is;
}

endian::endian stream_reader::get_endian() const
{
    return endian;
}

std::pair<std::string, std::unique_ptr<tag_compound>> stream_reader::read_compound()
{
    if(read_type() != tag_type::Compound)
    {
        is.setstate(std::ios::failbit);
        throw input_error("Tag is not a compound");
    }
    std::string key = read_string();
    auto comp = make_unique<tag_compound>();
    comp->read_payload(*this);
    return {std::move(key), std::move(comp)};
}

std::pair<std::string, std::unique_ptr<tag>> stream_reader::read_tag()
{
    tag_type type = read_type();
    std::string key = read_string();
    std::unique_ptr<tag> t = read_payload(type);
    return {std::move(key), std::move(t)};
}

std::unique_ptr<tag> stream_reader::read_payload(tag_type type)
{
    std::unique_ptr<tag> t = tag::create(type);
    t->read_payload(*this);
    return t;
}

tag_type stream_reader::read_type(bool allow_end)
{
    int type = is.get();
    if(!is)
        throw input_error("Error reading tag type");
    if(!is_valid_type(type, allow_end))
    {
        is.setstate(std::ios::failbit);
        throw input_error("Invalid tag type: " + std::to_string(type));
    }
    return static_cast<tag_type>(type);
}

std::string stream_reader::read_string()
{
    uint16_t len;
    read_num(len);
    if(!is)
        throw input_error("Error reading string");

    std::string ret(len, '\0');
    is.read(&ret[0], len); //C++11 allows us to do this
    if(!is)
        throw input_error("Error reading string");
    return ret;
}

}
}
