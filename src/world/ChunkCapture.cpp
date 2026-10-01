#include "world/ChunkCapture.h"

#include <cmath>
#include <functional>
#include <shared_mutex>

#include "world/BedrockFormat.h"
#include "world/WorldGeneration.h"

#include "mc/client/player/LocalPlayer.h"
#include "mc/deps/nbt/CompoundTag.h"
#include "mc/util/StringByteOutput.h"
#include "mc/world/item/SaveContext.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/biome/Biome.h"
#include "mc/world/level/chunk/LevelChunk.h"
#include "mc/world/level/chunk/SubChunk.h"
#include "mc/world/level/dimension/Dimension.h"
#include "mc/world/level/storage/LevelData.h"
#include "mc/world/level/storage/StorageVersion.h"

namespace worlddl {

std::optional<ChunkSnapshot> captureChunk(LevelChunk& chunk) {
    if (chunk.mIsEmptyClientChunk || chunk.mIsBeingMoved->load()) {
        return std::nullopt;
    }
    auto const& position = chunk.mPosition.get();
    auto const dimension = static_cast<int>(chunk.mDimension.getDimensionId());
    ChunkSnapshot snapshot{chunkKey(position.x, position.z, dimension, ChunkTag::Version), {}};
    auto add = [&](ChunkTag tag, std::string data, std::optional<std::int8_t> index = std::nullopt) {
        snapshot.records.push_back({chunkKey(position.x, position.z, dimension, tag, index), std::move(data)});
    };

    for (auto const& subChunk : *chunk.mSubChunks) {
        auto const state = subChunk.mSubChunkState;
        if (!subChunk.mIsInitialized || subChunk.isPlaceHolderSubChunk()
            || (state != SubChunk::SubChunkState::Normal
                && state != SubChunk::SubChunkState::ProcessedSubChunk
                && state != SubChunk::SubChunkState::RequestFinished)) {
            continue;
        }
        std::string data;
        StringByteOutput stream(data);
        subChunk.serialize(stream, false);
        add(ChunkTag::SubChunk, std::move(data), static_cast<std::int8_t>(subChunk.mAbsoluteIndex));
    }
    if (snapshot.records.empty()) {
        return std::nullopt;
    }

    add(ChunkTag::Version, std::string(1, static_cast<char>(LevelChunkFormat::V1_21_120)));
    std::string finalized;
    appendInt32(finalized, 2);
    add(ChunkTag::FinalizedState, std::move(finalized));

    std::string column;
    StringByteOutput columnStream(column);
    for (auto const& height : *chunk.mHeightmap) {
        columnStream.writeShort(height.mVal);
    }
    {
        std::shared_lock lock(chunk.mBiomes->mBiomesMutex.get());
        if (chunk.mUse3DBiomeMaps) {
            // This engine serializer writes numeric biome palettes in little endian form.
            chunk.serializeBiomes(columnStream);
            add(ChunkTag::Data3D, std::move(column));
        } else {
            for (auto const& biome : *chunk.mBiomes->m2DBiomes) {
                columnStream.writeByte(static_cast<char>(biome.mValue));
            }
            add(ChunkTag::Data2D, std::move(column));
        }
    }

    std::string blockEntities;
    StringByteOutput entityStream(blockEntities);
    SaveContext const context{SaveContext::SaveUseCase::SaveToDisk};
    chunk.serializeBlockEntities(entityStream, context);
    add(ChunkTag::BlockEntities, std::move(blockEntities));
    return snapshot;
}

std::string captureMetadata(Level& level, LocalPlayer const& player, std::string const& name) {
    auto tag = level.getLevelData().createTag();
    tag->putInt("StorageVersion", static_cast<int>(StorageVersion::LevelDataStrictSize));
    tag->putString("LevelName", name);
    tag->putBoolean("MultiplayerGame", false);
    tag->putBoolean("LANBroadcast", false);
    tag->putBoolean("commandsEnabled", true);
    tag->putBoolean("hasBeenLoadedInCreative", true);
    tag->putInt("GameType", 1);
    tag->putInt("XBLBroadcastIntent", 0);
    tag->putInt("PlatformBroadcastIntent", 0);
    tag->putBoolean("isFromWorldTemplate", false);
    tag->putBoolean("isWorldTemplateOptionLocked", false);
    tag->putBoolean("isRandomSeedAllowed", false);
    setVoidGeneration(*tag);
    auto const& position = player.getPosition();
    // A global spawn is always in the overworld, even if capture begins in another dimension.
    if (static_cast<int>(player.getDimension().getDimensionId()) == 0) {
        tag->putInt("SpawnX", static_cast<int>(std::floor(position.x)));
        tag->putInt("SpawnY", static_cast<int>(std::floor(position.y)));
        tag->putInt("SpawnZ", static_cast<int>(std::floor(position.z)));
    }
    return levelDat(tag->toBinaryNbt());
}

std::size_t snapshotHash(ChunkSnapshot const& snapshot) {
    std::size_t result = 0;
    for (auto const& record : snapshot.records) {
        for (auto const* text : {&record.key, &record.value}) {
            auto const hash = std::hash<std::string>{}(*text);
            result ^= hash + static_cast<std::size_t>(0x9e3779b9) + (result << 6) + (result >> 2);
        }
    }
    return result;
}

} // namespace worlddl
