//-----------------------------------------------------------------------------
// MurmurHash2 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.
//
// This was modified as to possibilitate it's usage incrementally.
// Those modifications are also placed in the public domain, and the author of
// such modifications hereby disclaims copyright to this source code.

#include "MurmurHash2.h"

namespace Murmur2 {

// 'm' and 'r' are mixing constants generated offline.
// They're not really 'magic', they just happen to work well.
const uint32_t m = 0x5bd1e995;
const int r = 24;

uint32_t hash(Reader* file_stream, std::size_t buffer_size, std::function<bool(char)> filter_out)
{
    auto* buffer = new char[buffer_size];
    char data[4];

    int read = 0;
    uint32_t size = 0;

    // We need the size without the filtered out characters before actually calculating the hash,
    // to setup the initial value for the hash.
    do {
        read = file_stream->read(buffer, buffer_size);
        for (int i = 0; i < read; i++) {
            if (!filter_out(buffer[i]))
                size += 1;
        }
    } while (!file_stream->eof());

    file_stream->goToBeginning();

    int index = 0;

    // This forces a seed of 1.
    IncrementalHashInfo info{ (uint32_t)1 ^ size, (uint32_t)size };
    do {
        read = file_stream->read(buffer, buffer_size);
        for (int i = 0; i < read; i++) {
            char c = buffer[i];

            if (filter_out(c))
                continue;

            data[index] = c;
            index = (index + 1) % 4;

            // Mix 4 bytes at a time into the hash
            if (index == 0)
                FourBytes_MurmurHash2(reinterpret_cast<unsigned char*>(&data), info);
        }
    } while (!file_stream->eof());

    // Do one last bit shuffle in the hash
    FourBytes_MurmurHash2(reinterpret_cast<unsigned char*>(&data), info);

    delete[] buffer;

    return info.h;
}

void FourBytes_MurmurHash2(const unsigned char* data, IncrementalHashInfo& prev)
{
    if (prev.len >= 4) {
        // Not the final mix
        uint32_t k = *reinterpret_cast<const uint32_t*>(data);

        k *= m;
        k ^= k >> r;
        k *= m;

        prev.h *= m;
        prev.h ^= k;

        prev.len -= 4;
    } else {
        // The final mix

        // Handle the last few bytes of the input array
        switch (prev.len) {
            case 3:
                prev.h ^= data[2] << 16;
                /* fall through */
            case 2:
                prev.h ^= data[1] << 8;
                /* fall through */
            case 1:
                prev.h ^= data[0];
                prev.h *= m;
        };

        // Do a few final mixes of the hash to ensure the last few
        // bytes are well-incorporated.

        prev.h ^= prev.h >> 13;
        prev.h *= m;
        prev.h ^= prev.h >> 15;

        prev.len = 0;
    }
}

}  // namespace Murmur2