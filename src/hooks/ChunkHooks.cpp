#include "hooks/ChunkHooks.h"

#include <memory>

#include "ll/api/memory/Hook.h"

#include "mc/client/multiplayer/ClientLevel.h"
#include "mc/world/level/chunk/ChunkSource.h"
#include "mc/world/level/chunk/LevelChunk.h"

#include "mod/WorldDLMod.h"

namespace worlddl::hooks {
namespace {

LL_TYPE_INSTANCE_HOOK(
    WdlSubChunkHook,
    ll::memory::HookPriority::Normal,
    ClientLevel,
    &ClientLevel::$onSubChunkLoaded,
    void,
    ChunkSource& source,
    LevelChunk&  chunk,
    short        index,
    bool         visibilityChanged
) {
    origin(source, chunk, index, visibilityChanged);
    WorldDLMod::getInstance().schedule(source, chunk);
}

LL_TYPE_INSTANCE_HOOK(WdlChunkDestroyHook, ll::memory::HookPriority::Normal, LevelChunk, &LevelChunk::$dtor, void) {
    WorldDLMod::getInstance().capture(*this, true);
    origin();
}

LL_TYPE_INSTANCE_HOOK(
    WdlLeaveHook,
    ll::memory::HookPriority::Normal,
    ClientLevel,
    &ClientLevel::$startLeaveGame,
    void
) {
    WorldDLMod::getInstance().leaving(*this);
    origin();
}

using Registration = ll::memory::HookRegistrar<WdlSubChunkHook, WdlChunkDestroyHook, WdlLeaveHook>;

std::unique_ptr<Registration> registration;

} // namespace

void install() {
    if (!registration) {
        registration = std::make_unique<Registration>();
    }
}

void uninstall() { registration.reset(); }

} // namespace worlddl::hooks
