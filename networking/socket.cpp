#include "socket.h"

#ifdef OS_LINUX
#include <unistd.h>
#endif

Socket::Socket()
{
    handle_ = socket(AF_INET, SOCK_STREAM, 0);
}

Socket::Socket(native_handle_t handle)
    : handle_(handle)
{
}

Socket::~Socket()
{
    close();
}

Socket::Socket(Socket&& other)
    : handle_(other.handle_), addr_(other.addr_)
{
    other.handle_ = INVALID_HANDLE;
}

Socket& Socket::operator=(Socket&& other)
{
    if (this != &other)
    {
        close();
        handle_ = other.handle_;
        addr_ = other.addr_;
        other.handle_ = INVALID_HANDLE;
    }
    return *this;
}

Socket::native_handle_t Socket::get_native_handle()
{
    return handle_;
}

void Socket::close()
{
    if (handle_ == INVALID_HANDLE)
        return;

    shutdown(handle_, SHUT_RDWR);
    ::close(handle_);
    handle_ = INVALID_HANDLE;
}

void Socket::set_addr(sockaddr_in_t addr)
{
    addr_ = addr;
}