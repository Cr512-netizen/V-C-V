#include "core/jobs.h"
#include <algorithm>
#include <thread>

namespace nuc {
namespace {
constexpr unsigned max_threads = 64;

unsigned resolve_threads(unsigned requested) {
    const unsigned available = std::max(1U, std::thread::hardware_concurrency());
    const unsigned wanted = requested == 0 ? available : requested;
    return std::clamp(wanted, 1U, max_threads);
}
}

Jobs::Jobs(unsigned threads) : threads_(resolve_threads(threads)) {
    scheduler_.Initialize(threads_);
}

Jobs::~Jobs() {
    // Every submitted task may capture pointers owned by the caller, so nothing
    // may still be running when this object dies.
    scheduler_.WaitforAll();
    owned_.clear();
}

std::size_t Jobs::pending_count() const {
    const std::lock_guard lock(mutex_);
    return owned_.size();
}

void Jobs::retire_finished() {
    const auto finished = [](const std::unique_ptr<enki::TaskSet>& task) { return task->GetIsComplete(); };
    owned_.erase(std::remove_if(owned_.begin(), owned_.end(), finished), owned_.end());
}

void Jobs::parallel_for(std::size_t count, const std::function<void(std::size_t)>& fn) {
    if (count == 0) return;
    enki::TaskSet task(static_cast<std::uint32_t>(count),
                       [&fn](enki::TaskSetPartition range, std::uint32_t) {
                           for (auto index = range.start; index < range.end; ++index) fn(index);
                       });
    // Small sets would be split into partitions cheaper to run inline.
    task.m_MinRange = count < 8 ? static_cast<std::uint32_t>(count) : 1;
    scheduler_.AddTaskSetToPipe(&task);
    scheduler_.WaitforTask(&task);
}

void Jobs::submit_task(std::function<void()> fn) {
    auto task = std::make_unique<enki::TaskSet>(
        1, [body = std::move(fn)](enki::TaskSetPartition, std::uint32_t) { body(); });
    const std::lock_guard lock(mutex_);
    retire_finished();
    auto* raw = task.get();
    owned_.push_back(std::move(task));
    scheduler_.AddTaskSetToPipe(raw);
}

void Jobs::wait_idle() {
    scheduler_.WaitforAll();
    const std::lock_guard lock(mutex_);
    owned_.clear();
}
}
