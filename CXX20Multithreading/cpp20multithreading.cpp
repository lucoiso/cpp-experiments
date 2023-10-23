// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/cpp-experiments

#include <array>
#include <barrier>
#include <coroutine>
#include <iostream>
#include <latch>
#include <semaphore>
#include <syncstream>
#include <thread>

#define IMPLEMENT_SIMPLE_COROUTINE_INTERNAL_PROMISE_TYPE(input_task_name)      \
  struct promise_type {                                                        \
    static input_task_name get_return_object() { return {}; }                  \
    static std::suspend_never initial_suspend() { return {}; }                 \
    static std::suspend_never final_suspend() noexcept { return {}; }          \
    void return_void() {}                                                      \
    void unhandled_exception() {}                                              \
  }

struct barrier_task {
  IMPLEMENT_SIMPLE_COROUTINE_INTERNAL_PROMISE_TYPE(barrier_task);

  static barrier_task do_task() {
    std::cout << "performing barrier coroutine task..." << std::endl;

    constexpr std::uint8_t max_threads = 5U;
    std::array<std::jthread, max_threads> works{};

    std::barrier work_barrier(std::size(works), []() noexcept {
      std::cout << "Completion called." << std::endl;
    });

    for (auto it = std::begin(works); it != std::end(works); ++it) {
      *it = std::jthread(
          [&work_barrier,
           index = 1U + std::distance(std::begin(works), it)]() noexcept {
            std::osyncstream(std::cout)
                << "Thread " << index << " is starting his work." << std::endl;

            std::this_thread::sleep_for(std::chrono::seconds(1U));
            std::osyncstream(std::cout)
                << "Thread " << index << " reached step 1." << std::endl;
            work_barrier.arrive_and_wait();

            std::this_thread::sleep_for(std::chrono::seconds(1U));
            std::osyncstream(std::cout)
                << "Thread " << index << " reached step 2." << std::endl;
            work_barrier.arrive_and_wait();

            std::this_thread::sleep_for(std::chrono::seconds(1U));
            std::osyncstream(std::cout)
                << "Thread " << index << " reached step 3." << std::endl;
            work_barrier.arrive_and_wait();

            std::osyncstream(std::cout)
                << "Thread " << index << " finished." << std::endl;
            work_barrier.arrive_and_wait();
          });
    }

    co_return;
  }
};

struct latch_task {
  IMPLEMENT_SIMPLE_COROUTINE_INTERNAL_PROMISE_TYPE(latch_task);

  static latch_task do_task() {
    std::cout << "performing latch coroutine task..." << std::endl;

    constexpr std::uint8_t max_threads = 5U;
    std::array<std::jthread, max_threads> works{};

    std::latch work_latch(std::size(works));
    std::latch exit_latch(1U);

    for (auto it = std::begin(works); it != std::end(works); ++it) {
      *it = std::jthread([&work_latch, &exit_latch,
                          index =
                              std::distance(std::begin(works), it)]() noexcept {
        std::osyncstream(std::cout)
            << "Thread " << index << " is starting his work." << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(1U));

        std::osyncstream(std::cout)
            << "Thread " << index << " is waiting for the signal." << std::endl;
        work_latch.count_down();

        exit_latch.wait();

        std::osyncstream(std::cout)
            << "Thread " << index << " finished." << std::endl;
      });
    }

    work_latch.wait();
    std::cout << "Emitting the signal to finish." << std::endl;
    exit_latch.count_down();

    co_return;
  }
};

void ping_pong() {
  std::binary_semaphore pingsem(1);
  std::binary_semaphore pongsem(0);

  std::jthread ping([&](std::stop_token stoptk) noexcept {
    while (!stoptk.stop_requested()) {
      pingsem.acquire();
      std::cout << "ping" << std::endl;
      pongsem.release();
      std::this_thread::sleep_for(std::chrono::seconds(1U));
    }
  });

  std::jthread pong([&](std::stop_token stoptk) noexcept {
    while (!stoptk.stop_requested()) {
      pongsem.acquire();
      std::cout << "pong" << std::endl;
      pingsem.release();
      std::this_thread::sleep_for(std::chrono::seconds(1U));
    }
  });

  std::this_thread::sleep_for(std::chrono::seconds(6U));
  ping.request_stop();
  pong.request_stop();
}

int main() {
  barrier_task::do_task();
  std::cout << std::endl;
  latch_task::do_task();
  std::cout << std::endl;
  ping_pong();

  return EXIT_SUCCESS;
}
