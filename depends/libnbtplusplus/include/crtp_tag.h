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
#ifndef CRTP_TAG_H_INCLUDED
#define CRTP_TAG_H_INCLUDED

#include "tag.h"
#include "nbt_visitor.h"
#include "make_unique.h"

#include "nbt++_export.h"

namespace nbt
{

namespace detail
{

    template<class Sub>
    class NBT___EXPORT crtp_tag : public tag
    {
    public:
        //Pure virtual destructor to make the class abstract
        virtual ~crtp_tag() noexcept = 0;

        tag_type get_type() const noexcept override final { return Sub::type; };

        std::unique_ptr<tag> clone() const& override final { return make_unique<Sub>(sub_this()); }
        std::unique_ptr<tag> move_clone() && override final { return make_unique<Sub>(std::move(sub_this())); }

        tag& assign(tag&& rhs) override final { return sub_this() = dynamic_cast<Sub&&>(rhs); }

        void accept(nbt_visitor& visitor) override final { visitor.visit(sub_this()); }
        void accept(const_nbt_visitor& visitor) const override final { visitor.visit(sub_this()); }

    private:
        bool equals(const tag& rhs) const override final { return sub_this() == static_cast<const Sub&>(rhs); }

        Sub& sub_this() { return static_cast<Sub&>(*this); }
        const Sub& sub_this() const { return static_cast<const Sub&>(*this); }
    };

    template<class Sub>
    crtp_tag<Sub>::~crtp_tag() noexcept {}

}

}

#endif // CRTP_TAG_H_INCLUDED
