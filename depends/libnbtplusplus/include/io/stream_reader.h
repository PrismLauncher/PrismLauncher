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
#ifndef STREAM_READER_H_INCLUDED
#define STREAM_READER_H_INCLUDED

#include "endian_str.h"
#include "tag.h"
#include "tag_compound.h"
#include <iosfwd>
#include <memory>
#include <stdexcept>
#include <utility>

#include "nbt++_export.h"

namespace nbt
{
namespace io
{

///Exception that gets thrown when reading is not successful
class NBT___EXPORT input_error : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};

/**
* @brief Reads a named tag from the stream, making sure that it is a compound
* @param is the stream to read from
* @param e the byte order of the source data. The Java edition
* of Minecraft uses Big Endian, the Pocket edition uses Little Endian
* @throw input_error on failure, or if the tag in the stream is not a compound
*/
NBT___EXPORT std::pair<std::string, std::unique_ptr<tag_compound>> read_compound(std::istream& is, endian::endian e = endian::big);

/**
* @brief Reads a named tag from the stream
* @param is the stream to read from
* @param e the byte order of the source data. The Java edition
* of Minecraft uses Big Endian, the Pocket edition uses Little Endian
* @throw input_error on failure
*/
NBT___EXPORT std::pair<std::string, std::unique_ptr<tag>> read_tag(std::istream& is, endian::endian e = endian::big);

/**
 * @brief Helper class for reading NBT tags from input streams
 *
 * Can be reused to read multiple tags
 */
class NBT___EXPORT stream_reader
{
public:
    /**
     * @param is the stream to read from
     * @param e the byte order of the source data. The Java edition
     * of Minecraft uses Big Endian, the Pocket edition uses Little Endian
     */
    explicit stream_reader(std::istream& is, endian::endian e = endian::big) noexcept;

    ///Returns the stream
    std::istream& get_istr() const;
    ///Returns the byte order
    endian::endian get_endian() const;

    /**
     * @brief Reads a named tag from the stream, making sure that it is a compound
     * @throw input_error on failure, or if the tag in the stream is not a compound
     */
    std::pair<std::string, std::unique_ptr<tag_compound>> read_compound();

    /**
     * @brief Reads a named tag from the stream
     * @throw input_error on failure
     */
    std::pair<std::string, std::unique_ptr<tag>> read_tag();

    /**
     * @brief Reads a tag of the given type without name from the stream
     * @throw input_error on failure
     */
    std::unique_ptr<tag> read_payload(tag_type type);

    /**
     * @brief Reads a tag type from the stream
     * @param allow_end whether to consider tag_type::End valid
     * @throw input_error on failure
     */
    tag_type read_type(bool allow_end = false);

    /**
     * @brief Reads a binary number from the stream
     *
     * On failure, will set the failbit on the stream.
     */
    template<class T>
    void read_num(T& x);

    /**
     * @brief Reads an NBT string from the stream
     *
     * An NBT string consists of two bytes indicating the length, followed by
     * the characters encoded in modified UTF-8.
     * @throw input_error on failure
     */
    std::string read_string();

private:
    std::istream& is;
    const endian::endian endian;
};

template<class T>
void stream_reader::read_num(T& x)
{
    endian::read(is, x, endian);
}

}
}

#endif // STREAM_READER_H_INCLUDED
