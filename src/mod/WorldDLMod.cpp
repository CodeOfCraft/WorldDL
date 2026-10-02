#include "mod/WorldDLMod.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <mutex>
#include <unordered_map>

#include "ll/api/command/CommandHandle.h"
#include "ll/api/command/CommandRegistrar.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/command/ClientCommandRegisterEvent.h"
#include "ll/api/event/world/ClientLevelTickEvent.h"
#include "ll/api/memory/Hook.h"
#include "ll/api/mod/RegisterHelper.h"
#include "ll/api/service/Bedrock.h"

#include "mc/client/game/ClientInstance.h"
#include "mc/client/multiplayer/ClientLevel.h"
#include "mc/client/player/LocalPlayer.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/chunk/ChunkSource.h"
#include "mc/world/level/chunk/LevelChunk.h"

#include "world/ChunkCapture.h"
#include "world/CaptureQueue.h"

namespace worlddl {

LL_TYPE_INSTANCE_HOOK(WdlSubChunkHook, ll::memory::HookPriority::Normal, ClientLevel,
                      &ClientLevel::$onSubChunkLoaded, void, ChunkSource& source, LevelChunk& chunk,
                      short index, bool visibilityChanged) {
    origin(source, chunk, index, visibilityChanged);
    WorldDLMod::getInstance().schedule(source, chunk);
}

LL_TYPE_INSTANCE_HOOK(WdlChunkDestroyHook, ll::memory::HookPriority::Normal, LevelChunk,
                      &LevelChunk::$dtor, void) {
    WorldDLMod::getInstance().capture(*this, true);
    origin();
}

LL_TYPE_INSTANCE_HOOK(WdlLeaveHook, ll::memory::HookPriority::Normal, ClientLevel,
                      &ClientLevel::$startLeaveGame, void) {
    WorldDLMod::getInstance().leaving(*this);
    origin();
}

struct WorldDLMod::Impl {
    WorldDLMod& mod;
    std::recursive_mutex mutex;
    std::unique_ptr<WorldWriter> writer;
    WriterStatus lastStatus;
    Level* activeLevel{};
    bool enabled{};
    bool captureErrorReported{};
    int ticks{};
    int scanIntervalTicks{20};
    int chunksPerTick{4};
    std::chrono::microseconds captureBudget{2000};
    std::size_t queueLimit{256};
    std::unordered_map<std::string, std::size_t> hashes;
    CaptureQueue<LevelChunk> loaded;
    CaptureQueue<LevelChunk> scan;
    ll::event::ListenerPtr commandListener;
    ll::event::ListenerPtr tickListener;
    std::unique_ptr<ll::memory::HookRegistrar<WdlSubChunkHook, WdlChunkDestroyHook, WdlLeaveHook>> hooks;

    explicit Impl(WorldDLMod& owner) : mod(owner) {}

    void discover() {
        auto client = ll::service::getClientInstance();
        if (!client || client->getLevel() != activeLevel || !client->getRegion()) {
            return;
        }
        auto* source = &client->getRegion()->getChunkSource();
        for (int depth = 0; source && depth < 16; ++depth, source = source->mParent) {
            if (auto* chunks = source->getChunkMap()) {
                for (auto const& [position, chunk] : *chunks) {
                    scan.push(chunk.lock());
                }
                break;
            }
        }
    }

    void captureAll() {
        scan.clear();
        discover();
        while (!loaded.empty()) {
            if (auto chunk = loaded.pop()) {
                scan.erase(chunk.get());
                mod.capture(*chunk, true);
            }
        }
        loaded.clear();
        while (!scan.empty()) {
            if (auto chunk = scan.pop()) {
                mod.capture(*chunk, true);
            }
        }
    }

    void stop() {
        if (writer) {
            writer->stop();
            lastStatus = writer->status();
            mod.getSelf().getLogger().info("WorldDL saved {} chunks to {}",
                                           lastStatus.chunks, lastStatus.directory.string());
            if (!lastStatus.error.empty()) {
                mod.getSelf().getLogger().error("WorldDL write failed: {}", lastStatus.error);
            }
            writer.reset();
        }
        activeLevel = nullptr;
        hashes.clear();
        loaded.clear();
        scan.clear();
    }

