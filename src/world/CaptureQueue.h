#pragma once

#include <list>
#include <memory>
#include <unordered_map>
#include <utility>

namespace worlddl {

// Coalesce notifications without keeping unloaded engine chunks alive.
template <typename Chunk>
class CaptureQueue {
public:
    CaptureQueue()                               = default;
    CaptureQueue(CaptureQueue const&)            = delete;
    CaptureQueue& operator=(CaptureQueue const&) = delete;

    void push(std::shared_ptr<Chunk> const& chunk) {
        if (!chunk) {
            return;
        }
        if (auto found = mPending.find(chunk.get()); found != mPending.end()) {
            found->second->second = chunk;
            return;
        }
        auto entry = mOrder.emplace(mOrder.end(), chunk.get(), std::weak_ptr<Chunk>{chunk});
        try {
            mPending.emplace(chunk.get(), entry);
        } catch (...) {
            mOrder.erase(entry);
            throw;
        }
    }

    std::shared_ptr<Chunk> pop() {
        if (mOrder.empty()) {
            return {};
        }
        auto entry = std::move(mOrder.front());
        mPending.erase(entry.first);
        mOrder.pop_front();
        return entry.second.lock();
    }

    void erase(Chunk* chunk) {
        if (auto found = mPending.find(chunk); found != mPending.end()) {
            mOrder.erase(found->second);
            mPending.erase(found);
        }
    }

    [[nodiscard]] bool empty() const { return mOrder.empty(); }

    void clear() {
        mPending.clear();
        mOrder.clear();
    }

private:
    using Order = std::list<std::pair<Chunk*, std::weak_ptr<Chunk>>>;
    Order                                                mOrder;
    std::unordered_map<Chunk*, typename Order::iterator> mPending;
};

} // namespace worlddl
