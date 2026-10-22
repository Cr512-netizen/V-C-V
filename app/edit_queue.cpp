#include "app/edit_queue.h"

namespace nuc {
void EditRequestQueue::observe(bool left_down,bool right_down) {
    // Rising edges only: holding a button must not repeat an edit on every frame.
    if(left_down&&!previous_left_&&pending_.size()<max_pending)
        pending_.push_back(EditAction::break_block);
    if(right_down&&!previous_right_&&pending_.size()<max_pending)
        pending_.push_back(EditAction::place_block);
    previous_left_=left_down;
    previous_right_=right_down;
}
std::vector<EditAction> EditRequestQueue::drain() {
    std::vector<EditAction> out;
    out.swap(pending_);
    return out;
}
void EditRequestQueue::reset() noexcept {
    previous_left_=false;
    previous_right_=false;
    pending_.clear();
}
}
