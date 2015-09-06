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
#include "nbt_tags.h"
#include "nbt_visitor.h"
#include <algorithm>
#include <set>
#include <stdexcept>

using namespace nbt;

class nbttest : public CxxTest::TestSuite
{
public:
    void test_tag()
    {
        TS_ASSERT(!is_valid_type(-1));
        TS_ASSERT(!is_valid_type(0));
        TS_ASSERT(is_valid_type(0, true));
        TS_ASSERT(is_valid_type(1));
        TS_ASSERT(is_valid_type(5, false));
        TS_ASSERT(is_valid_type(7, true));
        TS_ASSERT(is_valid_type(11));
        TS_ASSERT(!is_valid_type(12));

        //looks like TS_ASSERT_EQUALS can't handle abstract classes...
        TS_ASSERT(*tag::create(tag_type::Byte) == tag_byte());
        TS_ASSERT_THROWS(tag::create(tag_type::Null), std::invalid_argument);
        TS_ASSERT_THROWS(tag::create(tag_type::End), std::invalid_argument);

        tag_string tstr("foo");
        auto cl = tstr.clone();
        TS_ASSERT_EQUALS(tstr.get(), "foo");
        TS_ASSERT(tstr == *cl);

        cl = std::move(tstr).clone();
        TS_ASSERT(*cl == tag_string("foo"));
        TS_ASSERT(*cl != tag_string("bar"));

        cl = std::move(*cl).move_clone();
        TS_ASSERT(*cl == tag_string("foo"));

        tstr.assign(tag_string("bar"));
        TS_ASSERT_THROWS(tstr.assign(tag_int(6)), std::bad_cast);
        TS_ASSERT_EQUALS(tstr.get(), "bar");

        TS_ASSERT_EQUALS(&tstr.as<tag_string>(), &tstr);
        TS_ASSERT_THROWS(tstr.as<tag_byte_array>(), std::bad_cast);
    }

    void test_get_type()
    {
        TS_ASSERT_EQUALS(tag_byte().get_type()      , tag_type::Byte);
        TS_ASSERT_EQUALS(tag_short().get_type()     , tag_type::Short);
        TS_ASSERT_EQUALS(tag_int().get_type()       , tag_type::Int);
        TS_ASSERT_EQUALS(tag_long().get_type()      , tag_type::Long);
        TS_ASSERT_EQUALS(tag_float().get_type()     , tag_type::Float);
        TS_ASSERT_EQUALS(tag_double().get_type()    , tag_type::Double);
        TS_ASSERT_EQUALS(tag_byte_array().get_type(), tag_type::Byte_Array);
        TS_ASSERT_EQUALS(tag_string().get_type()    , tag_type::String);
        TS_ASSERT_EQUALS(tag_list().get_type()      , tag_type::List);
        TS_ASSERT_EQUALS(tag_compound().get_type()  , tag_type::Compound);
        TS_ASSERT_EQUALS(tag_int_array().get_type() , tag_type::Int_Array);
    }

    void test_tag_primitive()
    {
        tag_int tag(6);
        TS_ASSERT_EQUALS(tag.get(), 6);
        int& ref = tag;
        ref = 12;
        TS_ASSERT(tag == 12);
        TS_ASSERT(tag != 6);
        tag.set(24);
        TS_ASSERT_EQUALS(ref, 24);
        tag = 7;
        TS_ASSERT_EQUALS(static_cast<int>(tag), 7);

        TS_ASSERT_EQUALS(tag, tag_int(7));
        TS_ASSERT_DIFFERS(tag_float(2.5), tag_float(-2.5));
        TS_ASSERT_DIFFERS(tag_float(2.5), tag_double(2.5));

        TS_ASSERT(tag_double() == 0.0);

        TS_ASSERT_EQUALS(tag_byte(INT8_MAX).get(), INT8_MAX);
        TS_ASSERT_EQUALS(tag_byte(INT8_MIN).get(), INT8_MIN);
        TS_ASSERT_EQUALS(tag_short(INT16_MAX).get(), INT16_MAX);
        TS_ASSERT_EQUALS(tag_short(INT16_MIN).get(), INT16_MIN);
        TS_ASSERT_EQUALS(tag_int(INT32_MAX).get(), INT32_MAX);
        TS_ASSERT_EQUALS(tag_int(INT32_MIN).get(), INT32_MIN);
        TS_ASSERT_EQUALS(tag_long(INT64_MAX).get(), INT64_MAX);
        TS_ASSERT_EQUALS(tag_long(INT64_MIN).get(), INT64_MIN);
    }

