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
#include <cxxtest/TestSuite.h>
#include "io/stream_reader.h"
#include "nbt_tags.h"
#include <iostream>
#include <fstream>
#include <sstream>

using namespace nbt;

class read_test : public CxxTest::TestSuite
{
public:
    void test_stream_reader_big()
    {
        std::string input{
            1, //tag_type::Byte
            0, //tag_type::End
            11, //tag_type::Int_Array

            0x0a, 0x0b, 0x0c, 0x0d, //0x0a0b0c0d in Big Endian

            0x00, 0x06, //String length in Big Endian
            'f', 'o', 'o', 'b', 'a', 'r',

            0 //tag_type::End (invalid with allow_end = false)
        };
        std::istringstream is(input);
        nbt::io::stream_reader reader(is);

        TS_ASSERT_EQUALS(&reader.get_istr(), &is);
        TS_ASSERT_EQUALS(reader.get_endian(), endian::big);

        TS_ASSERT_EQUALS(reader.read_type(), tag_type::Byte);
        TS_ASSERT_EQUALS(reader.read_type(true), tag_type::End);
        TS_ASSERT_EQUALS(reader.read_type(false), tag_type::Int_Array);

        int32_t i;
        reader.read_num(i);
        TS_ASSERT_EQUALS(i, 0x0a0b0c0d);

        TS_ASSERT_EQUALS(reader.read_string(), "foobar");

        TS_ASSERT_THROWS(reader.read_type(false), io::input_error);
        TS_ASSERT(!is);
        is.clear();

        //Test for invalid tag type 12
        is.str("\x0c");
        TS_ASSERT_THROWS(reader.read_type(), io::input_error);
        TS_ASSERT(!is);
        is.clear();

        //Test for unexpcted EOF on numbers (input too short for int32_t)
        is.str("\x03\x04");
        reader.read_num(i);
        TS_ASSERT(!is);
    }

    void test_stream_reader_little()
    {
        std::string input{
            0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, //0x0d0c0b0a09080706 in Little Endian

            0x06, 0x00, //String length in Little Endian
            'f', 'o', 'o', 'b', 'a', 'r',

            0x10, 0x00, //String length (intentionally too large)
            'a', 'b', 'c', 'd' //unexpected EOF
        };
        std::istringstream is(input);
        nbt::io::stream_reader reader(is, endian::little);

        TS_ASSERT_EQUALS(reader.get_endian(), endian::little);

        int64_t i;
        reader.read_num(i);
        TS_ASSERT_EQUALS(i, 0x0d0c0b0a09080706);

        TS_ASSERT_EQUALS(reader.read_string(), "foobar");

        TS_ASSERT_THROWS(reader.read_string(), io::input_error);
        TS_ASSERT(!is);
    }

