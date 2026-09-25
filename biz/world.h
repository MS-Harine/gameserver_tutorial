#ifndef __WORLD_H__
#define __WORLD_H__

#include <map>
#include <memory>
#include <atomic>
#include <shared_mutex>
#include "socket.h"
#include "user.h"

class Session;

class World
{
public:
    struct raw_type {};
    static raw_type raw_type_t;

public:
    std::shared_ptr<User> add_user(std::shared_ptr<Session> session);
    std::shared_ptr<User> get_user(User::userid_t user_id) const;
    std::shared_ptr<User> get_user(Socket::native_handle_t sock_handle, raw_type) const;
    void remove_user(User::userid_t user_id);
    void remove_user(Socket::native_handle_t sock_handle, raw_type);

private:
    std::map<Socket::native_handle_t, std::shared_ptr<User>> users_;
    std::atomic<User::userid_t> unique_user_id_{0};
    mutable std::shared_mutex rw_mutex_;
};

#endif // __WORLD_H__