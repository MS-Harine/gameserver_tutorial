#ifndef __SESSION_H__
#define __SESSION_H__

#include "socket.h"
#include <any>
#include <memory>
#include <vector>
#include <cstddef>

class Session : public std::enable_shared_from_this<Session>
{
public:
    Session(std::shared_ptr<Socket> sock)
        : sock_(sock)
    {
        
    }
    
    void set_data(auto&& data)
    {
        data_ = std::forward<decltype(data)>(data);
    }

    std::any get_data() const { return data_; }
    std::shared_ptr<Socket> get_sock() const { return sock_; }
    std::vector<std::byte>& get_recv_buffer() { return recv_buffer; }
    std::vector<std::byte>& get_send_buffer() { return send_buffer; }

    void consume_recv_buffer(size_t bytes)
    {
        recv_buffer.erase(recv_buffer.begin(), std::min(recv_buffer.begin() + bytes, recv_buffer.end()));
    }

    void consume_send_buffer(size_t bytes)
    {
        send_buffer.erase(send_buffer.begin(), std::min(send_buffer.begin() + bytes, send_buffer.end()));
    }

private:
    std::shared_ptr<Socket> sock_;
    std::any data_;
    std::vector<std::byte> recv_buffer;
    std::vector<std::byte> send_buffer;
};

#endif // __SESSION_H__