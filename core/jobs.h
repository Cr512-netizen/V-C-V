#pragma once
#include <TaskScheduler.h>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <type_traits>
#include <utility>
#include <vector>

namespace nuc {
// Thin owner of an enkiTS scheduler. Tasks never touch OpenGL and never create
// threads of their own: the pool size is fixed at construction.
//
// One producer: submit(), wait_idle() and parallel_for() are meant to be called
// from the thread that owns this object (in the game, the main thread). Tasks
// themselves may run on any worker.
class Jobs final {
public:
    // threads == 0 means "as many as the machine has", clamped to [1, 64].
    // The number counts the calling thread as one participant, as enkiTS does.
    explicit Jobs(unsigned threads = 0);
    ~Jobs();
    Jobs(const Jobs&) = delete;
    Jobs& operator=(const Jobs&) = delete;

    [[nodiscard]] unsigned thread_count() const noexcept { return threads_; }
    [[nodiscard]] std::size_t pending_count() const;

    // Runs fn(0) .. fn(count - 1) across the pool and returns when all are done.
    void parallel_for(std::size_t count, const std::function<void(std::size_t)>& fn);

    // Queues fn and returns immediately. The task outlives this call but never
    // outlives the Jobs object: ~Jobs waits for everything still running.
    // Move-only callables are allowed; the pool only needs to be able to call them.
    template <typename Fn>
    void submit(Fn&& fn) {
        auto body = std::make_shared<std::decay_t<Fn>>(std::forward<Fn>(fn));
        submit_task([body] { (*body)(); });
    }

    // Waits for every submitted task and releases their bookkeeping.
    void wait_idle();

private:
    void submit_task(std::function<void()> body);
    void retire_finished();  // caller holds mutex_

    enki::TaskScheduler scheduler_;
    mutable std::mutex mutex_;
    std::vector<std::unique_ptr<enki::TaskSet>> owned_;
    unsigned threads_{};
};
}
