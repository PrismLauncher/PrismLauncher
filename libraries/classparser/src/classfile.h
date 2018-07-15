#pragma once
#include "membuffer.h"
#include "constants.h"
#include "annotations.h"
#include <map>
namespace java
{
/**
 * Class representing a Java .class file
 */
class classfile : public util::membuffer
{
public:
    classfile(char *data, std::size_t size) : membuffer(data, size)
    {
        valid = false;
        is_synthetic = false;
        read_be(magic);
        if (magic != 0xCAFEBABE)
            throw new classfile_exception();
        read_be(minor_version);
        read_be(major_version);
        constants.load(*this);
        read_be(access_flags);
        read_be(this_class);
        read_be(super_class);

        // Interfaces
        uint16_t iface_count = 0;
        read_be(iface_count);
        while (iface_count)
        {
            uint16_t iface;
            read_be(iface);
            interfaces.push_back(iface);
            iface_count--;
        }

        // Fields
        // read fields (and attributes from inside fields) (and possible inner classes. yay for
        // recursion!)
        // for now though, we will ignore all attributes
        /*
         * field_info
         * {
         *     u2 access_flags;
         *     u2 name_index;
         *     u2 descriptor_index;
         *     u2 attributes_count;
         *     attribute_info attributes[attributes_count];
         * }
         */
        uint16_t field_count = 0;
        read_be(field_count);
        while (field_count)
        {
            // skip field stuff
            skip(6);
            // and skip field attributes
            uint16_t attr_count = 0;
            read_be(attr_count);
            while (attr_count)
            {
                skip(2);
                uint32_t attr_length = 0;
                read_be(attr_length);
                skip(attr_length);
                attr_count--;
            }
            field_count--;
        }

        // class methods
        /*
         * method_info
         * {
         *     u2 access_flags;
         *     u2 name_index;
         *     u2 descriptor_index;
         *     u2 attributes_count;
         *     attribute_info attributes[attributes_count];
         * }
         */
        uint16_t method_count = 0;
        read_be(method_count);
        while (method_count)
        {
            skip(6);
            // and skip method attributes
            uint16_t attr_count = 0;
            read_be(attr_count);
            while (attr_count)
            {
                skip(2);
                uint32_t attr_length = 0;
                read_be(attr_length);
                skip(attr_length);
                attr_count--;
            }
            method_count--;
        }

        // class attributes
        // there are many kinds of attributes. this is just the generic wrapper structure.
        // type is decided by attribute name. extensions to the standard are *possible*
        // class annotations are one kind of a attribute (one per class)
        /*
         * attribute_info
         * {
         *     u2 attribute_name_index;
         *     u4 attribute_length;
         *     u1 info[attribute_length];
         * }
         */
        uint16_t class_attr_count = 0;
        read_be(class_attr_count);
        while (class_attr_count)
        {
            uint16_t name_idx = 0;
            read_be(name_idx);
            uint32_t attr_length = 0;
            read_be(attr_length);

            auto name = constants[name_idx];
            if (name.str_data == "RuntimeVisibleAnnotations")
            {
                uint16_t num_annotations = 0;
                read_be(num_annotations);
                while (num_annotations)
                {
                    visible_class_annotations.push_back(annotation::read(*this, constants));
                    num_annotations--;
                }
            }
            else
                skip(attr_length);
            class_attr_count--;
        }
        valid = true;
    }
    ;
    bool valid;
    bool is_synthetic;
    uint32_t magic;
    uint16_t minor_version;
    uint16_t major_version;
    constant_pool constants;
    uint16_t access_flags;
    uint16_t this_class;
    uint16_t super_class;
    // interfaces this class implements ? must be. investigate.
    std::vector<uint16_t> interfaces;
    // FIXME: doesn't free up memory on delete
    java::annotation_table visible_class_annotations;
};
}