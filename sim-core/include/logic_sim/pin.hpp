#pragma once

#include <string>

namespace logic_sim {

/// Describes a single pin on a device.
struct Pin {
    /// Direction of a pin.
    enum Direction {
        INPUT = 0,
        OUTPUT = 1,
        BIDIRECTIONAL = 2
    };

    /// Active level of a pin (for display / metadata purposes).
    enum ActiveLevel {
        ACTIVE_HIGH = 0,
        ACTIVE_LOW  = 1
    };

    /// Unique pin identifier (e.g. "A", "Q0", "CP").
    std::string id;

    /// Human-readable name (may match id or be more descriptive).
    std::string name;

    /// Direction: input, output, or bidirectional.
    Direction direction = INPUT;

    /// Active level for display.
    ActiveLevel active_level = ACTIVE_HIGH;

    /// Semantic role of this pin (e.g. "address", "enable", "data", "clock", "clear").
    std::string role;

    /// Index of the node this pin is connected to (-1 = unconnected).
    int node_index = -1;

    Pin() = default;
    Pin(std::string id, std::string name, Direction dir,
        ActiveLevel al = ACTIVE_HIGH, std::string role = "")
        : id(std::move(id))
        , name(std::move(name))
        , direction(dir)
        , active_level(al)
        , role(std::move(role)) {}

    bool is_input() const { return direction == INPUT; }
    bool is_output() const { return direction == OUTPUT; }
    bool is_bidirectional() const { return direction == BIDIRECTIONAL; }
    bool is_active_low() const { return active_level == ACTIVE_LOW; }
};

}  // namespace logic_sim
