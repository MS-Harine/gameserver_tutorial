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
    set_login(false);
    session_->close();
    return true;
}

bool User::send(const std::vector<std::byte>& data)
{
    return session_->send(data);
}

bool User::is_login() const
{
    return logined_;
}

void User::set_login(bool state)
{
    logined_ = state;
}

User::PosInfo User::get_pos() const
{
    return { pos_.first, pos_.second, facing_right_ };
}

void User::set_pos(float x, float y, bool facing_right)
{
    pos_ = { x, y };
    facing_right_ = facing_right;
}
