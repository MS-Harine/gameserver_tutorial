#include "acceptor.h"
#include <iostream>
#include "os.h"

#ifdef OS_WINDOWS
#else
#include <netinet/in.h>
#endif

Acceptor::Acceptor()
{
    socket_ = std::make_shared<Socket>();
}

bool Acceptor::ready(uint16_t port, int backlog)
{
    Socket::native_handle_t handle = socket_->get_native_handle();
    if (handle == Socket::INVALID_HANDLE)
        return false;
    
#ifdef OS_WINDOWS
#else
    int opt = 1;
    setsockopt(handle, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    addr_.sin_family = AF_INET;
    addr_.sin_addr.s_addr = INADDR_ANY;
    addr_.sin_port = htons(port);

    if (bind(handle, (sockaddr*)&addr_, sizeof(addr_)) < 0)
        return false;
    
    if (listen(handle, backlog) < 0)
        return false;
#endif
    return true;
}

std::shared_ptr<Socket> Acceptor::accept()
{
    Socket::sockaddr_in_t addr;
    Socket::socklen_t_t addr_len = sizeof(addr);

#ifdef OS_WINDOWS
#else
    int client_handle = ::accept(socket_->get_native_handle(), (sockaddr*)&addr, &addr_len);
    if (client_handle == -1)
        return nullptr;
#endif

    Socket client(client_handle);
    client.set_addr(addr);
    return std::make_shared<Socket>(std::move(client));
}

std::shared_ptr<Socket> Acceptor::get_socket()
{
    return socket_;
}
