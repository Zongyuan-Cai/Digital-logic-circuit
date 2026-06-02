#include "logic_sim/event_queue.hpp"

namespace logic_sim {

void EventQueue::push(const Event& e) {
    queue_.push(e);
}

void EventQueue::push(Event&& e) {
    queue_.push(std::move(e));
}

bool EventQueue::pop_next(Event& out) {
    if (queue_.empty()) return false;
    out = queue_.top();
    queue_.pop();
    return true;
}

bool EventQueue::peek(Event& out) const {
    if (queue_.empty()) return false;
    out = queue_.top();
    return true;
}

bool EventQueue::empty() const {
    return queue_.empty();
}

size_t EventQueue::size() const {
    return queue_.size();
}

void EventQueue::clear() {
    while (!queue_.empty()) queue_.pop();
}

}  // namespace logic_sim
