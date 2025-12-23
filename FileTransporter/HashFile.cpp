#include "HashFile.h"

std::function<void(const uint64_t)> ft::filehash::onCalculatingProgress = nullptr;

std::atomic<bool> ft::filehash::IsCancelCalculatingHash = false;

uint64_t ft::filehash::CalculateSimpleFileHash(const std::string& filename)
{
    IsCancelCalculatingHash.store(false);

    std::ifstream in(filename, std::ios::binary);
    if (!in)
    {
        std::cerr << "Cannot open file: " << filename << std::endl;
        return 0;
    }

    constexpr size_t BUFFER_SIZE = 1 << 20; // 1 MB
    std::vector<char> buffer(BUFFER_SIZE);

    // create state for XXH3
    XXH3_state_t* state = XXH3_createState();
    if (!state)
    {
        return 0;
    }

    XXH3_64bits_reset(state);

    // filesize for calculating progress
    in.seekg(0, std::ios::end);
    uint64_t totalBytes = static_cast<uint64_t>(in.tellg());
    in.seekg(0, std::ios::beg);

    uint64_t progress = 0;

    while (!IsCancelCalculatingHash.load())
    {
        in.read(buffer.data(), buffer.size());
        std::streamsize count = in.gcount();
        if (count <= 0)
        {
            break;
        }

        XXH3_64bits_update(state, buffer.data(), static_cast<size_t>(count));

        progress += static_cast<uint64_t>(count);

        // update progress every 10 ла
        if (onCalculatingProgress && (progress % (10ull << 20)) == 0)
        {
            onCalculatingProgress(progress * 100 / totalBytes);
        }
    }

    uint64_t hash = XXH3_64bits_digest(state);
    XXH3_freeState(state);

    return hash;
}

