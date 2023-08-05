//-----------------------------------------------------------------------------
// The original MurmurHash2 was written by Austin Appleby, and is placed in the
// public domain. The author hereby disclaims copyright to this source code.
//
// This was modified as to possibilitate it's usage incrementally.
// Those modifications are also placed in the public domain, and the author of
// such modifications hereby disclaims copyright to this source code.

#pragma once

#include <cstdint>
#include <fstream>

#include <functional>

//-----------------------------------------------------------------------------

#define KiB 1024
#define MiB 1024 * KiB

uint32_t MurmurHash2(
    std::ifstream&& file_stream,
    std::size_t buffer_size = 4 * MiB,
    std::function<bool(char)> filter_out = [](char) { return false; });

struct IncrementalHashInfo {
    uint32_t h;
    uint32_t len;
};

void FourBytes_MurmurHash2(const unsigned char* data, IncrementalHashInfo& prev);

//-----------------------------------------------------------------------------
