#ifndef __USER_H__
#define __USER_H__

#include <memory>

class Session;

class User
{
public:
    using userid_t = std::uint64_t;

public:
    User(std::shared_ptr<Session> session, userid_t user_id);

    userid_t get_user_id() const;

private:
    std::shared_ptr<Session> session_;
    userid_t user_id_;
};

#endif // __USER_H__