    void start(std::string name, CommandOutput& output) {
        std::lock_guard lock(mutex);
        if (!enabled) {
            output.error("WorldDL is disabled.");
            return;
        }
        if (writer) {
            output.error("A download is already running. Use /wdl stop first.");
            return;
        }
        auto client = ll::service::getClientInstance();
        if (!client || !client->getLevel() || !client->getLocalPlayer() || !client->getRegion()) {
            output.error("Join a world before starting a download.");
            return;
        }
        if (name.empty()) {
            name = "WorldDL";
        }
        try {
            auto const root = mod.getSelf().getDataDir() / "worlds";
            std::filesystem::create_directories(root);
            auto const timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            auto directory = root / ("wdl_" + std::to_string(timestamp));
            for (unsigned int suffix = 1; std::filesystem::exists(directory); ++suffix) {
                directory = root / ("wdl_" + std::to_string(timestamp) + "_" + std::to_string(suffix));
            }
            auto metadata = captureMetadata(*client->getLevel(), *client->getLocalPlayer(), name);
            writer = std::make_unique<WorldWriter>(directory, name, metadata, queueLimit);
            activeLevel = client->getLevel();
            hashes.clear();
            loaded.clear();
            scan.clear();
            captureErrorReported = false;
            ticks = scanIntervalTicks - 1;
            discover();
            output.success("WorldDL started: " + directory.string());
        } catch (std::exception const& error) {
            stop();
            output.error(std::string("Cannot start WorldDL: ") + error.what());
        }
    }

    void showStatus(CommandOutput& output) {
        std::lock_guard lock(mutex);
        auto const state = writer ? writer->status() : lastStatus;
        output.success(std::string(writer ? "Recording" : "Stopped") + ": "
                       + std::to_string(state.chunks) + " chunks, " + std::to_string(state.pending)
                       + " queued, " + std::to_string(state.rejected) + " queue rejections, "
                       + std::to_string(state.writes) + " writes in " + std::to_string(state.batches) + " batches.\n"
                       + state.directory.string());
        if (!state.error.empty()) {
            output.error("Storage error: " + state.error);
        }
    }

    void registerCommands() {
        auto& command = ll::command::CommandRegistrar::getClientInstance().getOrCreateCommand(
            "wdl", "WorldDL", CommandPermissionLevel::Any);
        command.overload().execute([this](CommandOrigin const&, CommandOutput& output) { showStatus(output); });
        command.overload().text("status").execute(
            [this](CommandOrigin const&, CommandOutput& output) { showStatus(output); });
        command.overload().text("start").execute(
            [this](CommandOrigin const&, CommandOutput& output) { start({}, output); });
        struct StartParams { std::string name; };
        command.overload<StartParams>().text("start").required("name").execute(
            [this](CommandOrigin const&, CommandOutput& output, StartParams const& params) {
                start(params.name, output);
            });
        command.overload().text("save").execute([this](CommandOrigin const&, CommandOutput& output) {
            std::lock_guard lock(mutex);
            if (!writer) {
                output.error("No download is running.");
                return;
            }
            captureAll();
            if (writer->flush()) {
                output.success("WorldDL saved: " + writer->status().directory.string());
            } else {
                output.error("Storage error: " + writer->status().error);
            }
        });
        command.overload().text("stop").execute([this](CommandOrigin const&, CommandOutput& output) {
            std::lock_guard lock(mutex);
            if (!writer) {
                output.error("No download is running.");
                return;
            }
            captureAll();
            stop();
            if (lastStatus.error.empty()) {
                output.success("WorldDL stopped: " + lastStatus.directory.string());
            } else {
                output.error("Storage error: " + lastStatus.error);
            }
        });
    }

