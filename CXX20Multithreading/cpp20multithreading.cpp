// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/cpp-experiments

#include <array>
#include <barrier>
#include <coroutine>
#include <future>
#include <iostream>
#include <latch>
#include <syncstream>
#include <thread>

#define IMPLEMENT_SIMPLE_COROUTINE_INTERNAL_PROMISE_TYPE(input_task_name) \
    struct promise_type {                                                 \
        static input_task_name get_return_object()                        \
        {                                                                 \
            return {};                                                    \
        }                                                                 \
        static std::suspend_never initial_suspend()                       \
        {                                                                 \
            return {};                                                    \
        }                                                                 \
        static std::suspend_never final_suspend() noexcept                \
        {                                                                 \
            return {};                                                    \
        }                                                                 \
        void return_void()                                                \
        {                                                                 \
        }                                                                 \
        void unhandled_exception()                                        \
        {                                                                 \
        }                                                                 \
    }

struct barrier_task {
    IMPLEMENT_SIMPLE_COROUTINE_INTERNAL_PROMISE_TYPE(barrier_task);

    static barrier_task do_task()
    {
        std::cout << "performing barrier coroutine task..." << std::endl;

        constexpr std::uint8_t max_threads = 5U;
        std::array<std::jthread, max_threads> works {};

        std::barrier work_barrier(std::size(works), []() noexcept {
            std::cout << "All threads has finished their work." << std::endl;
        });

        for (auto it = std::begin(works); it != std::end(works); ++it)
        {
            *it = std::jthread([&work_barrier, max_threads, index = 1U + std::distance(std::begin(works), it)]() noexcept {
                std::osyncstream(std::cout) << "Thread " << index << " is starting his work." << std::endl;

                std::this_thread::sleep_for(std::chrono::seconds(std::rand() % std::clamp<std::uint8_t>(index, 1U, 1U + max_threads)));
                std::osyncstream(std::cout) << "Thread " << index << " reached step 1." << std::endl;
                work_barrier.arrive_and_wait();

                std::this_thread::sleep_for(std::chrono::seconds(std::rand() % std::clamp<std::uint8_t>(index, 1U, 1U + max_threads)));
                std::osyncstream(std::cout) << "Thread " << index << " reached step 2." << std::endl;
                work_barrier.arrive_and_wait();

                std::this_thread::sleep_for(std::chrono::seconds(std::rand() % std::clamp<std::uint8_t>(index, 1U, 1U + max_threads)));
                std::osyncstream(std::cout) << "Thread " << index << " reached step 3." << std::endl;
                work_barrier.arrive_and_wait();

                std::osyncstream(std::cout) << "Thread " << index << " finished." << std::endl;
                work_barrier.arrive_and_wait();
            });
        }

        for (auto& work: works)
        {
            work.join();
        }

        co_return;
    }
};

struct latch_task {
    IMPLEMENT_SIMPLE_COROUTINE_INTERNAL_PROMISE_TYPE(latch_task);

    static latch_task do_task()
    {
        std::cout << "performing latch coroutine task..." << std::endl;

        constexpr std::uint8_t max_threads = 5U;
        std::array<std::jthread, max_threads> works {};

        std::latch work_latch(std::size(works));
        std::latch exit_latch(1U);

        for (auto it = std::begin(works); it != std::end(works); ++it)
        {
            *it = std::jthread([&work_latch, &exit_latch, max_threads, index = 1U + std::distance(std::begin(works), it)]() noexcept {
                std::osyncstream(std::cout) << "Thread " << index << " is starting his work." << std::endl;

                std::this_thread::sleep_for(std::chrono::seconds(std::rand() % std::clamp<std::uint8_t>(index, 1U, 1U + max_threads)));
                std::osyncstream(std::cout) << "Thread " << index << " completed his work and will wait for the signal to finish." << std::endl;
                work_latch.count_down();

                exit_latch.wait();

                std::osyncstream(std::cout) << "Thread " << index << " finished." << std::endl;
            });
        }

        work_latch.wait();
        std::cout << "All threads has finished their work. Emitting the signal to finish." << std::endl;
        exit_latch.count_down();

        for (auto& work: works)
        {
            work.join();
        }

        co_return;
    }
};

int main()
{
    barrier_task::do_task();
    std::cout << std::endl;
    latch_task::do_task();

    return EXIT_SUCCESS;
}
