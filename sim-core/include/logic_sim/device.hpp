#pragma once

#include <functional>
#include <map>
#include <string>
#include <vector>

#include "pin.hpp"
#include "signal.hpp"
#include "node.hpp"

namespace logic_sim {

/// Abstract base class for all simulation devices (gates, flip-flops, chips).
///
/// Each device owns its pins and internal state.  It reads input values
/// from the circuit nodes and produces output values during evaluation.
class Device {
public:
    Device(const std::string& id, const std::string& type)
        : id_(id), type_(type) {}

    virtual ~Device() = default;

    // ---- Accessors ----

    const std::string& id()   const { return id_; }
    const std::string& type() const { return type_; }

    const std::vector<Pin>& pins() const { return pins_; }
    std::vector<Pin>& pins() { return pins_; }

    /// Look up a pin by id.  Returns nullptr if not found.
    const Pin* pin(const std::string& pid) const;
    Pin* pin(const std::string& pid);

    /// Get or set a device parameter (e.g. "delay", "initial", "edge").
    void set_param(const std::string& key, SignalValue val);
    void set_param_int(const std::string& key, int val);
    SignalValue get_param(const std::string& key, SignalValue def = SignalValue::ZERO) const;
    int get_param_int(const std::string& key, int def = 0) const;

    // ---- Node connection ----

    /// Map a pin of this device to a node index in the circuit.
    void connect_pin(const std::string& pid, int node_index);

    /// Get the node index for a pin (-1 if unconnected).
    int pin_node(const std::string& pid) const;

    // ---- Input reading helpers ----

    /// Read the current value on an input pin (from its node).
    SignalValue input(const std::string& pid,
                      const std::vector<Node>& nodes) const;

    /// Read the previous value on an input pin (for edge detection).
    SignalValue prev_input(const std::string& pid,
                           const std::vector<Node>& nodes) const;

    /// Read multi-bit input from a bus of pins (e.g. "D0","D1","D2","D3").
    std::vector<SignalValue> input_bus(
        const std::vector<std::string>& pids,
        const std::vector<Node>& nodes) const;

    // ---- Core evaluation ----

    /// Evaluate the device: read inputs from nodes, compute outputs.
    ///
    /// @param nodes  Current circuit node values.
    /// @param changed_input  Which input pin triggered this evaluation
    ///                       (empty string for initial evaluation).
    /// @return  Map of output_pin_id -> new_value for outputs that should be
    ///          scheduled.  Only include outputs that actually changed.
    virtual std::map<std::string, SignalValue> eval(
        const std::vector<Node>& nodes,
        const std::string& changed_input) = 0;

    /// Reset internal state to initial conditions.
    virtual void reset() {}

protected:
    std::string id_;
    std::string type_;
    std::vector<Pin> pins_;
    std::map<std::string, int> pin_node_map_;   ///< pin_id -> node_index

    /// Device parameters (delay, initial values, etc.)
    std::map<std::string, int> int_params_;
    std::map<std::string, SignalValue> sig_params_;

    /// Add a pin definition.
    void add_pin(const std::string& pid, const std::string& name,
                 Pin::Direction dir, Pin::ActiveLevel al = Pin::ACTIVE_HIGH,
                 const std::string& role = "");

    /// Detect a rising edge on a pin (prev=0, curr=1).
    bool rising_edge(const std::string& pid, const std::vector<Node>& nodes) const;

    /// Detect a falling edge on a pin (prev=1, curr=0).
    bool falling_edge(const std::string& pid, const std::vector<Node>& nodes) const;

    /// Detect any edge (rising or falling).
    bool any_edge(const std::string& pid, const std::vector<Node>& nodes) const;

    /// Get the configured trigger edge type ("rising" or "falling", default "rising").
    std::string trigger_edge() const;

public:
    /// Default delay for this device.
    int delay() const { return get_param_int("delay", 1); }
};

}  // namespace logic_sim
