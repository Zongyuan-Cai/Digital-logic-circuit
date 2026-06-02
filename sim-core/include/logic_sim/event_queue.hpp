#pragma once

#include <cstdint>
#include <functional>
#include <queue>
#include <string>
#include <vector>

#include "signal.hpp"

namespace logic_sim {

/// A simulation event: a node's value changed (or will change) at a given time.
struct Event {
    uint64_t time = 0;          ///< Scheduled time (ticks)
    int node_index = -1;        ///< Which node changed
    SignalValue new_value = SignalValue::Z;  ///< New value
    int source_device = -1;     ///< Which device produced this event

    // For priority_queue: earlier time = higher priority.
    // If times are equal, break tie by node_index for determinism.
    bool operator<(const Event& other) const {
        if (time != other.time) return time > other.time;   // earlier time first
        return node_index > other.node_index;                // smaller index first
    }
};

/// Minimal event queue for event-driven simulation.
class EventQueue {
public:
    EventQueue() = default;

    /// Push an event into the queue.
    void push(const Event& e);

    /// Push an event (move semantics).
    void push(Event&& e);

    /// Pop and return the next (earliest) event.
    /// Returns false if the queue is empty.
    bool pop_next(Event& out);

    /// Peek at the next event without removing it.
    /// Returns false if the queue is empty.
    bool peek(Event& out) const;

    /// Check whether the queue is empty.
    bool empty() const;

    /// Number of events remaining.
    size_t size() const;

    /// Clear all events.
    void clear();

private:
    std::priority_queue<Event> queue_;
};

}  // namespace logic_sim
