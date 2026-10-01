#include "world/WorldGeneration.h"

#include "mc/deps/nbt/CompoundTag.h"
#include "mc/world/level/GeneratorType.h"

namespace worlddl {

void setVoidGeneration(CompoundTag& tag) {
    // Replace inherited flat options in the exported tag with an explicit air-only world.
    tag.putInt("Generator", static_cast<int>(GeneratorType::Flat));
    tag.putString("FlatWorldLayers", VoidFlatWorldLayers);
}

} // namespace worlddl
