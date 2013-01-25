#pragma once
#include <stdint.h>
#include <string>
#include <vector>
#include <exception>
#include "endian.h"

namespace util
{
	class membuffer
	{
	public:
		membuffer(char * buffer, std::size_t size)
		{
			current = start = buffer;
			end = start + size;
		}
		~membuffer()
		{
			// maybe? possibly? left out to avoid confusion. for now.
			//delete start;
		}
		/**
			* Read some value. That's all ;)
			*/
		template <class T>
		void read(T& val)
		{
			val = *(T *)current;
			current += sizeof(T);
		}
		/**
			* Read a big-endian number
			* valid for 2-byte, 4-byte and 8-byte variables
			*/
		template <class T>
		void read_be(T& val)
		{
			val = util::bigswap(*(T *)current);
			current += sizeof(T);
		}
		/**
			* Read a string in the format:
			* 2B length (big endian, unsigned)
			* length bytes data
			*/
		void read_jstr(std::string & str)
		{
			uint16_t length = 0;
			read_be(length);
			str.append(current,length);
			current += length;
		}
		/**
			* Skip N bytes
			*/
		void skip (std::size_t N)
		{
			current += N;
		}
	private:
		char * start, *end, *current;
	};
}
