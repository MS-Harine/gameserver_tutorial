#ifndef __DISPATCHER_H__
#define __DISPATCHER_H__

#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <stop_token>
#include <span>
#include <utility>
#include <map>
#include <functional>
#include "packets_generated.h"

class World;
class User;

enum class Error
{
    None,
    AlreadyLoggedIn,
};

template <typename T>
concept ValidPacket = requires(std::span<const std::byte> payload)
{
    { T::ID } -> std::convertible_to<std::uint32_t>;
    { T::deserialize(payload) } -> std::same_as<T>;
};

class Dispatcher
{
public:
    Dispatcher(std::shared_ptr<World> world);

    void run(std::stop_token token);
    void add_packet(std::shared_ptr<User> user, std::vector<std::byte> packet);

    template <ValidPacket PacketType, typename HandlerFunc>
    void register_handler(HandlerFunc&& handler)
    {
        dispatch_list_[PacketType::ID] = [func = std::forward<HandlerFunc>(handler)] (
            std::shared_ptr<User> user,
            std::span<const std::byte> payload
        ) -> Error
        {
            PacketType packet = PacketType::deserialize(payload);
            return func(user, packet);
        };
    }

private:
    void init_handlers();

    Error handle_connect(std::shared_ptr<User> user, const Packet::C2S_Connect& packet);
    Error handle_disconnect(std::shared_ptr<User> user, const Packet::C2S_Disconnect& packet);
    Error handle_set_position(std::shared_ptr<User> user, const Packet::C2S_SetPosition& packet);
    Error handle_move(std::shared_ptr<User> user, const Packet::C2S_Move& packet);

private:
    std::shared_ptr<World> world_;
    std::queue<std::pair<std::shared_ptr<User>, std::vector<std::byte>>> packet_queue_;
    std::mutex mutex_;
    std::condition_variable_any cv_;
    std::map<std::uint32_t, std::function<Error(std::shared_ptr<User> user, std::span<const std::byte> payload)>> dispatch_list_;
};

#endif // __DISPATCHER_H__