    void test_tag_string()
    {
        tag_string tag("foo");
        TS_ASSERT_EQUALS(tag.get(), "foo");
        std::string& ref = tag;
        ref = "bar";
        TS_ASSERT_EQUALS(tag.get(), "bar");
        TS_ASSERT_DIFFERS(tag.get(), "foo");
        tag.set("baz");
        TS_ASSERT_EQUALS(ref, "baz");
        tag = "quux";
        TS_ASSERT_EQUALS("quux", static_cast<std::string>(tag));
        std::string str("foo");
        tag = str;
        TS_ASSERT_EQUALS(tag.get(),str);

        TS_ASSERT_EQUALS(tag_string(str).get(), "foo");
        TS_ASSERT_EQUALS(tag_string().get(), "");
    }

    void test_tag_compound()
    {
        tag_compound comp{
            {"foo", int16_t(12)},
            {"bar", "baz"},
            {"baz", -2.0},
            {"list", tag_list{16, 17}}
        };

        //Test assignments and conversions, and exceptions on bad conversions
        TS_ASSERT_EQUALS(comp["foo"].get_type(), tag_type::Short);
        TS_ASSERT_EQUALS(static_cast<int32_t>(comp["foo"]), 12);
        TS_ASSERT_EQUALS(static_cast<int16_t>(comp.at("foo")), int16_t(12));
        TS_ASSERT(comp["foo"] == tag_short(12));
        TS_ASSERT_THROWS(static_cast<int8_t>(comp["foo"]), std::bad_cast);
        TS_ASSERT_THROWS(static_cast<std::string>(comp["foo"]), std::bad_cast);

        TS_ASSERT_THROWS(comp["foo"] = 32, std::bad_cast);
        comp["foo"] = int8_t(32);
        TS_ASSERT_EQUALS(static_cast<int16_t>(comp["foo"]), 32);

        TS_ASSERT_EQUALS(comp["bar"].get_type(), tag_type::String);
        TS_ASSERT_EQUALS(static_cast<std::string>(comp["bar"]), "baz");
        TS_ASSERT_THROWS(static_cast<int>(comp["bar"]), std::bad_cast);

        TS_ASSERT_THROWS(comp["bar"] = -128, std::bad_cast);
        comp["bar"] = "barbaz";
        TS_ASSERT_EQUALS(static_cast<std::string>(comp["bar"]), "barbaz");

        TS_ASSERT_EQUALS(comp["baz"].get_type(), tag_type::Double);
        TS_ASSERT_EQUALS(static_cast<double>(comp["baz"]), -2.0);
        TS_ASSERT_THROWS(static_cast<float>(comp["baz"]), std::bad_cast);

        //Test nested access
        comp["quux"] = tag_compound{{"Hello", "World"}, {"zero", 0}};
        TS_ASSERT_EQUALS(comp.at("quux").get_type(), tag_type::Compound);
        TS_ASSERT_EQUALS(static_cast<std::string>(comp["quux"].at("Hello")), "World");
        TS_ASSERT_EQUALS(static_cast<std::string>(comp["quux"]["Hello"]), "World");
        TS_ASSERT(comp["list"][1] == tag_int(17));

        TS_ASSERT_THROWS(comp.at("nothing"), std::out_of_range);

        //Test equality comparisons
        tag_compound comp2{
            {"foo", int16_t(32)},
            {"bar", "barbaz"},
            {"baz", -2.0},
            {"quux", tag_compound{{"Hello", "World"}, {"zero", 0}}},
            {"list", tag_list{16, 17}}
        };
        TS_ASSERT(comp == comp2);
        TS_ASSERT(comp != dynamic_cast<const tag_compound&>(comp2["quux"].get()));
        TS_ASSERT(comp != comp2["quux"]);
        TS_ASSERT(dynamic_cast<const tag_compound&>(comp["quux"].get()) == comp2["quux"]);

        //Test whether begin() through end() goes through all the keys and their
        //values.  The order of iteration is irrelevant there.
        std::set<std::string> keys{"bar", "baz", "foo", "list", "quux"};
        TS_ASSERT_EQUALS(comp2.size(), keys.size());
        unsigned int i = 0;
        for(const std::pair<const std::string, value>& val: comp2)
        {
            TS_ASSERT_LESS_THAN(i, comp2.size());
            TS_ASSERT(keys.count(val.first));
            TS_ASSERT(val.second == comp2[val.first]);
            ++i;
        }
        TS_ASSERT_EQUALS(i, comp2.size());

        //Test erasing and has_key
        TS_ASSERT_EQUALS(comp.erase("nothing"), false);
        TS_ASSERT(comp.has_key("quux"));
        TS_ASSERT(comp.has_key("quux", tag_type::Compound));
        TS_ASSERT(!comp.has_key("quux", tag_type::List));
        TS_ASSERT(!comp.has_key("quux", tag_type::Null));

        TS_ASSERT_EQUALS(comp.erase("quux"), true);
        TS_ASSERT(!comp.has_key("quux"));
        TS_ASSERT(!comp.has_key("quux", tag_type::Compound));
        TS_ASSERT(!comp.has_key("quux", tag_type::Null));

        comp.clear();
        TS_ASSERT(comp == tag_compound{});

        //Test inserting values
        TS_ASSERT_EQUALS(comp.put("abc", tag_double(6.0)).second, true);
        TS_ASSERT_EQUALS(comp.put("abc", tag_long(-28)).second, false);
        TS_ASSERT_EQUALS(comp.insert("ghi", tag_string("world")).second, true);
        TS_ASSERT_EQUALS(comp.insert("abc", tag_string("hello")).second, false);
        TS_ASSERT_EQUALS(comp.emplace<tag_string>("def", "ghi").second, true);
        TS_ASSERT_EQUALS(comp.emplace<tag_byte>("def", 4).second, false);
        TS_ASSERT((comp == tag_compound{
            {"abc", tag_long(-28)},
            {"def", tag_byte(4)},
            {"ghi", tag_string("world")}
        }));
    }