    void tick(Level& level) {
        std::lock_guard lock(mutex);
        if (!writer || !enabled) {
            return;
        }
        if (&level != activeLevel) {
            stop();
            return;
        }
        auto const state = writer->status();
        if (!state.error.empty()) {
            stop();
            return;
        }
        if (++ticks >= scanIntervalTicks) {
            ticks = 0;
            if (scan.empty()) {
                discover();
            }
        }
        if (state.pending >= queueLimit) {
            return;
        }
        auto const deadline = std::chrono::steady_clock::now() + captureBudget;
        for (int count = 0; count < chunksPerTick && (!loaded.empty() || !scan.empty()); ++count) {
            if (std::chrono::steady_clock::now() >= deadline) {
                break;
            }
            auto chunk = loaded.empty() ? scan.pop() : loaded.pop();
            if (chunk) {
                scan.erase(chunk.get());
                mod.capture(*chunk);
            }
        }
    }
};

WorldDLMod& WorldDLMod::getInstance() {
    static WorldDLMod instance;
    return instance;
}

WorldDLMod::WorldDLMod()
: mImpl(std::make_unique<Impl>(*this)), mSelf(*ll::mod::NativeMod::current()) {}

WorldDLMod::~WorldDLMod() = default;

bool WorldDLMod::load() {
    getSelf().getLogger().info("WorldDL 0.1.0 - LeviLamina 26.51.6 client");
    return true;
}

bool WorldDLMod::enable() {
    std::lock_guard lock(mImpl->mutex);
    if (mImpl->enabled) {
        return true;
    }
    mImpl->enabled = true;
    auto& bus = ll::event::EventBus::getInstance();
    mImpl->commandListener = bus.emplaceListener<ll::event::ClientCommandRegisterEvent>(
        [this](auto&) { mImpl->registerCommands(); });
    mImpl->tickListener = bus.emplaceListener<ll::event::ClientLevelTickEvent>(
        [this](auto& event) { mImpl->tick(event.level()); });
    mImpl->hooks = std::make_unique<ll::memory::HookRegistrar<WdlSubChunkHook, WdlChunkDestroyHook, WdlLeaveHook>>();
    if (ll::service::getCommandRegistry(true)) {
        mImpl->registerCommands();
    }
    return true;
}

bool WorldDLMod::disable() {
    auto& bus = ll::event::EventBus::getInstance();
    if (mImpl->commandListener) {
        bus.removeListener(mImpl->commandListener);
        mImpl->commandListener.reset();
    }
    if (mImpl->tickListener) {
        bus.removeListener(mImpl->tickListener);
        mImpl->tickListener.reset();
    }
    mImpl->hooks.reset();
    std::lock_guard lock(mImpl->mutex);
    mImpl->enabled = false;
    mImpl->stop();
    return true;
}

bool WorldDLMod::unload() { return disable(); }

void WorldDLMod::schedule(ChunkSource& source, LevelChunk& chunk) noexcept {
    std::lock_guard lock(mImpl->mutex);
    if (!mImpl->enabled || !mImpl->writer || &chunk.mLevel != mImpl->activeLevel) {
        return;
    }
    try {
        auto owner = source.getExistingChunk(chunk.mPosition);
        if (owner && owner.get() == &chunk) {
            mImpl->loaded.push(owner);
            mImpl->scan.erase(&chunk);
        } else {
            capture(chunk);
        }
    } catch (...) {
        capture(chunk);
    }
}

void WorldDLMod::capture(LevelChunk& chunk, bool ensureQueued) noexcept {
    std::lock_guard lock(mImpl->mutex);
    if (!mImpl->enabled || !mImpl->writer || &chunk.mLevel != mImpl->activeLevel) {
        return;
    }
    if (ensureQueued) {
        mImpl->loaded.erase(&chunk);
        mImpl->scan.erase(&chunk);
    }
    try {
        if (auto snapshot = captureChunk(chunk)) {
            auto const hash = snapshotHash(*snapshot);
            auto const found = mImpl->hashes.find(snapshot->identity);
            if (found != mImpl->hashes.end() && found->second == hash) {
                return;
            }
            auto identity = snapshot->identity;
            bool accepted = mImpl->writer->submit(std::move(*snapshot));
            if (!accepted && ensureQueued && mImpl->writer->flush()) {
                accepted = mImpl->writer->submit(std::move(*snapshot));
            }
            if (accepted) {
                mImpl->hashes[std::move(identity)] = hash;
            }
        }
    } catch (std::exception const& error) {
        if (!mImpl->captureErrorReported) {
            getSelf().getLogger().error("WorldDL capture failed: {}", error.what());
            mImpl->captureErrorReported = true;
        }
    } catch (...) {
        if (!mImpl->captureErrorReported) {
            getSelf().getLogger().error("WorldDL capture failed with an unknown exception");
            mImpl->captureErrorReported = true;
        }
    }
}

void WorldDLMod::leaving(Level& level) noexcept {
    std::lock_guard lock(mImpl->mutex);
    if (mImpl->writer && &level == mImpl->activeLevel) {
        try {
            mImpl->captureAll();
        } catch (std::exception const& error) {
            getSelf().getLogger().error("WorldDL final capture failed: {}", error.what());
        } catch (...) {
            getSelf().getLogger().error("WorldDL final capture failed");
        }
        mImpl->stop();
    }
}

} // namespace worlddl

LL_REGISTER_MOD(worlddl::WorldDLMod, worlddl::WorldDLMod::getInstance());
