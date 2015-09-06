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
#ifndef TAG_STRING_H_INCLUDED
#define TAG_STRING_H_INCLUDED

#include "crtp_tag.h"
#include <string>

#include "nbt++_export.h"

namespace nbt
{

///Tag that contains a UTF-8 string
class NBT___EXPORT tag_string final : public detail::crtp_tag<tag_string>
{
public:
    ///The type of the tag
    static constexpr tag_type type = tag_type::String;

    //Constructors
    tag_string() {}
    tag_string(const std::string& str): value(str) {}
    tag_string(std::string&& str) noexcept: value(std::move(str)) {}
    tag_string(const char* str): value(str) {}

    //Getters
    operator std::string&() { return value; }
    operator const std::string&() const { return value; }
    const std::string& get() const { return value; }

    //Setters
    tag_string& operator=(const std::string& str) { value = str; return *this; }
    tag_string& operator=(std::string&& str)      { value = std::move(str); return *this; }
    tag_string& operator=(const char* str)        { value = str; return *this; }
    void set(const std::string& str)              { value = str; }
    void set(std::string&& str)                   { value = std::move(str); }

    void read_payload(io::stream_reader& reader) override;
    /**
     * @inheritdoc
     * @throw std::length_error if the string is too long for NBT
     */
    void write_payload(io::stream_writer& writer) const override;

private:
    std::string value;
};

inline bool operator==(const tag_string& lhs, const tag_string& rhs)
{ return lhs.get() == rhs.get(); }
inline bool operator!=(const tag_string& lhs, const tag_string& rhs)
{ return !(lhs == rhs); }

}

#endif // TAG_STRING_H_INCLUDED
