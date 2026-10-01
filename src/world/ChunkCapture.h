#pragma once

#include <optional>
#include <string>

#include "world/WorldWriter.h"

class Level;
class LevelChunk;
class LocalPlayer;

namespace worlddl {

std::optional<ChunkSnapshot> captureChunk(LevelChunk& chunk);
std::string captureMetadata(Level& level, LocalPlayer const& player, std::string const& name);
std::size_t snapshotHash(ChunkSnapshot const& snapshot);

} // namespace worlddl
