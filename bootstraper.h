#ifndef __BOOTSTRAPER_H__
#define __BOOTSTRAPER_H__

#include <thread>
#include <memory>
#include <stop_token>

class Reactor;
class Acceptor;
class World;
class Dispatcher;

class Bootstraper
{
public:
    Bootstraper();
    ~Bootstraper();
    void run(int port = 8080);
    void stop();

private:
    void initialize_network(int port);
    void initialize_game();

    void accept_work(std::stop_token token, int port);
    void reactor_work(std::stop_token token);

private:
    std::shared_ptr<Reactor> reactor_;
    std::unique_ptr<Acceptor> acceptor_;
    std::jthread accept_thread_, reactor_thread_, dispatcher_thread_;

    std::shared_ptr<World> world_;
    std::shared_ptr<Dispatcher> dispatcher_;
};

#endif // __BOOTSTRAPER_H__