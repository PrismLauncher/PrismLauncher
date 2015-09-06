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
#include "value.h"
#include "nbt_tags.h"
#include <typeinfo>

namespace nbt
{

value::value(tag&& t):
    tag_(std::move(t).move_clone())
{}

value::value(const value& rhs):
    tag_(rhs.tag_ ? rhs.tag_->clone() : nullptr)
{}

value& value::operator=(const value& rhs)
{
    if(this != &rhs)
    {
        tag_ = rhs.tag_ ? rhs.tag_->clone() : nullptr;
    }
    return *this;
}

value& value::operator=(tag&& t)
{
    set(std::move(t));
    return *this;
}

void value::set(tag&& t)
{
    if(tag_)
        tag_->assign(std::move(t));
    else
        tag_ = std::move(t).move_clone();
}

//Primitive assignment
//FIXME: Make this less copypaste!
value& value::operator=(int8_t val)
{
    if(!tag_)
        set(tag_byte(val));
    else switch(tag_->get_type())
    {
    case tag_type::Byte:
        static_cast<tag_byte&>(*tag_).set(val);
        break;
    case tag_type::Short:
        static_cast<tag_short&>(*tag_).set(val);
        break;
    case tag_type::Int:
        static_cast<tag_int&>(*tag_).set(val);
        break;
    case tag_type::Long:
        static_cast<tag_long&>(*tag_).set(val);
        break;
    case tag_type::Float:
        static_cast<tag_float&>(*tag_).set(val);
        break;
    case tag_type::Double:
        static_cast<tag_double&>(*tag_).set(val);
        break;

    default:
        throw std::bad_cast();
    }
    return *this;
}

value& value::operator=(int16_t val)
{
    if(!tag_)
        set(tag_short(val));
    else switch(tag_->get_type())
    {
    case tag_type::Short:
        static_cast<tag_short&>(*tag_).set(val);
        break;
    case tag_type::Int:
        static_cast<tag_int&>(*tag_).set(val);
        break;
    case tag_type::Long:
        static_cast<tag_long&>(*tag_).set(val);
        break;
    case tag_type::Float:
        static_cast<tag_float&>(*tag_).set(val);
        break;
    case tag_type::Double:
        static_cast<tag_double&>(*tag_).set(val);
        break;

    default:
        throw std::bad_cast();
    }
    return *this;
}

value& value::operator=(int32_t val)
{
    if(!tag_)
        set(tag_int(val));
    else switch(tag_->get_type())
    {
    case tag_type::Int:
        static_cast<tag_int&>(*tag_).set(val);
        break;
    case tag_type::Long:
        static_cast<tag_long&>(*tag_).set(val);
        break;
    case tag_type::Float:
        static_cast<tag_float&>(*tag_).set(val);
        break;
    case tag_type::Double:
        static_cast<tag_double&>(*tag_).set(val);
        break;

    default:
        throw std::bad_cast();
    }
    return *this;
}

value& value::operator=(int64_t val)
{
    if(!tag_)
        set(tag_long(val));
    else switch(tag_->get_type())
    {
    case tag_type::Long:
        static_cast<tag_long&>(*tag_).set(val);
        break;
    case tag_type::Float:
        static_cast<tag_float&>(*tag_).set(val);
        break;
    case tag_type::Double:
        static_cast<tag_double&>(*tag_).set(val);
        break;

    default:
        throw std::bad_cast();
    }
    return *this;
}

value& value::operator=(float val)
{
    if(!tag_)
        set(tag_float(val));
    else switch(tag_->get_type())
    {
    case tag_type::Float:
        static_cast<tag_float&>(*tag_).set(val);
        break;
    case tag_type::Double:
        static_cast<tag_double&>(*tag_).set(val);
        break;

    default:
        throw std::bad_cast();
    }
    return *this;
}

value& value::operator=(double val)
{
    if(!tag_)
        set(tag_double(val));
    else switch(tag_->get_type())
    {
    case tag_type::Double:
        static_cast<tag_double&>(*tag_).set(val);
        break;

    default:
        throw std::bad_cast();
    }
    return *this;
}

//Primitive conversion
value::operator int8_t() const
{
    switch(tag_->get_type())
    {
    case tag_type::Byte:
        return static_cast<tag_byte&>(*tag_).get();

    default:
        throw std::bad_cast();
    }
}

value::operator int16_t() const
{
    switch(tag_->get_type())
    {
    case tag_type::Byte:
        return static_cast<tag_byte&>(*tag_).get();
    case tag_type::Short:
        return static_cast<tag_short&>(*tag_).get();

    default:
        throw std::bad_cast();
    }
}

value::operator int32_t() const
{
    switch(tag_->get_type())
    {
    case tag_type::Byte:
        return static_cast<tag_byte&>(*tag_).get();
    case tag_type::Short:
        return static_cast<tag_short&>(*tag_).get();
    case tag_type::Int:
        return static_cast<tag_int&>(*tag_).get();

    default:
        throw std::bad_cast();
    }
}

value::operator int64_t() const
{
    switch(tag_->get_type())
    {
    case tag_type::Byte:
        return static_cast<tag_byte&>(*tag_).get();
    case tag_type::Short:
        return static_cast<tag_short&>(*tag_).get();
    case tag_type::Int:
        return static_cast<tag_int&>(*tag_).get();
    case tag_type::Long:
        return static_cast<tag_long&>(*tag_).get();

    default:
        throw std::bad_cast();
    }
}

value::operator float() const
{
    switch(tag_->get_type())
    {
    case tag_type::Byte:
        return static_cast<tag_byte&>(*tag_).get();
    case tag_type::Short:
        return static_cast<tag_short&>(*tag_).get();
    case tag_type::Int:
        return static_cast<tag_int&>(*tag_).get();
    case tag_type::Long:
        return static_cast<tag_long&>(*tag_).get();
    case tag_type::Float:
        return static_cast<tag_float&>(*tag_).get();

    default:
        throw std::bad_cast();
    }
}

value::operator double() const
{
    switch(tag_->get_type())
    {
    case tag_type::Byte:
        return static_cast<tag_byte&>(*tag_).get();
    case tag_type::Short:
        return static_cast<tag_short&>(*tag_).get();
    case tag_type::Int:
        return static_cast<tag_int&>(*tag_).get();
    case tag_type::Long:
        return static_cast<tag_long&>(*tag_).get();
    case tag_type::Float:
        return static_cast<tag_float&>(*tag_).get();
    case tag_type::Double:
        return static_cast<tag_double&>(*tag_).get();

    default:
        throw std::bad_cast();
    }
}

value& value::operator=(std::string&& str)
{
    if(!tag_)
        set(tag_string(std::move(str)));
    else
        dynamic_cast<tag_string&>(*tag_).set(std::move(str));
    return *this;
}

value::operator const std::string&() const
{
    return dynamic_cast<tag_string&>(*tag_).get();
}

value& value::at(const std::string& key)
{
    return dynamic_cast<tag_compound&>(*tag_).at(key);
}

const value& value::at(const std::string& key) const
{
    return dynamic_cast<const tag_compound&>(*tag_).at(key);
}

value& value::operator[](const std::string& key)
{
    return dynamic_cast<tag_compound&>(*tag_)[key];
}

value& value::operator[](const char* key)
{
    return (*this)[std::string(key)];
}

value& value::at(size_t i)
{
    return dynamic_cast<tag_list&>(*tag_).at(i);
}

const value& value::at(size_t i) const
{
    return dynamic_cast<const tag_list&>(*tag_).at(i);
}

value& value::operator[](size_t i)
{
    return dynamic_cast<tag_list&>(*tag_)[i];
}

const value& value::operator[](size_t i) const
{
    return dynamic_cast<const tag_list&>(*tag_)[i];
}

tag_type value::get_type() const
{
    return tag_ ? tag_->get_type() : tag_type::Null;
}

bool operator==(const value& lhs, const value& rhs)
{
    if(lhs.tag_ != nullptr && rhs.tag_ != nullptr)
        return *lhs.tag_ == *rhs.tag_;
    else
        return lhs.tag_ == nullptr && rhs.tag_ == nullptr;
}

bool operator!=(const value& lhs, const value& rhs)
{
    return !(lhs == rhs);
}

}