    void test_value()
    {
        value val1;
        value val2(make_unique<tag_int>(42));
        value val3(tag_int(42));

        TS_ASSERT(!val1 && val2 && val3);
        TS_ASSERT(val1 == val1);
        TS_ASSERT(val1 != val2);
        TS_ASSERT(val2 == val3);
        TS_ASSERT(val3 == val3);

        value valstr(tag_string("foo"));
        TS_ASSERT_EQUALS(static_cast<std::string>(valstr), "foo");
        valstr = "bar";
        TS_ASSERT_THROWS(valstr = 5, std::bad_cast);
        TS_ASSERT_EQUALS(static_cast<std::string>(valstr), "bar");
        TS_ASSERT(valstr.as<tag_string>() == "bar");
        TS_ASSERT_EQUALS(&valstr.as<tag>(), &valstr.get());
        TS_ASSERT_THROWS(valstr.as<tag_float>(), std::bad_cast);

        val1 = int64_t(42);
        TS_ASSERT(val2 != val1);

        TS_ASSERT_THROWS(val2 = int64_t(12), std::bad_cast);
        TS_ASSERT_EQUALS(static_cast<int64_t>(val2), 42);
        tag_int* ptr = dynamic_cast<tag_int*>(val2.get_ptr().get());
        TS_ASSERT(*ptr == 42);
        val2 = 52;
        TS_ASSERT_EQUALS(static_cast<int32_t>(val2), 52);
        TS_ASSERT(*ptr == 52);

        TS_ASSERT_THROWS(val1["foo"], std::bad_cast);
        TS_ASSERT_THROWS(val1.at("foo"), std::bad_cast);

        val3 = 52;
        TS_ASSERT(val2 == val3);
        TS_ASSERT(val2.get_ptr() != val3.get_ptr());

        val3 = std::move(val2);
        TS_ASSERT(val3 == tag_int(52));
        TS_ASSERT(!val2);

        tag_int& tag = dynamic_cast<tag_int&>(val3.get());
        TS_ASSERT(tag == tag_int(52));
        tag = 21;
        TS_ASSERT_EQUALS(static_cast<int32_t>(val3), 21);
        val1.set_ptr(std::move(val3.get_ptr()));
        TS_ASSERT(val1.as<tag_int>() == 21);

        TS_ASSERT_EQUALS(val1.get_type(), tag_type::Int);
        TS_ASSERT_EQUALS(val2.get_type(), tag_type::Null);
        TS_ASSERT_EQUALS(val3.get_type(), tag_type::Null);

        val2 = val1;
        val1 = val3;
        TS_ASSERT(!val1 && val2 && !val3);
        TS_ASSERT(val1.get_ptr() == nullptr);
        TS_ASSERT(val2.get() == tag_int(21));
        TS_ASSERT(value(val1) == val1);
        TS_ASSERT(value(val2) == val2);
        val1 = val1;
        val2 = val2;
        TS_ASSERT(!val1);
        TS_ASSERT(val1 == value_initializer(nullptr));
        TS_ASSERT(val2 == tag_int(21));

        val3 = tag_short(2);
        TS_ASSERT_THROWS(val3 = tag_string("foo"), std::bad_cast);
        TS_ASSERT(val3.get() == tag_short(2));

        val2.set_ptr(make_unique<tag_string>("foo"));
        TS_ASSERT(val2 == tag_string("foo"));
    }

