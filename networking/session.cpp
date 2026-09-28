#include "session.h"
#include "reactor.h"
#include <memory>

Session::Session(std::weak_ptr<Reactor> reactor, std::shared_ptr<Socket> sock)
    : reactor_(reactor), sock_(sock)
{

}

std::shared_ptr<Socket> Session::get_sock() const
{
    return sock_;
}

std::vector<std::byte>& Session::get_recv_buffer()
{
    return recv_buffer;
}

std::vector<std::byte>& Session::get_send_buffer()
{
    return send_buffer;
}

bool Session::send(const std::vector<std::byte>& data)
{
    if (auto reactor = reactor_.lock())
    {
        return reactor->send(shared_from_this(), data);
    }
    return false;
}

void Session::consume_recv_buffer(size_t bytes)
{
    recv_buffer.erase(recv_buffer.begin(), std::min(recv_buffer.begin() + bytes, recv_buffer.end()));
}

void Session::consume_send_buffer(size_t bytes)
{
    send_buffer.erase(send_buffer.begin(), std::min(send_buffer.begin() + bytes, send_buffer.end()));
}