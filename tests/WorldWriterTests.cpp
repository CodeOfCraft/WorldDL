#include "world/BedrockFormat.h"
#include "world/WorldWriter.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <thread>

#include <leveldb/db.h>

using namespace worlddl;

namespace {

void require(bool value, char const* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

std::string readFile(std::filesystem::path const& file) {
    std::ifstream stream(file, std::ios::binary);
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}

std::unique_ptr<leveldb::DB> openDb(std::filesystem::path const& directory) {
    leveldb::Options options;
    leveldb::DB* db = nullptr;
    auto status = leveldb::DB::Open(options, (directory / "db").string(), &db);
    require(status.ok(), "Saved database must reopen after stopping");
    return std::unique_ptr<leveldb::DB>(db);
}

void formatTest() {
    auto const overworld = chunkKey(-1, 2, 0, ChunkTag::SubChunk, std::int8_t{-4});
    require(overworld == std::string("\xff\xff\xff\xff\x02\0\0\0\x2f\xfc", 10),
            "Negative coordinates and signed subchunk Y must be little endian");
    auto const nether = chunkKey(-1, 2, 1, ChunkTag::Version);
    require(nether == std::string("\xff\xff\xff\xff\x02\0\0\0\x01\0\0\0\x2c", 13),
            "Non-overworld dimensions must appear in the database key");
    auto const end = chunkKey(-1, 2, 2, ChunkTag::Version);
    require(end != nether, "Identical coordinates in different dimensions must remain separate");
    auto const metadata = levelDat(std::string("\x0a\0\0\0", 4));
    require(metadata == std::string("\x0a\0\0\0\x04\0\0\0\x0a\0\0\0", 12),
            "level.dat must have a storage version, byte count and binary NBT root");
}

void storageTest(std::filesystem::path const& root) {
    auto const directory = root / "world";
    auto const metadata = levelDat(std::string("\x0a\0\0\0", 4));
    std::string const negativeSubchunk = chunkKey(-1, -2, 0, ChunkTag::SubChunk, std::int8_t{-4});
    std::string const higherSubchunk = chunkKey(-1, -2, 0, ChunkTag::SubChunk, std::int8_t{10});
    std::string const netherSubchunk = chunkKey(-1, -2, 1, ChunkTag::SubChunk, std::int8_t{0});
    {
        WorldWriter writer(directory, "Test World", metadata, 32);
        auto submit = [&](ChunkSnapshot snapshot) {
            while (!writer.submit(std::move(snapshot))) {
                require(!snapshot.identity.empty() && !snapshot.records.empty(),
                        "Rejected submissions must remain available for retry");
                require(writer.status().error.empty(), "Writer must remain healthy while queue is full");
                std::this_thread::yield();
            }
        };
        for (int i = 0; i < 400; ++i) {
            submit({"column-" + std::to_string(i), {{"key-" + std::to_string(i), std::string(1000, 'a')}}});
        }
        submit({"overworld", {{negativeSubchunk, "first"}}});
        submit({"overworld", {{higherSubchunk, "later partial column"}}});
        submit({"overworld", {{negativeSubchunk, "updated"}}});
        submit({"nether", {{netherSubchunk, "nether"}}});
        require(writer.flush(), "Flush must wait until accepted writes are durable");
        require(writer.status().chunks == 402, "Status must count distinct columns rather than updates");
        submit({"last", {{"last-key", "drained on stop"}}});
        writer.stop();
        require(!writer.submit({"stopped", {{"unused", "unused"}}}), "Stopped writer must reject submissions");
        require(writer.flush(), "Flush after stop must not hang");
        writer.stop();
    }
    require(readFile(directory / "level.dat") == metadata, "Exported metadata must be binary-safe");
    require(readFile(directory / "level.dat_old") == metadata, "Metadata backup must exist");
    require(readFile(directory / "levelname.txt") == "Test World", "World name must be saved");
    auto db = openDb(directory);
    auto value = [&](std::string const& key) {
        std::string result;
        require(db->Get(leveldb::ReadOptions{}, key, &result).ok(), "Every accepted record must be persisted");
        return result;
    };
    require(value(negativeSubchunk) == "updated", "Latest block changes must replace earlier snapshots");
    require(value(higherSubchunk) == "later partial column", "Partial snapshots must preserve other subchunks");
    require(value(netherSubchunk) == "nether", "Nether data must stay separate from overworld data");
    require(value("last-key") == "drained on stop", "Stop must drain pending writes before releasing the database");
    for (int i = 0; i < 400; ++i) {
        require(value("key-" + std::to_string(i)) == std::string(1000, 'a'), "Queue pressure must not corrupt records");
    }
    db.reset();
    bool refused = false;
    try {
        WorldWriter writer(directory, "Replacement", metadata);
    } catch (std::exception const&) {
        refused = true;
    }
    require(refused, "Existing world directories must never be overwritten");
    require(readFile(directory / "levelname.txt") == "Test World", "Refusing overwrite must preserve existing files");
    bool invalidLimit = false;
    try {
        WorldWriter writer(root / "invalid", "Invalid", metadata, 0);
    } catch (std::invalid_argument const&) {
        invalidLimit = true;
    }
    require(invalidLimit, "A zero queue capacity must be rejected");
}

} // namespace

int main() {
    auto const root = std::filesystem::temp_directory_path() / ("worlddl-test-" + std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count()));
    try {
        std::filesystem::create_directory(root);
        formatTest();
        storageTest(root);
        std::filesystem::remove_all(root);
        std::cout << "WorldDL format and storage tests passed\n";
        return 0;
    } catch (std::exception const& error) {
        std::cerr << error.what() << "\nTest files: " << root << '\n';
        return 1;
    }
}