    void test_tag_list()
    {
        tag_list list;
        TS_ASSERT_EQUALS(list.el_type(), tag_type::Null);
        TS_ASSERT_THROWS(list.push_back(value(nullptr)), std::invalid_argument);

        list.emplace_back<tag_string>("foo");
        TS_ASSERT_EQUALS(list.el_type(), tag_type::String);
        list.push_back(tag_string("bar"));
        TS_ASSERT_THROWS(list.push_back(tag_int(42)), std::invalid_argument);
        TS_ASSERT_THROWS(list.emplace_back<tag_compound>(), std::invalid_argument);

        TS_ASSERT((list == tag_list{"foo", "bar"}));
        TS_ASSERT(list[0] == tag_string("foo"));
        TS_ASSERT_EQUALS(static_cast<std::string>(list.at(1)), "bar");

        TS_ASSERT_EQUALS(list.size(), 2u);
        TS_ASSERT_THROWS(list.at(2), std::out_of_range);
        TS_ASSERT_THROWS(list.at(-1), std::out_of_range);

        list.set(1, value(tag_string("baz")));
        TS_ASSERT_THROWS(list.set(1, value(nullptr)), std::invalid_argument);
        TS_ASSERT_THROWS(list.set(1, value(tag_int(-42))), std::invalid_argument);
        TS_ASSERT_EQUALS(static_cast<std::string>(list[1]), "baz");

        TS_ASSERT_EQUALS(list.size(), 2u);
        tag_string values[] = {"foo", "baz"};
        TS_ASSERT_EQUALS(list.end() - list.begin(), int(list.size()));
        TS_ASSERT(std::equal(list.begin(), list.end(), values));

        list.pop_back();
        TS_ASSERT(list == tag_list{"foo"});
        TS_ASSERT(list == tag_list::of<tag_string>({"foo"}));
        TS_ASSERT(tag_list::of<tag_string>({"foo"}) == tag_list{"foo"});
        TS_ASSERT((list != tag_list{2, 3, 5, 7}));

        list.clear();
        TS_ASSERT_EQUALS(list.size(), 0u);
        TS_ASSERT_EQUALS(list.el_type(), tag_type::String)
        TS_ASSERT_THROWS(list.push_back(tag_short(25)), std::invalid_argument);
        TS_ASSERT_THROWS(list.push_back(value(nullptr)), std::invalid_argument);

        list.reset();
        TS_ASSERT_EQUALS(list.el_type(), tag_type::Null);
        list.emplace_back<tag_int>(17);
        TS_ASSERT_EQUALS(list.el_type(), tag_type::Int);

        list.reset(tag_type::Float);
        TS_ASSERT_EQUALS(list.el_type(), tag_type::Float);
        list.emplace_back<tag_float>(17.0f);
        TS_ASSERT(list == tag_list({17.0f}));

        TS_ASSERT(tag_list() != tag_list(tag_type::Int));
        TS_ASSERT(tag_list() == tag_list());
        TS_ASSERT(tag_list(tag_type::Short) != tag_list(tag_type::Int));
        TS_ASSERT(tag_list(tag_type::Short) == tag_list(tag_type::Short));

        tag_list short_list = tag_list::of<tag_short>({25, 36});
        TS_ASSERT_EQUALS(short_list.el_type(), tag_type::Short);
        TS_ASSERT((short_list == tag_list{int16_t(25), int16_t(36)}));
        TS_ASSERT((short_list != tag_list{25, 36}));
        TS_ASSERT((short_list == tag_list{value(tag_short(25)), value(tag_short(36))}));

        TS_ASSERT_THROWS((tag_list{value(tag_byte(4)), value(tag_int(5))}), std::invalid_argument);
        TS_ASSERT_THROWS((tag_list{value(nullptr), value(tag_int(6))}), std::invalid_argument);
        TS_ASSERT_THROWS((tag_list{value(tag_int(7)), value(tag_int(8)), value(nullptr)}), std::invalid_argument);
        TS_ASSERT_EQUALS((tag_list(std::initializer_list<value>{})).el_type(), tag_type::Null);
        TS_ASSERT_EQUALS((tag_list{2, 3, 5, 7}).el_type(), tag_type::Int);
    }

