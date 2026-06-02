#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "device.hpp"
#include "node.hpp"
#include "signal.hpp"

namespace logic_sim {

/// A circuit is a collection of devices connected by nodes (signal nets).
///
/// It owns all devices and nodes, and maintains the wiring graph used
/// by the simulator to propagate signal changes.
class Circuit {
public:
    Circuit() = default;

    // ---- Device management ----

    /// Add a device (takes ownership).  Returns the device index.
    int add_device(std::unique_ptr<Device> dev);

    /// Find a device by its user-assigned id.  Returns nullptr if not found.
    Device* find_device(const std::string& id);
    const Device* find_device(const std::string& id) const;

    /// Get a device by index.
    Device* device(int index);
    const Device* device(int index) const;

    /// Number of devices.
    size_t device_count() const { return devices_.size(); }

    // ---- Node management ----

    /// Create a new node.  Returns its index.
    int add_node(const std::string& name = "");

    /// Get a node by index.
    Node& node(int index);
    const Node& node(int index) const;

    /// Number of nodes.
    size_t node_count() const { return nodes_.size(); }

    // ---- Wiring ----

    /// Connect two pins together, merging their nodes.
    /// If either pin already has a node, merge; otherwise create a new node.
    /// @return The (possibly new) node index.
    int connect(const std::string& dev_a, const std::string& pin_a,
                const std::string& dev_b, const std::string& pin_b);

    /// Mark nodes as probed (for waveform recording).
    void add_probe(const std::string& dev_id, const std::string& pin_id,
                   const std::string& probe_name = "");

    // ---- Validation ----

    struct ValidationError {
        enum Severity { WARNING, ERROR };
        Severity severity;
        std::string message;
        std::string detail;
    };

    /// Validate the circuit and return a list of issues.
    std::vector<ValidationError> validate() const;

    // ---- Node-to-device lookup (for event propagation) ----

    /// For a given node index, return the list of (device_index, pin_id) pairs
    /// for all input pins connected to this node.
    const std::vector<std::pair<int, std::string>>&
    node_listeners(int node_index) const;

    /// Rebuild the internal lookup structures.  Call after changing wiring.
    void rebuild();

    /// Get a reference to all nodes.
    std::vector<Node>& nodes() { return nodes_; }
    const std::vector<Node>& nodes() const { return nodes_; }

    /// Get a reference to all devices.
    std::vector<std::unique_ptr<Device>>& devices() { return devices_; }
    const std::vector<std::unique_ptr<Device>>& devices() const { return devices_; }

private:
    std::vector<std::unique_ptr<Device>> devices_;
    std::vector<Node> nodes_;
    std::map<std::string, int> device_id_to_index_;

    /// node_index -> list of (device_index, pin_id) for input pins
    std::vector<std::vector<std::pair<int, std::string>>> node_listeners_;

    /// Merge two nodes (node 'to' absorbs node 'from').
    void merge_nodes(int from_idx, int to_idx);
};

}  // namespace logic_sim
