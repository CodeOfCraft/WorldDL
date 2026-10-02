#include "world/WorldWriter.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <stdexcept>

#include <leveldb/db.h>
#include <leveldb/write_batch.h>

namespace worlddl {
namespace {

constexpr std::size_t writeBatchLimit = 32;
constexpr std::size_t writeBatchBytes = 4 * 1024 * 1024;
constexpr auto writeBatchDelay = std::chrono::milliseconds{2};

void writeFile(std::filesystem::path const& path, std::string const& data) {
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    stream.exceptions(std::ios::failbit | std::ios::badbit);
    stream.write(data.data(), static_cast<std::streamsize>(data.size()));
    stream.close();
}

} // namespace

WorldWriter::WorldWriter(std::filesystem::path directory, std::string const& name,
                         std::string const& metadata, std::size_t queueLimit)
: mDirectory(std::move(directory)), mQueueLimit(queueLimit) {
    if (queueLimit == 0) {
        throw std::invalid_argument("queueLimit must be positive");
    }
    // A fresh directory prevents mixing two unrelated remote worlds.
    if (!std::filesystem::create_directory(mDirectory)) {
        throw std::runtime_error("Export directory already exists");
    }
    writeFile(mDirectory / "level.dat", metadata);
    writeFile(mDirectory / "level.dat_old", metadata);
    writeFile(mDirectory / "levelname.txt", name);
    leveldb::Options options;
    options.create_if_missing = true;
    // Bedrock understands standard uncompressed tables. Standard Snappy tables are incompatible.
    options.compression = leveldb::kNoCompression;
    leveldb::DB* database = nullptr;
    auto const result = leveldb::DB::Open(options, (mDirectory / "db").string(), &database);
    if (!result.ok()) {
        throw std::runtime_error(result.ToString());
    }
    mDatabase.reset(database);
    mThread = std::thread(&WorldWriter::run, this);
}

WorldWriter::~WorldWriter() { stop(); }

bool WorldWriter::submit(ChunkSnapshot&& snapshot) {
    std::lock_guard lock(mMutex);
    if (mStopping || !mError.empty()) {
        return false;
    }
    if (auto existing = mPending.find(snapshot.identity); existing != mPending.end()) {
        // A later partial column must not erase a previously queued subchunk.
        auto& records = existing->second.records;
        for (auto& incoming : snapshot.records) {
            auto found = std::find_if(records.begin(), records.end(), [&](Record const& record) {
                return record.key == incoming.key;
            });
            if (found == records.end()) {
                records.push_back(std::move(incoming));
            } else {
                found->value = std::move(incoming.value);
            }
        }
        return true;
    }
    if (mPending.size() >= mQueueLimit) {
        ++mRejected;
        return false;
    }
    auto identity = snapshot.identity;
    mOrder.push_back(identity);
    mPending.emplace(std::move(identity), std::move(snapshot));
    mReady.notify_one();
    return true;
}

bool WorldWriter::flush() {
    std::unique_lock lock(mMutex);
    mDrained.wait(lock, [&] { return (mPending.empty() && mInFlight == 0) || !mError.empty(); });
    return mError.empty();
}

void WorldWriter::stop() {
    {
        std::lock_guard lock(mMutex);
        mStopping = true;
    }
    mReady.notify_one();
    if (mThread.joinable()) {
        mThread.join();
    }
    mDatabase.reset();
}

WriterStatus WorldWriter::status() const {
    std::lock_guard lock(mMutex);
    return {mDirectory, mSaved.size(), mWrites, mBatches, mPending.size() + mInFlight, mRejected, mError};
}

void WorldWriter::run() noexcept {
    try {
        for (;;) {
            std::vector<ChunkSnapshot> snapshots;
            snapshots.reserve(writeBatchLimit);
            {
                std::unique_lock lock(mMutex);
                mReady.wait(lock, [&] { return mStopping || !mOrder.empty(); });
                if (mOrder.empty()) {
                    break;
                }
                // A short window lets a burst share one durable LevelDB commit.
                if (!mStopping && mOrder.size() < writeBatchLimit) {
                    mReady.wait_for(lock, writeBatchDelay, [&] {
                        return mStopping || mOrder.size() >= writeBatchLimit;
                    });
                }
                std::size_t bytes = 0;
                while (!mOrder.empty() && snapshots.size() < writeBatchLimit && bytes < writeBatchBytes) {
                    auto entry = mPending.extract(mOrder.front());
                    mOrder.pop_front();
                    for (auto const& record : entry.mapped().records) {
                        bytes += record.key.size() + record.value.size();
                    }
                    snapshots.push_back(std::move(entry.mapped()));
                }
                mInFlight = snapshots.size();
            }
            leveldb::WriteBatch batch;
            for (auto const& snapshot : snapshots) {
                for (auto const& record : snapshot.records) {
                    batch.Put(record.key, record.value);
                }
            }
            leveldb::WriteOptions options;
            options.sync = true;
            auto const result = mDatabase->Write(options, &batch);
            if (!result.ok()) {
                throw std::runtime_error(result.ToString());
            }
            {
                std::lock_guard lock(mMutex);
                for (auto const& snapshot : snapshots) {
                    mSaved.insert(snapshot.identity);
                }
                mWrites += snapshots.size();
                ++mBatches;
                mInFlight = 0;
            }
            mDrained.notify_all();
        }
    } catch (std::exception const& error) {
        std::lock_guard lock(mMutex);
        mError = error.what();
        mInFlight = 0;
    } catch (...) {
        std::lock_guard lock(mMutex);
        mError = "Unknown LevelDB writer failure";
        mInFlight = 0;
    }
    mDrained.notify_all();
}

} // namespace worlddl
