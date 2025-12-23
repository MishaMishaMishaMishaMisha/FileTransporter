#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>
#include <atomic>
#include <functional>
#include <vector>

#define XXH_INLINE_ALL
#include "xxhash.h"


namespace ft
{
    namespace filehash
    {
        uint64_t CalculateSimpleFileHash(const std::string& filename);
        extern std::atomic<bool> IsCancelCalculatingHash;
        extern std::function<void(const uint64_t)> onCalculatingProgress;
    }
}
