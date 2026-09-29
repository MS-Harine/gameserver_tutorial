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

void Dispatcher::add_packet(std::shared_ptr<User> user, std::vector<std::byte> packet)
{
    std::lock_guard<std::mutex> lock(mutex_);
    packet_queue_.push({ user, std::move(packet) });
    cv_.notify_one();
}

void Dispatcher::run(std::stop_token token)
{
    while (!token.stop_requested())
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (cv_.wait(lock, token, [this]() { return !packet_queue_.empty(); }) == false)
            break;

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

        Error result = Error::None;
        try
        {
            result = std::invoke(func_iter->second, user, payload);
        }
        catch(const std::exception& e)
        {
            std::cout << "[DISPATCH] Handler throwed exception. ID " << header->packet_id
                      << " from user " << user->get_user_id() << " : " << e.what() << std::endl;
            handle_disconnect(user, {});
            continue;
        }
        
        if (result != Error::None)
        {
            std::cout << "[DISPATCH] Handler return error. ID " << header->packet_id
                      << " from user " << user->get_user_id() << " : " << static_cast<int>(result) << std::endl;
            handle_disconnect(user, {});
            continue;
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
    if (user->is_login() == true)
    {
        return Error::AlreadyLoggedIn;
    }

    user->set_login(true);
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

        if (other_user->get_user_id() == user->get_user_id())
            continue;
        
        auto user_pos = other_user->get_pos();
        Packet::S2C_SetPosition pos_packet;
        pos_packet.userid = other_user->get_user_id();
        pos_packet.x = user_pos.x;
        pos_packet.y = user_pos.y;
        pos_packet.facing_right = user_pos.facing_right;
        pos_packet.reset_velocity = true;
        user->send(pos_packet);
    }

    return Error::None;
}

Error Dispatcher::handle_disconnect(std::shared_ptr<User> user, [[ maybe_unused ]] const Packet::C2S_Disconnect& packet)
{
    std::cout << "[CONNECTION] disconnect user id : " << user->get_user_id() << std::endl;

    user->disconnect();
    if (world_->remove_user(user->get_user_id()) == false)
        return Error::None;

    Packet::S2C_Disconnect send_packet;
    send_packet.userid = user->get_user_id();
    send_packet.reason = "Client disconnected";
    world_->broadcast_except_user(send_packet, user);
    return Error::None;
}

Error Dispatcher::handle_set_position(std::shared_ptr<User> user, const Packet::C2S_SetPosition& packet)
{
    user->set_pos(packet.x, packet.y, packet.facing_right);

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