#ifndef __USER_H__
#define __USER_H__

#include <memory>
#include <vector>
#include <concepts>

class Session;

class User
{
public:
    using userid_t = std::uint64_t;

public:
    User(std::shared_ptr<Session> session, userid_t user_id);

    userid_t get_user_id() const;

    template <typename PacketType>
    requires requires(const PacketType& p) {
        { p.serialize() } -> std::same_as<std::vector<std::byte>>;
    }
    void send(const PacketType& packet)
    {
        send(packet.serialize());
    }

private:
    void send(const std::vector<std::byte>& data);

private:
    std::shared_ptr<Session> session_;
    userid_t user_id_;
};

#endif // __USER_H__