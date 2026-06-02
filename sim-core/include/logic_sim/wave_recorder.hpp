#pragma once

#include <functional>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "signal.hpp"

namespace logic_sim {

/// A single waveform transition point.
struct WavePoint {
    uint64_t t = 0;   ///< Time (ticks)
    SignalValue v = SignalValue::Z;
};

/// A complete digital signal trace (list of transition points).
struct WaveSignal {
    std::string id;     ///< Signal identifier (e.g. "u1.Q0")
    std::string name;   ///< Display name
    std::vector<WavePoint> values;
};

/// Records waveform data during simulation.
///
/// Only probes that are explicitly watched are recorded.
/// Recording is done as value-change points (compressed representation).
class WaveRecorder {
public:
    WaveRecorder() = default;

    /// Start watching a node.  All value changes on this node will be recorded.
    /// @param node_index Index of the node in the circuit.
    /// @param name Display name for the signal.
    void watch(int node_index, const std::string& name);

    /// Stop watching a node.
    void unwatch(int node_index);

    /// Record a value change.  Only records if the node is being watched.
    void record(uint64_t time, int node_index, SignalValue value);

    /// Record the initial value for all watched nodes at time 0.
    /// @param get_value Callback to get the current value of a node.
    void record_initial(uint64_t time,
                        const std::function<SignalValue(int)>& get_value);

    /// Serialize all recorded signals to a JSON-like structure.
    /// Returns a vector of WaveSignal ready for serialization.
    std::vector<WaveSignal> to_signals() const;

    /// Clear all recorded data.
    void clear();

    /// How many nodes are being watched.
    size_t watched_count() const;

private:
    struct WatchedNode {
        int node_index;
        std::string name;
    };

    std::vector<WatchedNode> watched_;
    // node_index -> signal data (accumulated WavePoints)
    std::map<int, std::vector<WavePoint>> data_;
};

}  // namespace logic_sim
