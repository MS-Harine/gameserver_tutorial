#ifndef __USER_H__
#define __USER_H__

#include <memory>
#include <vector>
#include <concepts>
#include <string>
#include <atomic>
#include "packet.h"

class Session;

class User
{
public:
    using userid_t = std::uint32_t;

public:
    User(std::shared_ptr<Session> session, userid_t user_id);

    userid_t get_user_id() const;
    void set_username(std::string str);
    const std::string& get_username() const;

    bool is_connected() const;
    bool disconnect();

    template <PacketType T>
    bool send(const T& packet)
    {
        return send(packet.serialize());
    }

    bool send(const std::vector<std::byte>& data);

private:
    std::shared_ptr<Session> session_;
    std::atomic<bool> connected_{ true };
    userid_t user_id_;
    std::string username_;
};

#endif // __USER_H__