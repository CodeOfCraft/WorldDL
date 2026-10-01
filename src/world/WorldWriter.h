#pragma once

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

namespace leveldb { class DB; }

namespace worlddl {

struct Record {
    std::string key;
    std::string value;
};

struct ChunkSnapshot {
    std::string         identity;
    std::vector<Record> records;
};

struct WriterStatus {
    std::filesystem::path directory;
    std::size_t chunks{};
    std::size_t writes{};
    std::size_t pending{};
    std::size_t rejected{};
    std::string error;
};

// Only owned byte strings cross into the writer thread.
class WorldWriter {
public:
    WorldWriter(std::filesystem::path directory, std::string const& name, std::string const& metadata,
                std::size_t queueLimit = 256);
    ~WorldWriter();
    WorldWriter(WorldWriter const&) = delete;
    WorldWriter& operator=(WorldWriter const&) = delete;

    bool submit(ChunkSnapshot&& snapshot);
    bool flush();
    void stop();
    [[nodiscard]] WriterStatus status() const;

private:
    void run() noexcept;

    std::filesystem::path            mDirectory;
    std::unique_ptr<leveldb::DB>     mDatabase;
    std::size_t                     mQueueLimit;
    mutable std::mutex              mMutex;
    std::condition_variable         mReady;
    std::condition_variable         mDrained;
    std::deque<std::string>         mOrder;
    std::map<std::string, ChunkSnapshot> mPending;
    std::set<std::string>           mSaved;
    std::size_t                     mWrites{};
    std::size_t                     mRejected{};
    bool                            mBusy{};
    bool                            mStopping{};
    std::string                     mError;
    std::thread                     mThread;
};

} // namespace worlddl
