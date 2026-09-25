#include "user.h"

User::User(std::shared_ptr<Session> session, userid_t user_id)
    : session_(std::move(session)), user_id_(user_id)
{
}

User::userid_t User::get_user_id() const
{
    return user_id_;
}