#include "world/CaptureQueue.h"

#include <iostream>
#include <memory>
#include <stdexcept>

namespace {

struct Chunk {};

void require(bool value, char const* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

void queueTest() {
    worlddl::CaptureQueue<Chunk> queue;
    auto                         first  = std::make_shared<Chunk>();
    auto                         second = std::make_shared<Chunk>();
    for (int i = 0; i < 100; ++i) {
        queue.push(first);
        queue.push(second);
    }
    require(queue.pop() == first, "Repeated notifications must preserve FIFO order");
    require(queue.pop() == second, "Each live chunk must be captured only once per notification burst");
    require(queue.empty(), "Duplicate notifications must not grow the capture backlog");

    queue.push(first);
    queue.push(second);
    queue.erase(first.get());
    queue.push(first);
    require(queue.pop() == second, "Removing a scheduled scan must remove its old queue position");
    require(queue.pop() == first, "A removed chunk can be scheduled again");

    queue.push(first);
    queue.clear();
    require(queue.empty() && !queue.pop(), "A new session must not inherit queued captures");
    queue.push({});
    require(queue.empty(), "Expired discoveries must not enter the queue");

    std::weak_ptr<Chunk> lifetime = first;
    queue.push(first);
    queue.push(second);
    first.reset();
    require(lifetime.expired(), "Scheduling must not retain unloaded engine chunks");
    require(!queue.pop(), "An unloaded chunk must not be dereferenced");
    require(queue.pop() == second && queue.empty(), "Expired entries must not prevent subsequent captures");
}

} // namespace

int main() {
    try {
        queueTest();
        std::cout << "WorldDL capture queue tests passed\n";
        return 0;
    } catch (std::exception const& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
