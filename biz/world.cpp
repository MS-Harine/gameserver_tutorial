#include "world.h"
#include "user.h"
#include "session.h"
#include "socket.h"
#include <memory>
#include <mutex>
#include <shared_mutex>

std::shared_ptr<User> World::add_user(std::shared_ptr<Session> session)
{
    std::unique_lock<std::shared_mutex> guard(rw_mutex_);
    auto user = std::make_shared<User>(session, unique_user_id_.fetch_add(1));
    users_[session->get_sock()->get_native_handle()] = user;
    return user;
}

std::shared_ptr<User> World::get_user(User::userid_t user_id) const
{
    std::shared_lock<std::shared_mutex> guard(rw_mutex_);
    for (const auto& [_, user] : users_)
    {
        if (user->get_user_id() == user_id)
            return user;
    }

    return nullptr;
}

std::shared_ptr<User> World::get_user(Socket::native_handle_t sock_handle, raw_type) const
{
    std::shared_lock<std::shared_mutex> guard(rw_mutex_);
    if (auto iter = users_.find(sock_handle); iter != users_.end())
    {
        return iter->second;
    }
    return nullptr;
}

void World::remove_user(User::userid_t user_id)
{
    std::unique_lock<std::shared_mutex> guard(rw_mutex_);
    for (auto iter = users_.begin(); iter != users_.end(); ++iter)
    {
        if (iter->second->get_user_id() == user_id)
        {
            users_.erase(iter);
            break;
        }
    }
}

void World::remove_user(Socket::native_handle_t sock_handle, raw_type)
{
    std::unique_lock<std::shared_mutex> guard(rw_mutex_);
    users_.erase(sock_handle);
}