    //Tests if comp equals an extended variant of Notch's bigtest NBT
    void verify_bigtest_structure(const tag_compound& comp)
    {
        TS_ASSERT_EQUALS(comp.size(), 13u);

        TS_ASSERT(comp.at("byteTest") == tag_byte(127));
        TS_ASSERT(comp.at("shortTest") == tag_short(32767));
        TS_ASSERT(comp.at("intTest") == tag_int(2147483647));
        TS_ASSERT(comp.at("longTest") == tag_long(9223372036854775807));
        TS_ASSERT(comp.at("floatTest") == tag_float(std::stof("0xff1832p-25"))); //0.4982315
        TS_ASSERT(comp.at("doubleTest") == tag_double(std::stod("0x1f8f6bbbff6a5ep-54"))); //0.493128713218231

        //From bigtest.nbt: "the first 1000 values of (n*n*255+n*7)%100, starting with n=0 (0, 62, 34, 16, 8, ...)"
        tag_byte_array byteArrayTest;
        for(int n = 0; n < 1000; ++n)
            byteArrayTest.push_back((n*n*255 + n*7) % 100);
        TS_ASSERT(comp.at("byteArrayTest (the first 1000 values of (n*n*255+n*7)%100, starting with n=0 (0, 62, 34, 16, 8, ...))") == byteArrayTest);

        TS_ASSERT(comp.at("stringTest") == tag_string("HELLO WORLD THIS IS A TEST STRING \u00C5\u00C4\u00D6!"));

        TS_ASSERT(comp.at("listTest (compound)") == tag_list::of<tag_compound>({
            {{"created-on", tag_long(1264099775885)}, {"name", "Compound tag #0"}},
            {{"created-on", tag_long(1264099775885)}, {"name", "Compound tag #1"}}
        }));
        TS_ASSERT(comp.at("listTest (long)") == tag_list::of<tag_long>({11, 12, 13, 14, 15}));
        TS_ASSERT(comp.at("listTest (end)") == tag_list());

        TS_ASSERT((comp.at("nested compound test") == tag_compound{
            {"egg", tag_compound{{"value", 0.5f},  {"name", "Eggbert"}}},
            {"ham", tag_compound{{"value", 0.75f}, {"name", "Hampus"}}}
        }));

        TS_ASSERT(comp.at("intArrayTest") == tag_int_array(
            {0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f}));
    }

    void test_read_bigtest()
    {
        //Uses an extended variant of Notch's original bigtest file
        std::ifstream file("bigtest_uncompr", std::ios::binary);
        TS_ASSERT(file);

        auto pair = nbt::io::read_compound(file);
        TS_ASSERT_EQUALS(pair.first, "Level");
        verify_bigtest_structure(*pair.second);
    }

    void test_read_littletest()
    {
        //Same as bigtest, but little endian
        std::ifstream file("littletest_uncompr", std::ios::binary);
        TS_ASSERT(file);

        auto pair = nbt::io::read_compound(file, endian::little);
        TS_ASSERT_EQUALS(pair.first, "Level");
        TS_ASSERT_EQUALS(pair.second->get_type(), tag_type::Compound);
        verify_bigtest_structure(*pair.second);
    }

    void test_read_errors()
    {
        std::ifstream file;
        nbt::io::stream_reader reader(file);

        //EOF within a tag_double payload
        file.open("errortest_eof1", std::ios::binary);
        TS_ASSERT(file);
        TS_ASSERT_THROWS(reader.read_tag(), io::input_error);
        TS_ASSERT(!file);

        //EOF within a key in a compound
        file.close();
        file.open("errortest_eof2", std::ios::binary);
        TS_ASSERT(file);
        TS_ASSERT_THROWS(reader.read_tag(), io::input_error);
        TS_ASSERT(!file);

        //Missing tag_end
        file.close();
        file.open("errortest_noend", std::ios::binary);
        TS_ASSERT(file);
        TS_ASSERT_THROWS(reader.read_tag(), io::input_error);
        TS_ASSERT(!file);

        //Negative list length
        file.close();
        file.open("errortest_neg_length", std::ios::binary);
        TS_ASSERT(file);
        TS_ASSERT_THROWS(reader.read_tag(), io::input_error);
        TS_ASSERT(!file);
    }

    void test_read_misc()
    {
        std::ifstream file;
        nbt::io::stream_reader reader(file);

        //Toplevel tag other than compound
        file.open("toplevel_string", std::ios::binary);
        TS_ASSERT(file);
        TS_ASSERT_THROWS(reader.read_compound(), io::input_error);
        TS_ASSERT(!file);

        //Rewind and try again with read_tag
        file.clear();
        TS_ASSERT(file.seekg(0));
        auto pair = reader.read_tag();
        TS_ASSERT_EQUALS(pair.first, "Test (toplevel tag_string)");
        TS_ASSERT(*pair.second == tag_string(
            "Even though unprovided for by NBT, the library should also handle "
            "the case where the file consists of something else than tag_compound"));
    }
};
