#pragma once

class CompoundTag;

namespace worlddl {

// A valid air layer avoids the default solid layers used for missing/empty flat options.
inline constexpr char VoidFlatWorldLayers[] = R"({"biome_id":1,"block_layers":[{"block_data":0,"block_name":"minecraft:air","count":1}],"encoding_version":4,"structure_options":null})";

void setVoidGeneration(CompoundTag& tag);

} // namespace worlddl
