#pragma once
#include "app/edit_mode.h"
#include <cstddef>
#include <vector>

namespace nuc {
// Buffers pointer presses between logic ticks so world edits happen only inside a tick.
// Polling stays in the frame loop; this class owns no clock, thread or platform state,
// which is what makes the applied edit sequence reproducible from the tick alone.
class EditRequestQueue final {
public:
    // Bounded, so a held or stuck button can never grow the queue without limit.
    static constexpr std::size_t max_pending=8;
    // Called once per rendered frame with the current button state.
    void observe(bool left_down,bool right_down);
    // Called once per logic tick; returns the queued edits in arrival order.
    [[nodiscard]] std::vector<EditAction> drain();
    [[nodiscard]] std::size_t pending() const noexcept { return pending_.size(); }
    void reset() noexcept;
private:
    bool previous_left_{},previous_right_{};
    std::vector<EditAction> pending_;
};
}
