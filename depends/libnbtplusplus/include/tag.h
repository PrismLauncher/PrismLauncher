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
#ifndef TAG_H_INCLUDED
#define TAG_H_INCLUDED

#include <cstdint>
#include <iosfwd>
#include <memory>

#include "nbt++_export.h"

namespace nbt
{

///Tag type values used in the binary format
enum class tag_type : int8_t
{
    End = 0,
    Byte = 1,
    Short = 2,
    Int = 3,
    Long = 4,
    Float = 5,
    Double = 6,
    Byte_Array = 7,
    String = 8,
    List = 9,
    Compound = 10,
    Int_Array = 11,
    Null = -1   ///< Used to denote empty @ref value s
};

/**
 * @brief Returns whether the given number falls within the range of valid tag types
 * @param allow_end whether to consider tag_type::End (0) valid
 */
NBT___EXPORT bool is_valid_type(int type, bool allow_end = false);

//Forward declarations
class nbt_visitor;
class const_nbt_visitor;
namespace io
{
    class stream_reader;
    class stream_writer;
}

///Base class for all NBT tag classes
class NBT___EXPORT tag
{
public:
    //Virtual destructor
    virtual ~tag() noexcept {}

    ///Returns the type of the tag
    virtual tag_type get_type() const noexcept = 0;

    //Polymorphic clone methods
    virtual std::unique_ptr<tag> clone() const& = 0;
    virtual std::unique_ptr<tag> move_clone() && = 0;
    std::unique_ptr<tag> clone() &&;

    /**
     * @brief Returns a reference to the tag as an instance of T
     * @throw std::bad_cast if the tag is not of type T
     */
    template<class T>
    T& as();
    template<class T>
    const T& as() const;

    /**
     * @brief Move-assigns the given tag if the class is the same
     * @throw std::bad_cast if @c rhs is not the same type as @c *this
     */
    virtual tag& assign(tag&& rhs) = 0;

    /**
     * @brief Calls the appropriate overload of @c visit() on the visitor with
     * @c *this as argument
     *
     * Implementing the Visitor pattern
     */
    virtual void accept(nbt_visitor& visitor) = 0;
    virtual void accept(const_nbt_visitor& visitor) const = 0;

    /**
     * @brief Reads the tag's payload from the stream
     * @throw io::stream_reader::input_error on failure
     */
    virtual void read_payload(io::stream_reader& reader) = 0;

    /**
     * @brief Writes the tag's payload into the stream
     */
    virtual void write_payload(io::stream_writer& writer) const = 0;

    /**
     * @brief Default-constructs a new tag of the given type
     * @throw std::invalid_argument if the type is not valid (e.g. End or Null)
     */
    static std::unique_ptr<tag> create(tag_type type);

    friend bool operator==(const tag& lhs, const tag& rhs);
    friend bool operator!=(const tag& lhs, const tag& rhs);

private:
    /**
     * @brief Checks for equality to a tag of the same type
     * @param rhs an instance of the same class as @c *this
     */
    virtual bool equals(const tag& rhs) const = 0;
};

///Output operator for tag types
NBT___EXPORT std::ostream& operator<<(std::ostream& os, tag_type tt);

/**
 * @brief Output operator for tags
 *
 * Uses @ref text::json_formatter
 * @relates tag
 */
NBT___EXPORT std::ostream& operator<<(std::ostream& os, const tag& t);

template<class T>
T& tag::as()
{
    static_assert(std::is_base_of<tag, T>::value, "T must be a subclass of tag");
    return dynamic_cast<T&>(*this);
}

template<class T>
const T& tag::as() const
{
    static_assert(std::is_base_of<tag, T>::value, "T must be a subclass of tag");
    return dynamic_cast<const T&>(*this);
}

}

#endif // TAG_H_INCLUDED
