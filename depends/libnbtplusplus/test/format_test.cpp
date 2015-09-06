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
//#include "text/json_formatter.h"
//#include "io/stream_reader.h"
#include <fstream>
#include <iostream>
#include <limits>
#include "nbt_tags.h"

using namespace nbt;

int main()
{
    //TODO: Write that into a file
    tag_compound comp{
        {"byte", tag_byte(-128)},
        {"short", tag_short(-32768)},
        {"int", tag_int(-2147483648)},
        {"long", tag_long(-9223372036854775808U)},

        {"float 1", 1.618034f},
        {"float 2", 6.626070e-34f},
        {"float 3", 2.273737e+29f},
        {"float 4", -std::numeric_limits<float>::infinity()},
        {"float 5", std::numeric_limits<float>::quiet_NaN()},

        {"double 1", 3.141592653589793},
        {"double 2", 1.749899444387479e-193},
        {"double 3", 2.850825855152578e+175},
        {"double 4", -std::numeric_limits<double>::infinity()},
        {"double 5", std::numeric_limits<double>::quiet_NaN()},

        {"string 1", "Hello World! \u00E4\u00F6\u00FC\u00DF"},
        {"string 2", "String with\nline breaks\tand tabs"},

        {"byte array", tag_byte_array{12, 13, 14, 15, 16}},
        {"int array", tag_int_array{0x0badc0de, -0x0dedbeef, 0x1badbabe}},

        {"list (empty)", tag_list::of<tag_byte_array>({})},
        {"list (float)", tag_list{2.0f, 1.0f, 0.5f, 0.25f}},
        {"list (list)", tag_list::of<tag_list>({
            {},
            {4, 5, 6},
            {tag_compound{{"egg", "ham"}}, tag_compound{{"foo", "bar"}}}
        })},
        {"list (compound)", tag_list::of<tag_compound>({
            {{"created-on", 42}, {"names", tag_list{"Compound", "tag", "#0"}}},
            {{"created-on", 45}, {"names", tag_list{"Compound", "tag", "#1"}}}
        })},

        {"compound (empty)", tag_compound()},
        {"compound (nested)", tag_compound{
            {"key", "value"},
            {"key with \u00E4\u00F6\u00FC", tag_byte(-1)},
            {"key with\nnewline and\ttab", tag_compound{}}
        }},

        {"null", nullptr}
    };

    std::cout << "----- default operator<<:\n";
    std::cout << comp;
    std::cout << "\n-----" << std::endl;
}
