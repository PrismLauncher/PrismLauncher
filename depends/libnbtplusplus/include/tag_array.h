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
#ifndef TAG_ARRAY_H_INCLUDED
#define TAG_ARRAY_H_INCLUDED

#include "crtp_tag.h"
#include <type_traits>
#include <vector>

#include "nbt++_export.h"

namespace nbt
{

///@cond
namespace detail
{
    ///Meta-struct that holds the tag_type value for a specific array type
    template<class T> struct get_array_type
    { static_assert(sizeof(T) != sizeof(T), "Invalid type paramter for tag_primitive, can only use byte or int"); };

    template<> struct get_array_type<int8_t>  : public std::integral_constant<tag_type, tag_type::Byte_Array> {};
    template<> struct get_array_type<int32_t> : public std::integral_constant<tag_type, tag_type::Int_Array> {};
}
///@cond

/**
 * @brief Tag that contains an array of byte or int values
 *
 * Common class for tag_byte_array and tag_int_array.
 */
template<class T>
class NBT___EXPORT tag_array final : public detail::crtp_tag<tag_array<T>>
{
public:
    //Iterator types
    typedef typename std::vector<T>::iterator iterator;
    typedef typename std::vector<T>::const_iterator const_iterator;

    ///The type of the contained values
    typedef T value_type;

    ///The type of the tag
    static constexpr tag_type type = detail::get_array_type<T>::value;

    ///Constructs an empty array
    tag_array() {}

    ///Constructs an array with the given values
    tag_array(std::initializer_list<T> init): data(init) {}
    tag_array(std::vector<T>&& vec) noexcept: data(std::move(vec)) {}

    ///Returns a reference to the vector that contains the values
    std::vector<T>& get() { return data; }
    const std::vector<T>& get() const { return data; }

    /**
     * @brief Accesses a value by index with bounds checking
     * @throw std::out_of_range if the index is out of range
     */
    T& at(size_t i);
    T at(size_t i) const;

    /**
     * @brief Accesses a value by index
     *
     * No bounds checking is performed.
     */
    T& operator[](size_t i) { return data[i]; }
    T operator[](size_t i) const { return data[i]; }

    ///Appends a value at the end of the array
    void push_back(T val) { data.push_back(val); }

    ///Removes the last element from the array
    void pop_back() { data.pop_back(); }

    ///Returns the number of values in the array
    size_t size() const { return data.size(); }

    ///Erases all values from the array.
    void clear() { data.clear(); }

    //Iterators
    iterator begin() { return data.begin(); }
    iterator end()   { return data.end(); }
    const_iterator begin() const  { return data.begin(); }
    const_iterator end() const    { return data.end(); }
    const_iterator cbegin() const { return data.cbegin(); }
    const_iterator cend() const   { return data.cend(); }

    void read_payload(io::stream_reader& reader) override;
    /**
     * @inheritdoc
     * @throw std::length_error if the array is too large for NBT
     */
    void write_payload(io::stream_writer& writer) const override;

private:
    std::vector<T> data;
};

template<class T> bool operator==(const tag_array<T>& lhs, const tag_array<T>& rhs)
{ return lhs.get() == rhs.get(); }
template<class T> bool operator!=(const tag_array<T>& lhs, const tag_array<T>& rhs)
{ return !(lhs == rhs); }

//Typedefs that should be used instead of the template tag_array.
typedef tag_array<int8_t> tag_byte_array;
typedef tag_array<int32_t> tag_int_array;

}

#endif // TAG_ARRAY_H_INCLUDED
