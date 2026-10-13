// Throwaway feasibility probe: Boost.Asio compiled with Emscripten, without threads, driven by the host's
// event loop through io_context::poll() instead of a blocking run(): post, steady_timer and a C++20 coroutine.
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <emscripten/emscripten.h>
#include <emscripten/eventloop.h>

#include <chrono>
#include <cstdio>

namespace asio = boost::asio;

namespace {

asio::io_context io;
const auto start = std::chrono::steady_clock::now();

long elapsed_ms() {
    return static_cast<long>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count());
}

asio::awaitable<void> announce() {
    // Trystero's announce cadence: 0, 233, 766, 2099 ms after joining.
    asio::steady_timer timer(co_await asio::this_coro::executor);
    for (const int at : {0, 233, 766, 2099}) {
        timer.expires_at(start + std::chrono::milliseconds(at));
        co_await timer.async_wait(asio::use_awaitable);
        std::printf("asio: announce at %ld ms (scheduled %d)\n", elapsed_ms(), at);
    }
    std::printf("asio: coroutine done, stopping the interval\n");
    emscripten_force_exit(0);
}

void drive(void*) {
    io.poll();  // never run(): the host's event loop must keep turning, or no JS callback is ever delivered
}

}  // namespace

int main() {
    asio::post(io, [] { std::printf("asio: posted handler ran at %ld ms\n", elapsed_ms()); });
    asio::co_spawn(io, announce(), asio::detached);
    emscripten_set_interval(drive, 10, nullptr);
    emscripten_exit_with_live_runtime();
}
