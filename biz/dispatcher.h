#ifndef __DISPATCHER_H__
#define __DISPATCHER_H__

#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <stop_token>
#include <span>
#include <utility>
#include <map>
#include <functional>

class World;
class User;

enum class Error
{
    None
};

class Dispatcher
{
public:
    Dispatcher(std::shared_ptr<World> world);

    void run(std::stop_token token);
    void add_packet(std::shared_ptr<User> user, const std::vector<std::byte>& packet);

private:
    std::shared_ptr<World> world_;
    std::queue<std::pair<std::shared_ptr<User>, std::vector<std::byte>>> packet_queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::map<std::uint32_t, std::function<Error(std::shared_ptr<User> user, std::span<const std::byte> payload)>> dispatch_list_;
};

#endif // __DISPATCHER_H__