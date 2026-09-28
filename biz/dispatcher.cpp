#include "dispatcher.h"
#include "world.h"
#include "packet.h"
#include "packets_generated.h"
#include <memory>
#include <span>
#include <iostream>

Dispatcher::Dispatcher(std::shared_ptr<World> world)
    : world_(std::move(world))
{
    init_handlers();
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

        std::cout << "[DISPATCH] Receive packet " << header->packet_id << " from user " << user->get_user_id() << std::endl;

        auto func_iter = dispatch_list_.find(header->packet_id);
        if (func_iter == dispatch_list_.end())
        {
            // Invalid packet
            std::cout << "Cannot find packet " << header->packet_id << ". Invalid packet." << std::endl;
            continue;
        }

        Error result = std::invoke(func_iter->second, user, payload);
        if (result != Error::None)
        {
            // Error handling
        }
    }
}

void Dispatcher::init_handlers()
{
    register_handler<Packet::C2S_Connect>([this](auto user, const auto& packet) { return handle_connect(user, packet); });
    register_handler<Packet::C2S_Disconnect>([this](auto user, const auto& packet) { return handle_disconnect(user, packet); });
    register_handler<Packet::C2S_SetPosition>([this](auto user, const auto& packet) { return handle_set_position(user, packet); });
    register_handler<Packet::C2S_Move>([this](auto user, const auto& packet) { return handle_move(user, packet); });
}

Error Dispatcher::handle_connect(std::shared_ptr<User> user, const Packet::C2S_Connect& packet)
{
    Packet::S2C_Connect send_packet;
    send_packet.userid = user->get_user_id();
    send_packet.username = packet.username;
    send_packet.is_other_user = true;
    world_->broadcast_except_user(send_packet, user);
    user->set_username(packet.username);

    for (auto& other_user : world_->get_users())
    {
        send_packet.userid = other_user->get_user_id();
        send_packet.username = other_user->get_username();
        send_packet.is_other_user = other_user->get_user_id() != user->get_user_id();
        user->send(send_packet);
    }

    return Error::None;
}

Error Dispatcher::handle_disconnect(std::shared_ptr<User> user, [[ maybe_unused ]] const Packet::C2S_Disconnect& packet)
{
    Packet::S2C_Disconnect send_packet;
    send_packet.userid = user->get_user_id();
    send_packet.reason = "Client request";

    world_->broadcast(send_packet);
    
    return Error::None;
}

Error Dispatcher::handle_set_position(std::shared_ptr<User> user, const Packet::C2S_SetPosition& packet)
{
    Packet::S2C_SetPosition send_packet;
    send_packet.userid = user->get_user_id();
    send_packet.x = packet.x;
    send_packet.y = packet.y;
    send_packet.facing_right = packet.facing_right;
    send_packet.reset_velocity = packet.reset_velocity;
    
    world_->broadcast(send_packet);

    return Error::None;
}

Error Dispatcher::handle_move(std::shared_ptr<User> user, const Packet::C2S_Move& packet)
{
    Packet::S2C_Move send_packet;
    send_packet.userid = user->get_user_id();
    send_packet.x = packet.x;
    send_packet.y = packet.y;
    send_packet.vx = packet.vx;
    send_packet.vy = packet.vy;
    send_packet.facing_right = packet.facing_right;

    world_->broadcast(send_packet);

    return Error::None;
}