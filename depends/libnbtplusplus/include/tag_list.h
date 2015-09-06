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
#ifndef TAG_LIST_H_INCLUDED
#define TAG_LIST_H_INCLUDED

#include "crtp_tag.h"
#include "tagfwd.h"
#include "value_initializer.h"
#include <stdexcept>
#include <vector>

#include "nbt++_export.h"

namespace nbt
{

/**
 * @brief Tag that contains multiple unnamed tags of the same type
 *
 * All the tags contained in the list have the same type, which can be queried
 * with el_type(). The types of the values contained in the list should not
 * be changed to mismatch the element type.
 *
 * If the list is empty, the type can be undetermined, in which case el_type()
 * will return tag_type::Null. The type will then be set when the first tag
 * is added to the list.
 */
class NBT___EXPORT tag_list final : public detail::crtp_tag<tag_list>
{
public:
    //Iterator types
    typedef std::vector<value>::iterator iterator;
    typedef std::vector<value>::const_iterator const_iterator;

    ///The type of the tag
    static constexpr tag_type type = tag_type::List;

    /**
     * @brief Constructs a list of type T with the given values
     *
     * Example: @code tag_list::of<tag_byte>({3, 4, 5}) @endcode
     * @param init list of values from which the elements are constructed
     */
    template<class T>
    static tag_list of(std::initializer_list<T> init);

    /**
     * @brief Constructs an empty list
     *
     * The content type is determined when the first tag is added.
     */
    tag_list(): tag_list(tag_type::Null) {}

    ///Constructs an empty list with the given content type
    explicit tag_list(tag_type type): el_type_(type) {}

    ///Constructs a list with the given contents
    tag_list(std::initializer_list<int8_t> init);
    tag_list(std::initializer_list<int16_t> init);
    tag_list(std::initializer_list<int32_t> init);
    tag_list(std::initializer_list<int64_t> init);
    tag_list(std::initializer_list<float> init);
    tag_list(std::initializer_list<double> init);
    tag_list(std::initializer_list<std::string> init);
    tag_list(std::initializer_list<tag_byte_array> init);
    tag_list(std::initializer_list<tag_list> init);
    tag_list(std::initializer_list<tag_compound> init);
    tag_list(std::initializer_list<tag_int_array> init);

    /**
     * @brief Constructs a list with the given contents
     * @throw std::invalid_argument if the tags are not all of the same type
     */
    tag_list(std::initializer_list<value> init);

    /**
     * @brief Accesses a tag by index with bounds checking
     *
     * Returns a value to the tag at the specified index, or throws an
     * exception if it is out of range.
     * @throw std::out_of_range if the index is out of range
     */
    value& at(size_t i);
    const value& at(size_t i) const;

    /**
     * @brief Accesses a tag by index
     *
     * Returns a value to the tag at the specified index. No bounds checking
     * is performed.
     */
    value& operator[](size_t i) { return tags[i]; }
    const value& operator[](size_t i) const { return tags[i]; }

    /**
     * @brief Assigns a value at the given index
     * @throw std::invalid_argument if the type of the value does not match the list's
     * content type
     * @throw std::out_of_range if the index is out of range
     */
    void set(size_t i, value&& val);

    /**
     * @brief Appends the tag to the end of the list
     * @throw std::invalid_argument if the type of the tag does not match the list's
     * content type
     */
    void push_back(value_initializer&& val);

    /**
     * @brief Constructs and appends a tag to the end of the list
     * @throw std::invalid_argument if the type of the tag does not match the list's
     * content type
     */
    template<class T, class... Args>
    void emplace_back(Args&&... args);

    ///Removes the last element of the list
    void pop_back() { tags.pop_back(); }

    ///Returns the content type of the list, or tag_type::Null if undetermined
    tag_type el_type() const { return el_type_; }

    ///Returns the number of tags in the list
    size_t size() const { return tags.size(); }

    ///Erases all tags from the list. Preserves the content type.
    void clear() { tags.clear(); }

    /**
     * @brief Erases all tags from the list and changes the content type.
     * @param type the new content type. Can be tag_type::Null to leave it undetermined.
     */
    void reset(tag_type type = tag_type::Null);

    //Iterators
    iterator begin() { return tags.begin(); }
    iterator end()   { return tags.end(); }
    const_iterator begin() const  { return tags.begin(); }
    const_iterator end() const    { return tags.end(); }
    const_iterator cbegin() const { return tags.cbegin(); }
    const_iterator cend() const   { return tags.cend(); }

    /**
     * @inheritdoc
     * In case of a list of tag_end, the content type will be undetermined.
     */
    void read_payload(io::stream_reader& reader) override;
    /**
     * @inheritdoc
     * In case of a list of undetermined content type, the written type will be tag_end.
     * @throw std::length_error if the list is too long for NBT
     */
    void write_payload(io::stream_writer& writer) const override;

    /**
     * @brief Equality comparison for lists
     *
     * Lists are considered equal if their content types and the contained tags
     * are equal.
     */
    NBT___EXPORT friend bool operator==(const tag_list& lhs, const tag_list& rhs);
    NBT___EXPORT friend bool operator!=(const tag_list& lhs, const tag_list& rhs);

private:
    std::vector<value> tags;
    tag_type el_type_;

    /**
     * Internally used initialization function that initializes the list with
     * tags of type T, with the constructor arguments of each T given by il.
     * @param il list of values that are, one by one, given to a constructor of T
     */
    template<class T, class Arg>
    void init(std::initializer_list<Arg> il);
};

template<class T>
tag_list tag_list::of(std::initializer_list<T> il)
{
    tag_list result;
    result.init<T, T>(il);
    return result;
}

template<class T, class... Args>
void tag_list::emplace_back(Args&&... args)
{
    if(el_type_ == tag_type::Null) //set content type if undetermined
        el_type_ = T::type;
    else if(el_type_ != T::type)
        throw std::invalid_argument("The tag type does not match the list's content type");
    tags.emplace_back(make_unique<T>(std::forward<Args>(args)...));
}

template<class T, class Arg>
void tag_list::init(std::initializer_list<Arg> init)
{
    el_type_ = T::type;
    tags.reserve(init.size());
    for(const Arg& arg: init)
        tags.emplace_back(make_unique<T>(arg));
}

}

#endif // TAG_LIST_H_INCLUDED
