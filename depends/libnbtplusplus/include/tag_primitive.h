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
#ifndef TAG_PRIMITIVE_H_INCLUDED
#define TAG_PRIMITIVE_H_INCLUDED

#include "crtp_tag.h"
#include "primitive_detail.h"
#include "io/stream_reader.h"
#include "io/stream_writer.h"
#include <istream>
#include <sstream>

#include "nbt++_export.h"

namespace nbt
{

/**
 * @brief Tag that contains an integral or floating-point value
 *
 * Common class for tag_byte, tag_short, tag_int, tag_long, tag_float and tag_double.
 */
template<class T>
class tag_primitive final : public detail::crtp_tag<tag_primitive<T>>
{
public:
    ///The type of the value
    typedef T value_type;

    ///The type of the tag
    static constexpr tag_type type = detail::get_primitive_type<T>::value;

    //Constructor
    constexpr tag_primitive(T val = 0) noexcept: value(val) {}

    //Getters
    operator T&() { return value; }
    constexpr operator T() const { return value; }
    constexpr T get() const { return value; }

    //Setters
    tag_primitive& operator=(T val) { value = val; return *this; }
    void set(T val) { value = val; }

    void read_payload(io::stream_reader& reader) override;
    void write_payload(io::stream_writer& writer) const override;

private:
    T value;
};

template<class T> bool operator==(const tag_primitive<T>& lhs, const tag_primitive<T>& rhs)
{ return lhs.get() == rhs.get(); }
template<class T> bool operator!=(const tag_primitive<T>& lhs, const tag_primitive<T>& rhs)
{ return !(lhs == rhs); }

//Typedefs that should be used instead of the template tag_primitive.
typedef tag_primitive<int8_t> tag_byte;
typedef tag_primitive<int16_t> tag_short;
typedef tag_primitive<int32_t> tag_int;
typedef tag_primitive<int64_t> tag_long;
typedef tag_primitive<float> tag_float;
typedef tag_primitive<double> tag_double;

template<class T>
void tag_primitive<T>::read_payload(io::stream_reader& reader)
{
    reader.read_num(value);
    if(!reader.get_istr())
    {
        std::ostringstream str;
        str << "Error reading tag_" << type;
        throw io::input_error(str.str());
    }
}

template<class T>
void tag_primitive<T>::write_payload(io::stream_writer& writer) const
{
    writer.write_num(value);
}

}

#endif // TAG_PRIMITIVE_H_INCLUDED
