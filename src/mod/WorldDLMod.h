#pragma once

#include <memory>
#include <string>

#include "ll/api/mod/NativeMod.h"

class ChunkSource;
class CommandOutput;
class Level;
class LevelChunk;

namespace worlddl {

class WorldDLMod {
public:
    static WorldDLMod& getInstance();
    WorldDLMod();
    ~WorldDLMod();

    [[nodiscard]] ll::mod::NativeMod& getSelf() const { return mSelf; }
    bool                              load();
    bool                              enable();
    bool                              disable();
    bool                              unload();

    // Capture entry points, called by hooks/ChunkHooks.cpp.
    void capture(LevelChunk& chunk, bool ensureQueued = false) noexcept;
    void schedule(ChunkSource& source, LevelChunk& chunk) noexcept;
    void leaving(Level& level) noexcept;

    // Session commands, called by command/CommandHandler.cpp.
    void startSession(std::string name, CommandOutput& output);
    void showStatus(CommandOutput& output);
    void saveSession(CommandOutput& output);
    void stopSession(CommandOutput& output);

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
    ll::mod::NativeMod&   mSelf;
};

} // namespace worlddl
