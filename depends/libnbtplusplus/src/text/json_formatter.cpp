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
#include "text/json_formatter.h"
#include "nbt_tags.h"
#include "nbt_visitor.h"
#include <cmath>
#include <iomanip>
#include <limits>

namespace nbt
{
namespace text
{

namespace //anonymous
{
    ///Helper class which uses the Visitor pattern to pretty-print tags
    class json_fmt_visitor : public const_nbt_visitor
    {
    public:
        json_fmt_visitor(std::ostream& os, const json_formatter& fmt):
            os(os)
        {}

        void visit(const tag_byte& b) override
        { os << static_cast<int>(b.get()) << "b"; } //We don't want to print a character

        void visit(const tag_short& s) override
        { os << s.get() << "s"; }

        void visit(const tag_int& i) override
        { os << i.get(); }

        void visit(const tag_long& l) override
        { os << l.get() << "l"; }

        void visit(const tag_float& f) override
        {
            write_float(f.get());
            os << "f";
        }

        void visit(const tag_double& d) override
        {
            write_float(d.get());
            os << "d";
        }

        void visit(const tag_byte_array& ba) override
        { os << "[" << ba.size() << " bytes]"; }

        void visit(const tag_string& s) override
        { os << '"' << s.get() << '"'; } //TODO: escape special characters

        void visit(const tag_list& l) override
        {
            //Wrap lines for lists of lists or compounds.
            //Lists of other types can usually be on one line without problem.
            const bool break_lines = l.size() > 0 &&
                (l.el_type() == tag_type::List || l.el_type() == tag_type::Compound);

            os << "[";
            if(break_lines)
            {
                os << "\n";
                ++indent_lvl;
                for(unsigned int i = 0; i < l.size(); ++i)
                {
                    indent();
                    if(l[i])
                        l[i].get().accept(*this);
                    else
                        write_null();
                    if(i != l.size()-1)
                        os << ",";
                    os << "\n";
                }
                --indent_lvl;
                indent();
            }
            else
            {
                for(unsigned int i = 0; i < l.size(); ++i)
                {
                    if(l[i])
                        l[i].get().accept(*this);
                    else
                        write_null();
                    if(i != l.size()-1)
                        os << ", ";
                }
            }
            os << "]";
        }

        void visit(const tag_compound& c) override
        {
            if(c.size() == 0) //No line breaks inside empty compounds please
            {
                os << "{}";
                return;
            }

            os << "{\n";
            ++indent_lvl;
            unsigned int i = 0;
            for(const auto& kv: c)
            {
                indent();
                os << kv.first << ": ";
                if(kv.second)
                    kv.second.get().accept(*this);
                else
                    write_null();
                if(i != c.size()-1)
                    os << ",";
                os << "\n";
                ++i;
            }
            --indent_lvl;
            indent();
            os << "}";
        }

        void visit(const tag_int_array& ia) override
        {
            os << "[";
            for(unsigned int i = 0; i < ia.size(); ++i)
            {
                os << ia[i];
                if(i != ia.size()-1)
                    os << ", ";
            }
            os << "]";
        }

    private:
        const std::string indent_str = "  ";

        std::ostream& os;
        int indent_lvl = 0;

        void indent()
        {
            for(int i = 0; i < indent_lvl; ++i)
                os << indent_str;
        }

        template<class T>
        void write_float(T val, int precision = std::numeric_limits<T>::max_digits10)
        {
            if(std::isfinite(val))
                os << std::setprecision(precision) << val;
            else if(std::isinf(val))
            {
                if(std::signbit(val))
                    os << "-";
                os << "Infinity";
            }
            else
                os << "NaN";
        }

        void write_null()
        {
            os << "null";
        }
    };
}

void json_formatter::print(std::ostream& os, const tag& t) const
{
    json_fmt_visitor v(os, *this);
    t.accept(v);
}

}
}
