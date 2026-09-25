#include "bootstraper.h"
#include <atomic>
#include <csignal>

std::atomic<bool> g_stop_flag{false};

void signal_handler([[ maybe_unused ]] int signal)
{
    g_stop_flag = true;
    g_stop_flag.notify_all();
}

int main()
{
    std::signal(SIGINT, signal_handler);
    std::signal(SIGPIPE, SIG_IGN);

    Bootstraper bootstraper;
    bootstraper.run(8080);
    while (g_stop_flag == false) g_stop_flag.wait(false);
    bootstraper.stop();

    return 0;
}