    void test_tag_byte_array()
    {
        std::vector<int8_t> vec{1, 2, 127, -128};
        tag_byte_array arr{1, 2, 127, -128};
        TS_ASSERT_EQUALS(arr.size(), 4u);
        TS_ASSERT(arr.at(0) == 1 && arr[1] == 2 && arr[2] == 127 && arr.at(3) == -128);
        TS_ASSERT_THROWS(arr.at(-1), std::out_of_range);
        TS_ASSERT_THROWS(arr.at(4), std::out_of_range);

        TS_ASSERT(arr.get() == vec);
        TS_ASSERT(arr == tag_byte_array(std::vector<int8_t>(vec)));

        arr.push_back(42);
        vec.push_back(42);

        TS_ASSERT_EQUALS(arr.size(), 5u);
        TS_ASSERT_EQUALS(arr.end() - arr.begin(), int(arr.size()));
        TS_ASSERT(std::equal(arr.begin(), arr.end(), vec.begin()));

        arr.pop_back();
        arr.pop_back();
        TS_ASSERT_EQUALS(arr.size(), 3u);
        TS_ASSERT((arr == tag_byte_array{1, 2, 127}));
        TS_ASSERT((arr != tag_int_array{1, 2, 127}));
        TS_ASSERT((arr != tag_byte_array{1, 2, -1}));

        arr.clear();
        TS_ASSERT(arr == tag_byte_array());
    }

    void test_tag_int_array()
    {
        std::vector<int32_t> vec{100, 200, INT32_MAX, INT32_MIN};
        tag_int_array arr{100, 200, INT32_MAX, INT32_MIN};
        TS_ASSERT_EQUALS(arr.size(), 4u);
        TS_ASSERT(arr.at(0) == 100 && arr[1] == 200 && arr[2] == INT32_MAX && arr.at(3) == INT32_MIN);
        TS_ASSERT_THROWS(arr.at(-1), std::out_of_range);
        TS_ASSERT_THROWS(arr.at(4), std::out_of_range);

        TS_ASSERT(arr.get() == vec);
        TS_ASSERT(arr == tag_int_array(std::vector<int32_t>(vec)));

        arr.push_back(42);
        vec.push_back(42);

        TS_ASSERT_EQUALS(arr.size(), 5u);
        TS_ASSERT_EQUALS(arr.end() - arr.begin(), int(arr.size()));
        TS_ASSERT(std::equal(arr.begin(), arr.end(), vec.begin()));

        arr.pop_back();
        arr.pop_back();
        TS_ASSERT_EQUALS(arr.size(), 3u);
        TS_ASSERT((arr == tag_int_array{100, 200, INT32_MAX}));
        TS_ASSERT((arr != tag_int_array{100, -56, -1}));

        arr.clear();
        TS_ASSERT(arr == tag_int_array());
    }

    void test_visitor()
    {
        struct : public nbt_visitor
        {
            tag_type visited = tag_type::Null;

            void visit(tag_byte& tag)       { visited = tag_type::Byte; }
            void visit(tag_short& tag)      { visited = tag_type::Short; }
            void visit(tag_int& tag)        { visited = tag_type::Int; }
            void visit(tag_long& tag)       { visited = tag_type::Long; }
            void visit(tag_float& tag)      { visited = tag_type::Float; }
            void visit(tag_double& tag)     { visited = tag_type::Double; }
            void visit(tag_byte_array& tag) { visited = tag_type::Byte_Array; }
            void visit(tag_string& tag)     { visited = tag_type::String; }
            void visit(tag_list& tag)       { visited = tag_type::List; }
            void visit(tag_compound& tag)   { visited = tag_type::Compound; }
            void visit(tag_int_array& tag)  { visited = tag_type::Int_Array; }
        } v;

        tag_byte().accept(v);
        TS_ASSERT_EQUALS(v.visited, tag_type::Byte);
        tag_short().accept(v);
        TS_ASSERT_EQUALS(v.visited, tag_type::Short);
        tag_int().accept(v);
        TS_ASSERT_EQUALS(v.visited, tag_type::Int);
        tag_long().accept(v);
        TS_ASSERT_EQUALS(v.visited, tag_type::Long);
        tag_float().accept(v);
        TS_ASSERT_EQUALS(v.visited, tag_type::Float);
        tag_double().accept(v);
        TS_ASSERT_EQUALS(v.visited, tag_type::Double);
        tag_byte_array().accept(v);
        TS_ASSERT_EQUALS(v.visited, tag_type::Byte_Array);
        tag_string().accept(v);
        TS_ASSERT_EQUALS(v.visited, tag_type::String);
        tag_list().accept(v);
        TS_ASSERT_EQUALS(v.visited, tag_type::List);
        tag_compound().accept(v);
        TS_ASSERT_EQUALS(v.visited, tag_type::Compound);
        tag_int_array().accept(v);
        TS_ASSERT_EQUALS(v.visited, tag_type::Int_Array);
    }
};
