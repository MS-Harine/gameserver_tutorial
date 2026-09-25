#include "bootstraper.h"
#include "reactor.h"
#include "acceptor.h"
#include "world.h"
#include "packet.h"
#include "dispatcher.h"
#include <stop_token>

Bootstraper::Bootstraper() = default;

Bootstraper::~Bootstraper()
{
    stop();
}

void Bootstraper::run(int port)
{
    initialize_game();
    initialize_network(port);
}

void Bootstraper::stop()
{
    dispatcher_thread_.request_stop();
    accept_thread_.request_stop();
    reactor_thread_.request_stop();

    if (accept_thread_.joinable()) accept_thread_.join();
    if (reactor_thread_.joinable()) reactor_thread_.join();
    if (dispatcher_thread_.joinable()) dispatcher_thread_.join();
}

void Bootstraper::initialize_network(int port)
{
    reactor_ = std::make_shared<SysPoller>();
    acceptor_ = std::make_unique<Acceptor>();

    accept_thread_ = std::jthread([this, port](std::stop_token token) {
        accept_work(token, port);
    });
    reactor_thread_ = std::jthread([this](std::stop_token token) {
        reactor_work(token);
    });
}

void Bootstraper::initialize_game()
{
    world_ = std::make_shared<World>();
    dispatcher_ = std::make_shared<Dispatcher>(world_);
    dispatcher_thread_ = std::jthread([this](std::stop_token token) {
        dispatcher_->run(token);
    });
}

void Bootstraper::accept_work(std::stop_token token, int port)
{
    if (acceptor_->ready(port, 5) == false)
    {
        throw std::runtime_error("Acceptor ready failed");
    }

    std::shared_ptr<Socket> sock = acceptor_->get_socket();
    std::stop_callback cb(token, [this]() {
        acceptor_->get_socket()->close();
    });

    while (!token.stop_requested())
    {
        auto client_sock = acceptor_->accept();
        if (client_sock == nullptr)
            continue;
        
        auto session = std::make_shared<Session>(client_sock);
        reactor_->add(session, Reactor::Event::READ);
        world_->add_user(session);
    }
}

void Bootstraper::reactor_work(std::stop_token token)
{
    reactor_->poll(token, [this](
        std::shared_ptr<Session> session,
        std::ptrdiff_t byte_transmit, 
        bool is_write
    ) -> std::size_t 
    {
        if (is_write)
        {
            return byte_transmit;
        }

        if (byte_transmit <= 0) // Connection closed
        {
            world_->remove_user(session->get_sock()->get_native_handle(), World::raw_type_t);
            return byte_transmit;
        }

        const auto& buffer = session->get_recv_buffer();
        int buffer_index = 0;
        while (buffer.size() - buffer_index >= sizeof(PacketHeader))
        {
            const PacketHeader* header = reinterpret_cast<const PacketHeader*>(buffer.data() + buffer_index);
            if (header->packet_size < sizeof(PacketHeader) || header->packet_size > MAX_PACKET_SIZE)
            {
                // Invalid packet
                break;
            }

            if (buffer.size() - buffer_index < header->packet_size)
            {
                break;
            }

            dispatcher_->add_packet(
                world_->get_user(session->get_sock()->get_native_handle(), World::raw_type_t),
                std::vector<std::byte>(buffer.begin() + buffer_index, buffer.begin() + buffer_index + header->packet_size)
            );
            buffer_index += header->packet_size;
        }
        
        return buffer_index;
    });
}