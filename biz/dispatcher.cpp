#include "dispatcher.h"
#include "world.h"
#include "packet.h"
#include <memory>
#include <span>

Dispatcher::Dispatcher(std::shared_ptr<World> world)
    : world_(std::move(world))
{
}

void Dispatcher::add_packet(std::shared_ptr<User> user, const std::vector<std::byte>& packet)
{
    std::lock_guard<std::mutex> lock(mutex_);
    packet_queue_.push({ user, packet });
    cv_.notify_one();
}

void Dispatcher::run(std::stop_token token)
{
    std::stop_callback cb(token, [this]() {
        cv_.notify_one();
    });

    while (!token.stop_requested())
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this, &token]() { return !packet_queue_.empty() || token.stop_requested(); });

        if (token.stop_requested())
            break;

        auto [user, packet] = packet_queue_.front();
        packet_queue_.pop();
        lock.unlock();

        const auto* header = reinterpret_cast<const PacketHeader*>(packet.data());
        std::span<const std::byte> payload(packet.data() + sizeof(PacketHeader), header->packet_size - sizeof(PacketHeader));

        auto func_iter = dispatch_list_.find(header->packet_id);
        if (func_iter == dispatch_list_.end())
        {
            // Invalid packet
            continue;
        }

        Error result = std::invoke(func_iter->second, user, payload);
        if (result != Error::None)
        {
            // Error handling
        }
    }
}