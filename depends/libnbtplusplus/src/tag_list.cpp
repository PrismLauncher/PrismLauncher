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
#include "tag_list.h"
#include "nbt_tags.h"
#include "io/stream_reader.h"
#include "io/stream_writer.h"
#include <istream>

namespace nbt
{

tag_list::tag_list(std::initializer_list<int8_t>         il) { init<tag_byte>(il); }
tag_list::tag_list(std::initializer_list<int16_t>        il) { init<tag_short>(il); }
tag_list::tag_list(std::initializer_list<int32_t>        il) { init<tag_int>(il); }
tag_list::tag_list(std::initializer_list<int64_t>        il) { init<tag_long>(il); }
tag_list::tag_list(std::initializer_list<float>          il) { init<tag_float>(il); }
tag_list::tag_list(std::initializer_list<double>         il) { init<tag_double>(il); }
tag_list::tag_list(std::initializer_list<std::string>    il) { init<tag_string>(il); }
tag_list::tag_list(std::initializer_list<tag_byte_array> il) { init<tag_byte_array>(il); }
tag_list::tag_list(std::initializer_list<tag_list>       il) { init<tag_list>(il); }
tag_list::tag_list(std::initializer_list<tag_compound>   il) { init<tag_compound>(il); }
tag_list::tag_list(std::initializer_list<tag_int_array>  il) { init<tag_int_array>(il); }

tag_list::tag_list(std::initializer_list<value> init)
{
    if(init.size() == 0)
        el_type_ = tag_type::Null;
    else
    {
        el_type_ = init.begin()->get_type();
        for(const value& val: init)
        {
            if(!val || val.get_type() != el_type_)
                throw std::invalid_argument("The values are not all the same type");
        }
        tags.assign(init.begin(), init.end());
    }
}

value& tag_list::at(size_t i)
{
    return tags.at(i);
}

const value& tag_list::at(size_t i) const
{
    return tags.at(i);
}

void tag_list::set(size_t i, value&& val)
{
    if(val.get_type() != el_type_)
        throw std::invalid_argument("The tag type does not match the list's content type");
    tags.at(i) = std::move(val);
}

void tag_list::push_back(value_initializer&& val)
{
    if(!val) //don't allow null values
        throw std::invalid_argument("The value must not be null");
    if(el_type_ == tag_type::Null) //set content type if undetermined
        el_type_ = val.get_type();
    else if(el_type_ != val.get_type())
        throw std::invalid_argument("The tag type does not match the list's content type");
    tags.push_back(std::move(val));
}

void tag_list::reset(tag_type type)
{
    clear();
    el_type_ = type;
}

void tag_list::read_payload(io::stream_reader& reader)
{
    tag_type lt = reader.read_type(true);

    int32_t length;
    reader.read_num(length);
    if(length < 0)
        reader.get_istr().setstate(std::ios::failbit);
    if(!reader.get_istr())
        throw io::input_error("Error reading length of tag_list");

    if(lt != tag_type::End)
    {
        reset(lt);
        tags.reserve(length);

        for(int32_t i = 0; i < length; ++i)
            tags.emplace_back(reader.read_payload(lt));
    }
    else
    {
        //In case of tag_end, ignore the length and leave the type undetermined
        reset(tag_type::Null);
    }
}

void tag_list::write_payload(io::stream_writer& writer) const
{
    if(size() > io::stream_writer::max_array_len)
    {
        writer.get_ostr().setstate(std::ios::failbit);
        throw std::length_error("List is too large for NBT");
    }
    writer.write_type(el_type_ != tag_type::Null
                      ? el_type_
                      : tag_type::End);
    writer.write_num(static_cast<int32_t>(size()));
    for(const auto& val: tags)
    {
        //check if the value is of the correct type
        if(val.get_type() != el_type_)
        {
            writer.get_ostr().setstate(std::ios::failbit);
            throw std::logic_error("The tags in the list do not all match the content type");
        }
        writer.write_payload(val);
    }
}

bool operator==(const tag_list& lhs, const tag_list& rhs)
{
    return lhs.el_type_ == rhs.el_type_ && lhs.tags == rhs.tags;
}

bool operator!=(const tag_list& lhs, const tag_list& rhs)
{
    return !(lhs == rhs);
}

}
