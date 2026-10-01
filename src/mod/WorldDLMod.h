#pragma once

#include <memory>

#include "ll/api/mod/NativeMod.h"

class Level;
class LevelChunk;

namespace worlddl {

class WorldDLMod {
public:
    static WorldDLMod& getInstance();
    WorldDLMod();
    ~WorldDLMod();

    [[nodiscard]] ll::mod::NativeMod& getSelf() const { return mSelf; }
    bool load();
    bool enable();
    bool disable();
    bool unload();

    void capture(LevelChunk& chunk, bool ensureQueued = false) noexcept;
    void leaving(Level& level) noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
    ll::mod::NativeMod& mSelf;
};

} // namespace worlddl
