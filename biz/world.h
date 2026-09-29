#ifndef __WORLD_H__
#define __WORLD_H__

#include <map>
#include <memory>
#include <atomic>
#include <shared_mutex>
#include "socket.h"
#include "user.h"
#include "packet.h"

class Session;

class World
{
public:
    struct raw_type {};
    static constexpr raw_type raw_type_t{};

public:
    std::shared_ptr<User> add_user(std::shared_ptr<Session> session);
    std::shared_ptr<User> get_user(User::userid_t user_id) const;
    std::shared_ptr<User> get_user(Socket::native_handle_t sock_handle, raw_type) const;
    std::vector<std::shared_ptr<User>> get_users() const;
    bool remove_user(User::userid_t user_id);
    void remove_user(Socket::native_handle_t sock_handle, raw_type);

    template <PacketType T>
    void broadcast(T packet)
    {
        auto data = packet.serialize();
        for (auto& user : get_users())
        {
            user->send(data);
        }
    }

    template <PacketType T>
    void broadcast_except_user(T packet, std::shared_ptr<User> target_user)
    {
        auto data = packet.serialize();
        for (auto& user : get_users())
        {
            if (user->get_user_id() == target_user->get_user_id())
                continue;
            user->send(data);
        }
    }

private:
    std::map<Socket::native_handle_t, std::shared_ptr<User>> users_;
    std::atomic<User::userid_t> unique_user_id_{0};
    mutable std::shared_mutex rw_mutex_;
};

#endif // __WORLD_H__