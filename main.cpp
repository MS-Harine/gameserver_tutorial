#include <iostream>
#include <thread>

#include "poller.h"
#include "acceptor.h"

std::size_t callback(
    [[ maybe_unused ]] std::shared_ptr<Session> session, 
    [[ maybe_unused ]] std::ptrdiff_t byte_transmit, 
    [[ maybe_unused ]] bool is_write
)
{
    return 0;
}

void accept_work(std::unique_ptr<Acceptor> acceptor, std::shared_ptr<Poller> poller)
{
    auto client_sock = acceptor->accept();
    if (client_sock == nullptr)
        return;
    
    auto session = std::make_shared<Session>(client_sock);
    poller->add(session, Poller::Event::READ);
}

int main()
{
    std::shared_ptr<Poller> poller = std::make_shared<SysPoller>();
    std::jthread accept_thread([poller](std::stop_token token) {
        std::unique_ptr<Acceptor> acceptor = std::make_unique<Acceptor>();
        if (acceptor->ready(8080, 5) == false)
        {
            std::cerr << "Acceptor ready failed" << std::endl;
            return;
        }

        std::shared_ptr<Socket> sock = acceptor->get_socket();
        std::stop_callback cb(token, [sock]() {
            sock->close();
        });

        while (!token.stop_requested())
        {
            accept_work(std::move(acceptor), poller);
        }
    });
    
    std::jthread th([poller](std::stop_token token) {
        poller->poll(token, callback);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    accept_thread.request_stop();
    th.request_stop();

    return 0;
}
