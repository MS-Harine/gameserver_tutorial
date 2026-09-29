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

void User::set_username(std::string str)
{
    username_ = std::move(str);
}

const std::string& User::get_username() const
{
    return username_;
}

bool User::is_connected() const
{
    return connected_;
}

bool User::disconnect()
{
    if (connected_.exchange(false) == false)
        return false;
    session_->close();
    return true;
}

bool User::send(const std::vector<std::byte>& data)
{
    return session_->send(data);
}
