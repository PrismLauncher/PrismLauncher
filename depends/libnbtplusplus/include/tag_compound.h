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
#ifndef TAG_COMPOUND_H_INCLUDED
#define TAG_COMPOUND_H_INCLUDED

#include "crtp_tag.h"
#include "value_initializer.h"
#include <map>
#include <string>

#include "nbt++_export.h"

namespace nbt
{

///Tag that contains multiple unordered named tags of arbitrary types
class NBT___EXPORT tag_compound final : public detail::crtp_tag<tag_compound>
{
    typedef std::map<std::string, value> map_t_;

public:
    //Iterator types
    typedef map_t_::iterator iterator;
    typedef map_t_::const_iterator const_iterator;

    ///The type of the tag
    static constexpr tag_type type = tag_type::Compound;

    ///Constructs an empty compound
    tag_compound() {}

    ///Constructs a compound with the given key-value pairs
    tag_compound(std::initializer_list<std::pair<std::string, value_initializer>> init);

    /**
     * @brief Accesses a tag by key with bounds checking
     *
     * Returns a value to the tag with the specified key, or throws an
     * exception if it does not exist.
     * @throw std::out_of_range if given key does not exist
     */
    value& at(const std::string& key);
    const value& at(const std::string& key) const;

    /**
     * @brief Accesses a tag by key
     *
     * Returns a value to the tag with the specified key. If it does not exist,
     * creates a new uninitialized entry under the key.
     */
    value& operator[](const std::string& key) { return tags[key]; }

    /**
     * @brief Inserts or assigns a tag
     *
     * If the given key already exists, assigns the tag to it.
     * Otherwise, it is inserted under the given key.
     * @return a pair of the iterator to the value and a bool indicating
     * whether the key did not exist
     */
    std::pair<iterator, bool> put(const std::string& key, value_initializer&& val);

    /**
     * @brief Inserts a tag if the key does not exist
     * @return a pair of the iterator to the value with the key and a bool
     * indicating whether the value was actually inserted
     */
    std::pair<iterator, bool> insert(const std::string& key, value_initializer&& val);

    /**
     * @brief Constructs and assigns or inserts a tag into the compound
     *
     * Constructs a new tag of type @c T with the given args and inserts
     * or assigns it to the given key.
     * @note Unlike std::map::emplace, this will overwrite existing values
     * @return a pair of the iterator to the value and a bool indicating
     * whether the key did not exist
     */
    template<class T, class... Args>
    std::pair<iterator, bool> emplace(const std::string& key, Args&&... args);

    /**
     * @brief Erases a tag from the compound
     * @return true if a tag was erased
     */
    bool erase(const std::string& key);

    ///Returns true if the given key exists in the compound
    bool has_key(const std::string& key) const;
    ///Returns true if the given key exists and the tag has the given type
    bool has_key(const std::string& key, tag_type type) const;

    ///Returns the number of tags in the compound
    size_t size() const { return tags.size(); }

    ///Erases all tags from the compound
    void clear() { tags.clear(); }

    //Iterators
    iterator begin() { return tags.begin(); }
    iterator end()   { return tags.end(); }
    const_iterator begin() const  { return tags.begin(); }
    const_iterator end() const    { return tags.end(); }
    const_iterator cbegin() const { return tags.cbegin(); }
    const_iterator cend() const   { return tags.cend(); }

    void read_payload(io::stream_reader& reader) override;
    void write_payload(io::stream_writer& writer) const override;

    NBT___EXPORT friend bool operator==(const tag_compound& lhs, const tag_compound& rhs)
    { return lhs.tags == rhs.tags; }
    NBT___EXPORT friend bool operator!=(const tag_compound& lhs, const tag_compound& rhs)
    { return !(lhs == rhs); }

private:
    map_t_ tags;
};

template<class T, class... Args>
NBT___EXPORT std::pair<tag_compound::iterator, bool> tag_compound::emplace(const std::string& key, Args&&... args)
{
    return put(key, value(make_unique<T>(std::forward<Args>(args)...)));
}

}

#endif // TAG_COMPOUND_H_INCLUDED
