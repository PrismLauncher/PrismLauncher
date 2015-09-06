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
#ifndef NBT_VISITOR_H_INCLUDED
#define NBT_VISITOR_H_INCLUDED

#include "tagfwd.h"

#include "nbt++_export.h"

namespace nbt
{

/**
 * @brief Base class for visitors of tags
 *
 * Implementing the Visitor pattern
 */
class NBT___EXPORT nbt_visitor
{
public:
    virtual ~nbt_visitor() noexcept = 0; //Abstract class

    virtual void visit(tag_byte&) {}
    virtual void visit(tag_short&) {}
    virtual void visit(tag_int&) {}
    virtual void visit(tag_long&) {}
    virtual void visit(tag_float&) {}
    virtual void visit(tag_double&) {}
    virtual void visit(tag_byte_array&) {}
    virtual void visit(tag_string&) {}
    virtual void visit(tag_list&) {}
    virtual void visit(tag_compound&) {}
    virtual void visit(tag_int_array&) {}
};

/**
 * @brief Base class for visitors of constant tags
 *
 * Implementing the Visitor pattern
 */
class NBT___EXPORT const_nbt_visitor
{
public:
    virtual ~const_nbt_visitor() noexcept = 0; //Abstract class

    virtual void visit(const tag_byte&) {}
    virtual void visit(const tag_short&) {}
    virtual void visit(const tag_int&) {}
    virtual void visit(const tag_long&) {}
    virtual void visit(const tag_float&) {}
    virtual void visit(const tag_double&) {}
    virtual void visit(const tag_byte_array&) {}
    virtual void visit(const tag_string&) {}
    virtual void visit(const tag_list&) {}
    virtual void visit(const tag_compound&) {}
    virtual void visit(const tag_int_array&) {}
};

inline nbt_visitor::~nbt_visitor() noexcept {}

inline const_nbt_visitor::~const_nbt_visitor() noexcept {}

}

#endif // NBT_VISITOR_H_INCLUDED
