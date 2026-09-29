#ifndef __SESSION_H__
#define __SESSION_H__

#include <memory>
#include <vector>
#include <cstddef>

class Reactor;
class Socket;

class Session : public std::enable_shared_from_this<Session>
{
public:
    Session(std::weak_ptr<Reactor> reactor, std::shared_ptr<Socket> sock);

    std::shared_ptr<Socket> get_sock() const;
    std::vector<std::byte>& get_recv_buffer();
    std::vector<std::byte>& get_send_buffer();

    bool send(const std::vector<std::byte>& data);
    void consume_recv_buffer(size_t bytes);
    void consume_send_buffer(size_t bytes);

    bool close();

private:
    std::weak_ptr<Reactor> reactor_;
    std::shared_ptr<Socket> sock_;
    std::vector<std::byte> recv_buffer;
    std::vector<std::byte> send_buffer;
};

#endif // __SESSION_H__