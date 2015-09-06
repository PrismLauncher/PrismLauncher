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
#ifndef STREAM_WRITER_H_INCLUDED
#define STREAM_WRITER_H_INCLUDED

#include "tag.h"
#include "endian_str.h"
#include <iosfwd>

#include "nbt++_export.h"

namespace nbt
{
namespace io
{

/* Not sure if that is even needed
///Exception that gets thrown when writing is not successful
class output_error : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};*/

/**
* @brief Writes a named tag into the stream, including the tag type
* @param key the name of the tag
* @param t the tag
* @param os the stream to write to
* @param e the byte order of the written data. The Java edition
* of Minecraft uses Big Endian, the Pocket edition uses Little Endian
*/
NBT___EXPORT void write_tag(const std::string& key, const tag& t, std::ostream& os, endian::endian e = endian::big);

/**
 * @brief Helper class for writing NBT tags to output streams
 *
 * Can be reused to write multiple tags
 */
class NBT___EXPORT stream_writer
{
public:
    ///Maximum length of an NBT string (16 bit unsigned)
    static constexpr size_t max_string_len = UINT16_MAX;
    ///Maximum length of an NBT list or array (32 bit signed)
    static constexpr uint32_t max_array_len = INT32_MAX;

    /**
     * @param os the stream to write to
     * @param e the byte order of the written data. The Java edition
     * of Minecraft uses Big Endian, the Pocket edition uses Little Endian
     */
    explicit stream_writer(std::ostream& os, endian::endian e = endian::big) noexcept:
        os(os), endian(e)
    {}

    ///Returns the stream
    std::ostream& get_ostr() const { return os; }
    ///Returns the byte order
    endian::endian get_endian() const { return endian; }

    /**
     * @brief Writes a named tag into the stream, including the tag type
     */
    void write_tag(const std::string& key, const tag& t);

    /**
     * @brief Writes the given tag's payload into the stream
     */
    void write_payload(const tag& t) { t.write_payload(*this); }

    /**
     * @brief Writes a tag type to the stream
     */
    void write_type(tag_type tt) { write_num(static_cast<int8_t>(tt)); }

    /**
     * @brief Writes a binary number to the stream
     */
    template<class T>
    void write_num(T x);

    /**
     * @brief Writes an NBT string to the stream
     *
     * An NBT string consists of two bytes indicating the length, followed by
     * the characters encoded in modified UTF-8.
     * @throw std::length_error if the string is too long for NBT
     */
    void write_string(const std::string& str);

private:
    std::ostream& os;
    const endian::endian endian;
};

template<class T>
void stream_writer::write_num(T x)
{
    endian::write(os, x, endian);
}

}
}

#endif // STREAM_WRITER_H_INCLUDED
