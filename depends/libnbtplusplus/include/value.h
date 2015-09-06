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
#ifndef TAG_REF_PROXY_H_INCLUDED
#define TAG_REF_PROXY_H_INCLUDED

#include "tag.h"
#include <string>
#include <type_traits>

#include "nbt++_export.h"

namespace nbt
{

/**
 * @brief Contains an NBT value of fixed type
 *
 * This class is a convenience wrapper for @c std::unique_ptr<tag>.
 * A value can contain any kind of tag or no tag (nullptr) and provides
 * operations for handling tags of which the type is not known at compile time.
 * Assignment or the set method on a value with no tag will fill in the value.
 *
 * The rationale for the existance of this class is to provide a type-erasured
 * means of storing tags, especially when they are contained in tag_compound
 * or tag_list. The alternative would be directly using @c std::unique_ptr<tag>
 * and @c tag&, which is how it was done in libnbt++1. The main drawback is that
 * it becomes very cumbersome to deal with tags of unknown type.
 *
 * For example, in this case it would not be possible to allow a syntax like
 * <tt>compound["foo"] = 42</tt>. If the key "foo" does not exist beforehand,
 * the left hand side could not have any sensible value if it was of type
 * @c tag&.
 * Firstly, the compound tag would have to create a new tag_int there, but it
 * cannot know that the new tag is going to be assigned an integer.
 * Also, if the type was @c tag& and it allowed assignment of integers, that
 * would mean the tag base class has assignments and conversions like this.
 * Which means that all other tag classes would inherit them from the base
 * class, even though it does not make any sense to allow converting a
 * tag_compound into an integer. Attempts like this should be caught at
 * compile time.
 *
 * This is why all the syntactic sugar for tags is contained in the value class
 * while the tag class only contains common operations for all tag types.
 */
class NBT___EXPORT value
{
public:
    //Constructors
    value() noexcept {}
    explicit value(std::unique_ptr<tag>&& t) noexcept: tag_(std::move(t)) {}
    explicit value(tag&& t);

    //Moving
    value(value&&) noexcept = default;
    value& operator=(value&&) noexcept = default;

    //Copying
    explicit value(const value& rhs);
    value& operator=(const value& rhs);

    /**
     * @brief Assigns the given value to the tag if the type matches
     * @throw std::bad_cast if the type of @c t is not the same as the type
     * of this value
     */
    value& operator=(tag&& t);
    void set(tag&& t);

    //Conversion to tag
    /**
     * @brief Returns the contained tag
     *
     * If the value is uninitialized, the behavior is undefined.
     */
    operator tag&() { return get(); }
    operator const tag&() const { return get(); }
    tag& get() { return *tag_; }
    const tag& get() const { return *tag_; }

    /**
     * @brief Returns a reference to the contained tag as an instance of T
     * @throw std::bad_cast if the tag is not of type T
     */
    template<class T>
    T& as();
    template<class T>
    const T& as() const;

    //Assignment of primitives and string
    /**
     * @brief Assigns the given value to the tag if the type is compatible
     * @throw std::bad_cast if the value is not convertible to the tag type
     * via a widening conversion
     */
    value& operator=(int8_t val);
    value& operator=(int16_t val);
    value& operator=(int32_t val);
    value& operator=(int64_t val);
    value& operator=(float val);
    value& operator=(double val);

    /**
     * @brief Assigns the given string to the tag if it is a tag_string
     * @throw std::bad_cast if the contained tag is not a tag_string
     */
    value& operator=(const std::string& str);
    value& operator=(std::string&& str);

    //Conversions to primitives and string
    /**
     * @brief Returns the contained value if the type is compatible
     * @throw std::bad_cast if the tag type is not convertible to the desired
     * type via a widening conversion
     */
    explicit operator int8_t() const;
    explicit operator int16_t() const;
    explicit operator int32_t() const;
    explicit operator int64_t() const;
    explicit operator float() const;
    explicit operator double() const;

    /**
     * @brief Returns the contained string if the type is tag_string
     *
     * If the value is uninitialized, the behavior is undefined.
     * @throw std::bad_cast if the tag type is not tag_string
     */
    explicit operator const std::string&() const;

    ///Returns true if the value is not uninitialized
    explicit operator bool() const { return tag_ != nullptr; }

    /**
     * @brief In case of a tag_compound, accesses a tag by key with bounds checking
     *
     * If the value is uninitialized, the behavior is undefined.
     * @throw std::bad_cast if the tag type is not tag_compound
     * @throw std::out_of_range if given key does not exist
     * @sa tag_compound::at
     */
    value& at(const std::string& key);
    const value& at(const std::string& key) const;

    /**
     * @brief In case of a tag_compound, accesses a tag by key
     *
     * If the value is uninitialized, the behavior is undefined.
     * @throw std::bad_cast if the tag type is not tag_compound
     * @sa tag_compound::operator[]
     */
    value& operator[](const std::string& key);
    value& operator[](const char* key); //need this overload because of conflict with built-in operator[]

    /**
     * @brief In case of a tag_list, accesses a tag by index with bounds checking
     *
     * If the value is uninitialized, the behavior is undefined.
     * @throw std::bad_cast if the tag type is not tag_list
     * @throw std::out_of_range if the index is out of range
     * @sa tag_list::at
     */
    value& at(size_t i);
    const value& at(size_t i) const;

    /**
     * @brief In case of a tag_list, accesses a tag by index
     *
     * No bounds checking is performed. If the value is uninitialized, the
     * behavior is undefined.
     * @throw std::bad_cast if the tag type is not tag_list
     * @sa tag_list::operator[]
     */
    value& operator[](size_t i);
    const value& operator[](size_t i) const;

    ///Returns a reference to the underlying std::unique_ptr<tag>
    std::unique_ptr<tag>& get_ptr() { return tag_; }
    const std::unique_ptr<tag>& get_ptr() const { return tag_; }
    ///Resets the underlying std::unique_ptr<tag> to a different value
    void set_ptr(std::unique_ptr<tag>&& t) { tag_ = std::move(t); }

    ///@sa tag::get_type
    tag_type get_type() const;

    NBT___EXPORT friend bool operator==(const value& lhs, const value& rhs);
    NBT___EXPORT friend bool operator!=(const value& lhs, const value& rhs);

private:
    std::unique_ptr<tag> tag_;
};

template<class T>
T& value::as()
{
    return tag_->as<T>();
}

template<class T>
const T& value::as() const
{
    return tag_->as<T>();
}

}

#endif // TAG_REF_PROXY_H_INCLUDED
