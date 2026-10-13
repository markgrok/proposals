// Spike: Boost.Asio io_context driven by the browser/node event loop under Emscripten.
#include <boost/asio.hpp>
#include <emscripten/emscripten.h>
#include <emscripten/eventloop.h>
#include <chrono>
#include <cstdio>

namespace net = boost::asio;

struct app {
    net::io_context ioc{1};
    net::steady_timer timer{ioc};
    net::strand<net::io_context::executor_type> strand{ioc.get_executor()};
    int ticks = 0;
    bool done = false;
};

void arm(app& a) {
    a.timer.expires_after(std::chrono::milliseconds(50));
    a.timer.async_wait([&a](boost::system::error_code ec) {
        std::printf("tick %d ec=%d\n", a.ticks, ec.value());
        if (++a.ticks < 3) {
            arm(a);
        } else {
            a.done = true;
        }
    });
}

net::awaitable<void> coro(app& a) {
    net::steady_timer t{co_await net::this_coro::executor};
    t.expires_after(std::chrono::milliseconds(20));
    co_await t.async_wait(net::use_awaitable);
    std::puts("coroutine resumed after timer");
}

bool pump(double, void* p) {
    auto* a = static_cast<app*>(p);
    if (a->ioc.stopped()) {
        a->ioc.restart();
    }
    a->ioc.poll();
    if (a->done) {
        std::puts("done; leaving the loop");
        return false;
    }
    return true;
}

int main() {
    static app a;
    net::post(a.ioc, [] { std::puts("posted handler ran"); });
    net::post(a.strand, [] { std::puts("strand handler ran"); });
    net::co_spawn(a.ioc, coro(a), net::detached);
    arm(a);
    emscripten_set_timeout_loop(pump, 10, &a);
    std::puts("main returned control to the event loop");
    return 0;
}
