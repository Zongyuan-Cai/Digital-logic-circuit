#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "signal.hpp"

namespace logic_sim {

/// A node is a connection point (signal net) in the circuit.
/// Multiple pins from different devices can connect to the same node.
///
/// Node value resolution (per design doc Section 5.3):
///   - No driver  → Z
///   - Single driver 0/1 → 0/1
///   - Multiple drivers consistent → that value
///   - Multiple drivers conflicting → X + DRIVER_CONFLICT
///   - Driver is X → X if no strong driver determines otherwise
struct Node {
    int id = -1;
    std::string name;  ///< Optional display name

    /// Current resolved value of the node.
    SignalValue value = SignalValue::Z;

    /// Previous value (used for edge detection by sequential devices).
    SignalValue prev_value = SignalValue::Z;

    /// List of (device_index, pin_index) pairs for every pin connected to this node.
    /// The first element is (device_index, pin_index).
    struct PinRef {
        int device_index = -1;
        int pin_index   = -1;
    };
    std::vector<PinRef> pin_refs;

    /// Indices into pin_refs for pins that are drivers (output or bidirectional).
    std::vector<size_t> driver_indices;

    /// Whether this node is being probed (recorded in waveform).
    bool probed = false;

    /// Return true if this node has any driver.
    bool has_driver() const { return !driver_indices.empty(); }
};

}  // namespace logic_sim
