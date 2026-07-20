#ifndef __POLLER_H__
#define __POLLER_H__

#include "os.h"
#include "session.h"
#include <unordered_map>
#include <memory>
#include <functional>
#include <vector>
#include <cstddef>
#include <stop_token>
#include <mutex>

class Poller
{
public:
    enum Event : uint8_t
    {
        READ        = 0x1 << 0,
        WRITE       = 0x1 << 1,
        READ_WRITE  = READ | WRITE
    };

    // consumed_byte(Session, Transmitted byte, is_write)
    using EventCallback = std::function<std::size_t(std::shared_ptr<Session>, std::ptrdiff_t, bool)>;

public:
    virtual ~Poller() = default;

    virtual bool add(std::shared_ptr<Session> session, Event event) = 0;
    virtual bool remove(std::shared_ptr<Session> session) = 0;
    virtual void poll(std::stop_token token, EventCallback cb) = 0;
    virtual bool send(std::shared_ptr<Session> session, const std::vector<std::byte>& data) = 0;
};

#ifdef WINDOWS
#else
class Epoll : public Poller
{
public:
    Epoll();
    ~Epoll();

    bool add(std::shared_ptr<Session> session, Event event) override;
    bool remove(std::shared_ptr<Session> session) override;
    void poll(std::stop_token token, EventCallback cb) override;
    bool send(std::shared_ptr<Session> session, const std::vector<std::byte>& data) override;

private:
    void close_session(std::shared_ptr<Session> session);
    uint32_t create_flag(Event ev);

private:
    int epoll_fd_;
    std::unordered_map<int, std::pair<std::shared_ptr<Session>, Event>> session_map_;

    std::mutex write_lock_;
    std::mutex session_map_lock_;
};
#endif

using SysPoller =
#ifdef WINDOWS
#else
    Epoll
#endif
;

#endif // __POLLER_H__
