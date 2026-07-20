#include "os.h"
#include "poller.h"
#include <string>
#include <cstring>
#include <stdexcept>

#ifdef OS_LINUX
#include "sys/epoll.h"
#include <unistd.h>
#include <fcntl.h>

Epoll::Epoll()
{
    epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd_ == -1)
    {
        throw std::runtime_error(std::string("Cannot create epoll. error: ") + std::strerror(errno));
    }
}

Epoll::~Epoll()
{
    close(epoll_fd_);
}

bool Epoll::add(std::shared_ptr<Session> session, Event event)
{
    if (session == nullptr)
        return false;

    int sock = session->get_sock()->get_native_handle();
    {
        std::lock_guard<std::mutex> lock(session_map_lock_);
        if (session_map_.contains(sock))
            return false;
    }

    // Set socket as non-blocking
    fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) | O_NONBLOCK);
    
    epoll_event ev;
    ev.events = create_flag(event);
    ev.data.fd = session->get_sock()->get_native_handle();

    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, sock, &ev) == -1)
        return false;
    
    {
        std::lock_guard<std::mutex> lock(session_map_lock_);
        session_map_[sock] = std::make_pair(std::move(session), event);
    }
    return true;
}

bool Epoll::remove(std::shared_ptr<Session> session)
{
    if (session == nullptr)
        return false;
    
    int sock = session->get_sock()->get_native_handle();
    decltype(session_map_.end()) iter;
    {
        std::lock_guard<std::mutex> lock(session_map_lock_);
        iter = session_map_.find(sock);
        if (iter == session_map_.end())
            return false;  
    }

    if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, sock, nullptr) == -1)
        return false;
    
    session->get_sock()->close();
    {
        std::lock_guard<std::mutex> lock(session_map_lock_);
        session_map_.erase(iter);
    }
    return true;
}

void Epoll::poll(std::stop_token token, EventCallback cb)
{
    constexpr int MAX_EVENTS = 32;
    std::vector<struct epoll_event> events(MAX_EVENTS);
    
    while (!token.stop_requested())
    {
        // Wakeup event 500ms
        int fds = epoll_wait(epoll_fd_, events.data(), MAX_EVENTS, 500);

        if (fds == 0) [[ likely ]] continue; // timeout
        else if (fds == -1) [[ unlikely ]]
        {
            if (errno == EINTR) continue;
            break;
        }
        
        for (int i = 0; i < fds; ++i)
        {
            int client_sock = events[i].data.fd;
            std::shared_ptr<Session> session;
            {
                std::lock_guard<std::mutex> lock(session_map_lock_);
                auto iter = session_map_.find(client_sock);
                if (iter == session_map_.end())
                {
                    // Error
                    continue;
                }
                session = iter->second.first;
            }
            
            if (events[i].events & (EPOLLERR | EPOLLHUP))
            {
                cb(session, 0, false);
                close_session(session);
                continue;
            }

            // Read
            if (events[i].events & EPOLLIN)
            {
                auto& recv_buffer = session->get_recv_buffer();
                ssize_t bytes_read = read(client_sock, recv_buffer.data(), recv_buffer.size());

                if (bytes_read >= 0)
                {
                    std::size_t consumed = cb(session, bytes_read, false);
                    session->consume_recv_buffer(consumed);

                    if (bytes_read == 0)
                    {
                        close_session(session);
                        continue;
                    }
                }
                else
                {
                    // Error
                    if (errno != EAGAIN && errno != EWOULDBLOCK)
                    {
                        cb(session, -1, false);
                        close_session(session);
                        continue;
                    }
                }
            }
            
            // Write
            if (events[i].events & EPOLLOUT)
            {
                auto& send_buffer = session->get_send_buffer();
                if (send_buffer.empty() == false)
                {
                    ssize_t bytes_written = write(client_sock, send_buffer.data(), send_buffer.size());

                    if (bytes_written > 0)
                    {
                        std::size_t consumed = cb(session, bytes_written, true);
                        session->consume_send_buffer(consumed);
                    }

                    if (send_buffer.empty())
                    {
                        std::lock_guard<std::mutex> lock(session_map_lock_);
                        auto iter = session_map_.find(client_sock);
                        if (iter == session_map_.end())
                            continue;
                        auto current_event = iter->second.second;

                        current_event = static_cast<Event>(current_event & ~Event::WRITE);
                        struct epoll_event ev;
                        ev.events = create_flag(current_event);
                        ev.data.fd = session->get_sock()->get_native_handle();
                        epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, client_sock, &ev);
                    }
                }
            }
        }
    }
}

void Epoll::close_session(std::shared_ptr<Session> session)
{
    if (session == nullptr)
        return;
    
    int client_sock = session->get_sock()->get_native_handle();
    epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, client_sock, nullptr);
    session->get_sock()->close();
    {
        std::lock_guard<std::mutex> lock(session_map_lock_);
        session_map_.erase(client_sock);
    }
}

uint32_t Epoll::create_flag(Event event)
{
    uint32_t flag = EPOLLET | EPOLLRDHUP;
    if (event & Event::READ) flag |= EPOLLIN;
    if (event & Event::WRITE) flag |= EPOLLOUT;
    return flag;
}

bool Epoll::send(std::shared_ptr<Session> session, const std::vector<std::byte>& data)
{
    if (session == nullptr)
        return false;

    int sock = session->get_sock()->get_native_handle();
    decltype(session_map_.begin()) iter;
    {
        std::lock_guard<std::mutex> lock(session_map_lock_);
        iter = session_map_.find(sock);
        if (iter == session_map_.end())
            return false;
    }

    auto& send_buffer = session->get_send_buffer();
    std::lock_guard<std::mutex> lock(write_lock_);
    send_buffer.insert(send_buffer.end(), data.begin(), data.end());

    auto& [s, current_event] = iter->second;
    if (!(current_event & Event::WRITE))
    {
        current_event = static_cast<Event>(current_event | Event::WRITE);
        struct epoll_event ev;
        ev.events = create_flag(current_event);
        ev.data.fd = session->get_sock()->get_native_handle();
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, sock, &ev) == -1)
        {
            return false;
        }
    }
    return true;
}
#endif
