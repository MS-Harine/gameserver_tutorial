#include "os.h"
#include "reactor.h"
#include "socket.h"
#include <string>
#include <cstring>
#include <stdexcept>
#include <array>
#include <cerrno>

#ifdef OS_LINUX
#include <sys/epoll.h>
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

    epoll_event ev{};
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
    std::lock_guard<std::mutex> lock(session_map_lock_);
    auto iter = session_map_.find(sock);
    if (iter == session_map_.end())
        return false;  

    if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, sock, nullptr) == -1)
        return false;
    
    session_map_.erase(iter);
    session->get_sock()->close();
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
                std::array<std::byte, 4096> buffer;
                ssize_t bytes_read = read(client_sock, buffer.data(), buffer.size());

                if (bytes_read > 0)
                {
                    auto& recv_buffer = session->get_recv_buffer();
                    recv_buffer.insert(recv_buffer.end(), buffer.begin(), buffer.begin() + bytes_read);

                    std::ptrdiff_t consumed = cb(session, bytes_read, false);
                    if (consumed < 0)
                    {
                        close_session(session);
                        continue;
                    }
                    session->consume_recv_buffer(consumed);
                }
                else if (bytes_read == 0)
                {
                    cb(session, bytes_read, false);
                    close_session(session);
                    continue;
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
                bool need_close = false;
                {
                    std::scoped_lock lock(session_map_lock_, write_lock_);
                    auto& send_buffer = session->get_send_buffer();

                    if (send_buffer.empty() == false)
                    {
                        ssize_t bytes_written = write(client_sock, send_buffer.data(), send_buffer.size());

                        if (bytes_written > 0)
                        {
                            session->consume_send_buffer(static_cast<std::size_t>(bytes_written));
                        }
                        else if (bytes_written == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
                        {
                            need_close = true;
                        }
                    }

                    if (send_buffer.empty() && need_close == false)
                    {
                        if (auto iter = session_map_.find(client_sock); iter != session_map_.end())
                        {
                            auto& current_event = iter->second.second;
                            current_event = static_cast<Event>(current_event & ~Event::WRITE);
                            struct epoll_event ev{};
                            ev.events = create_flag(current_event);
                            ev.data.fd = client_sock;
                            epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, client_sock, &ev);
                        }
                    }
                }

                if (need_close)
                {
                    cb(session, -1, false);
                    close_session(session);
                    continue;
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
    if (client_sock == Socket::INVALID_HANDLE)
        return;
    
    {
        std::lock_guard<std::mutex> lock(session_map_lock_);
        epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, client_sock, nullptr);
        session_map_.erase(client_sock);
    }
    session->get_sock()->close();
}

uint32_t Epoll::create_flag(Event event)
{
    uint32_t flag = EPOLLRDHUP;
    if (event & Event::READ) flag |= EPOLLIN;
    if (event & Event::WRITE) flag |= EPOLLOUT;
    return flag;
}

bool Epoll::send(std::shared_ptr<Session> session, const std::vector<std::byte>& data)
{
    if (session == nullptr)
        return false;

    int sock = session->get_sock()->get_native_handle();
    std::lock_guard<std::mutex> lock(session_map_lock_);
    auto iter = session_map_.find(sock);
    if (iter == session_map_.end())
        return false;

    auto& send_buffer = session->get_send_buffer();
    std::lock_guard<std::mutex> write_lock(write_lock_);
    send_buffer.insert(send_buffer.end(), data.begin(), data.end());

    auto& [s, current_event] = iter->second;
    if (!(current_event & Event::WRITE))
    {
        current_event = static_cast<Event>(current_event | Event::WRITE);
        struct epoll_event ev{};
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
