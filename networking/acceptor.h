#ifndef __ACCEPTOR_H__
#define __ACCEPTOR_H__

#include <stdint.h>
#include <memory>
#include "socket.h"
#include "os.h"

#ifdef OS_WINDOWS
#else
#include <netinet/in.h>
#endif

class Acceptor
{
public:
    Acceptor();
    ~Acceptor() = default;

    Acceptor(const Acceptor&) = delete;
    Acceptor& operator=(const Acceptor&) = delete;
    Acceptor(Acceptor&&) = default;
    Acceptor& operator=(Acceptor&&) = default;

    bool ready(uint16_t port, int backlog = 5);
    std::shared_ptr<Socket> accept();
    std::shared_ptr<Socket> get_socket();

private:
    std::shared_ptr<Socket> socket_;
#ifdef OS_WINDOWS
#else
    sockaddr_in addr_;
#endif
};

#endif // __ACCEPTOR_H__