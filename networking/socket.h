#ifndef __SOCKET_H__
#define __SOCKET_H__

#include "os.h"

#ifdef OS_WINDOWS
#else
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#include <vector>

class Socket
{
public:
#ifdef OS_WINDOWS
    using native_handle_t = SOCKET;
    constexpr static native_handle_t INVALID_HANDLE = INVALID_SOCKET;
    using sockaddr_in_t = SOCKADDR_IN;
    using socklen_t = int;
#else
    using native_handle_t = int;
    constexpr static native_handle_t INVALID_HANDLE = -1;
    using sockaddr_in_t = sockaddr_in;
    using socklen_t = ::socklen_t;
#endif

public:
    Socket();
    Socket(native_handle_t handle);
    ~Socket();

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    Socket(Socket&&);
    Socket& operator=(Socket&&);

    native_handle_t get_native_handle();
    void close();
    void set_addr(sockaddr_in_t addr);

private:
    native_handle_t handle_;
    sockaddr_in_t addr_;
};

#endif // __SOCKET_H__