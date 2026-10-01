#pragma once

#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace worlddl {

enum class ChunkTag : unsigned char {
    Data3D = 0x2b,
    Version = 0x2c,
    Data2D = 0x2d,
    SubChunk = 0x2f,
    BlockEntities = 0x31,
    FinalizedState = 0x36,
};

inline void appendInt32(std::string& output, std::uint32_t value) {
    for (unsigned int shift = 0; shift < 32; shift += 8) {
        output.push_back(static_cast<char>((value >> shift) & 0xff));
    }
}

inline std::string chunkKey(
    int x, int z, int dimension, ChunkTag tag, std::optional<std::int8_t> subChunk = std::nullopt
) {
    std::string key;
    appendInt32(key, static_cast<std::uint32_t>(x));
    appendInt32(key, static_cast<std::uint32_t>(z));
    if (dimension != 0) {
        appendInt32(key, static_cast<std::uint32_t>(dimension));
    }
    key.push_back(static_cast<char>(tag));
    if (subChunk) {
        key.push_back(static_cast<char>(*subChunk));
    }
    return key;
}

inline std::string levelDat(std::string_view littleEndianNbt) {
    if (littleEndianNbt.size() > std::numeric_limits<std::uint32_t>::max()) {
        throw std::length_error("level.dat exceeds the Bedrock size limit");
    }
    std::string result;
    appendInt32(result, 10);
    appendInt32(result, static_cast<std::uint32_t>(littleEndianNbt.size()));
    result.append(littleEndianNbt);
    return result;
}

} // namespace worlddl
