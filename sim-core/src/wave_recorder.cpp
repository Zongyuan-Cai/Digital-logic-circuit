#include "logic_sim/wave_recorder.hpp"
#include <algorithm>

namespace logic_sim {

void WaveRecorder::watch(int node_index, const std::string& name) {
    // Avoid duplicates
    for (auto& w : watched_) {
        if (w.node_index == node_index) {
            w.name = name;
            return;
        }
    }
    watched_.push_back({node_index, name});
    data_[node_index] = {};  // ensure entry exists
}

void WaveRecorder::unwatch(int node_index) {
    watched_.erase(
        std::remove_if(watched_.begin(), watched_.end(),
                       [node_index](const WatchedNode& w) {
                           return w.node_index == node_index;
                       }),
        watched_.end());
    data_.erase(node_index);
}

void WaveRecorder::record(uint64_t time, int node_index, SignalValue value) {
    auto it = data_.find(node_index);
    if (it == data_.end()) return;  // not watched

    auto& points = it->second;
    // Deduplicate: skip if value hasn't actually changed from the last recorded point
    if (!points.empty() && points.back().v == value) return;

    points.push_back({time, value});
}

void WaveRecorder::record_initial(uint64_t time,
                                   const std::function<SignalValue(int)>& get_value) {
    for (auto& w : watched_) {
        SignalValue v = get_value(w.node_index);
        data_[w.node_index].push_back({time, v});
    }
}

std::vector<WaveSignal> WaveRecorder::to_signals() const {
    std::vector<WaveSignal> result;
    for (auto& w : watched_) {
        WaveSignal ws;
        ws.id = std::to_string(w.node_index);
        ws.name = w.name;
        auto it = data_.find(w.node_index);
        if (it != data_.end()) {
            ws.values = it->second;
        }
        result.push_back(std::move(ws));
    }
    return result;
}

void WaveRecorder::clear() {
    data_.clear();
    watched_.clear();
}

size_t WaveRecorder::watched_count() const {
    return watched_.size();
}

}  // namespace logic_sim
