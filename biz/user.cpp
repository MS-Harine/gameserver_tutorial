#include "user.h"
#include "session.h"
#include <vector>

User::User(std::shared_ptr<Session> session, userid_t user_id)
    : session_(std::move(session)), user_id_(user_id)
{
}

User::userid_t User::get_user_id() const
{
    return user_id_;
}

void User::send(const std::vector<std::byte>& data)
{
    if (session_->send(data) == false)
    {
        throw std::runtime_error("Failed to send message");
    }
}
