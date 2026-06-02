#include "logic_sim/circuit.hpp"
#include <algorithm>
#include <sstream>

namespace logic_sim {

// ---- Device management ----

int Circuit::add_device(std::unique_ptr<Device> dev) {
    int index = static_cast<int>(devices_.size());
    device_id_to_index_[dev->id()] = index;
    devices_.push_back(std::move(dev));
    return index;
}

Device* Circuit::find_device(const std::string& id) {
    auto it = device_id_to_index_.find(id);
    if (it == device_id_to_index_.end()) return nullptr;
    return devices_[it->second].get();
}

const Device* Circuit::find_device(const std::string& id) const {
    auto it = device_id_to_index_.find(id);
    if (it == device_id_to_index_.end()) return nullptr;
    return devices_[it->second].get();
}

Device* Circuit::device(int index) {
    if (index < 0 || index >= static_cast<int>(devices_.size())) return nullptr;
    return devices_[index].get();
}

const Device* Circuit::device(int index) const {
    if (index < 0 || index >= static_cast<int>(devices_.size())) return nullptr;
    return devices_[index].get();
}

// ---- Node management ----

int Circuit::add_node(const std::string& name) {
    int index = static_cast<int>(nodes_.size());
    Node n;
    n.id = index;
    n.name = name;
    n.value = SignalValue::Z;
    n.prev_value = SignalValue::Z;
    nodes_.push_back(std::move(n));
    return index;
}

Node& Circuit::node(int index) {
    return nodes_.at(index);
}

const Node& Circuit::node(int index) const {
    return nodes_.at(index);
}

// ---- Wiring ----

int Circuit::connect(const std::string& dev_a, const std::string& pin_a,
                     const std::string& dev_b, const std::string& pin_b) {
    Device* da = find_device(dev_a);
    Device* db = find_device(dev_b);
    if (!da || !db) return -1;

    Pin* pa = da->pin(pin_a);
    Pin* pb = db->pin(pin_b);
    if (!pa || !pb) return -1;

    int na = da->pin_node(pin_a);
    int nb = db->pin_node(pin_b);

    // Neither has a node: create a new one
    if (na < 0 && nb < 0) {
        int node_idx = add_node();
        da->connect_pin(pin_a, node_idx);
        db->connect_pin(pin_b, node_idx);
        return node_idx;
    }

    // Only one has a node: connect the other to it
    if (na >= 0 && nb < 0) {
        db->connect_pin(pin_b, na);
        return na;
    }
    if (nb >= 0 && na < 0) {
        da->connect_pin(pin_a, nb);
        return nb;
    }

    // Both have nodes: merge them
    if (na == nb) return na;  // already connected
    merge_nodes(na, nb);
    return na;  // nb merged into na
}

void Circuit::add_probe(const std::string& dev_id, const std::string& pin_id,
                        const std::string& probe_name) {
    Device* dev = find_device(dev_id);
    if (!dev) return;
    int ni = dev->pin_node(pin_id);
    if (ni >= 0 && ni < static_cast<int>(nodes_.size())) {
        nodes_[ni].probed = true;
        if (!probe_name.empty()) {
            nodes_[ni].name = probe_name;
        } else {
            nodes_[ni].name = dev_id + "." + pin_id;
        }
    }
}

// ---- Validation ----

std::vector<Circuit::ValidationError> Circuit::validate() const {
    std::vector<ValidationError> errors;

    for (auto& dev : devices_) {
        // Check that device type is known (we can't check type validity here,
        // but we know it was created by a factory)
        for (auto& p : dev->pins()) {
            if (p.is_input()) {
                int ni = dev->pin_node(p.id);
                if (ni < 0) {
                    ValidationError ve;
                    ve.severity = ValidationError::WARNING;
                    ve.message = "Floating input";
                    std::ostringstream oss;
                    oss << "Device " << dev->id() << " pin " << p.id
                        << " is unconnected";
                    ve.detail = oss.str();
                    errors.push_back(ve);
                }
            }
        }
    }

    // Check for node driver conflicts
    for (auto& n : nodes_) {
        if (n.driver_indices.size() > 1) {
            ValidationError ve;
            ve.severity = ValidationError::WARNING;
            ve.message = "Multiple drivers on node";
            std::ostringstream oss;
            oss << "Node " << n.id << " has " << n.driver_indices.size()
                << " drivers";
            ve.detail = oss.str();
            errors.push_back(ve);
        }
    }

    return errors;
}

const std::vector<std::pair<int, std::string>>&
Circuit::node_listeners(int node_index) const {
    static const std::vector<std::pair<int, std::string>> empty;
    if (node_index < 0 || node_index >= static_cast<int>(node_listeners_.size()))
        return empty;
    return node_listeners_[node_index];
}

void Circuit::rebuild() {
    // Clear node connections
    for (auto& n : nodes_) {
        n.pin_refs.clear();
        n.driver_indices.clear();
    }

    // Build pin_refs for each node
    for (int di = 0; di < static_cast<int>(devices_.size()); ++di) {
        auto& dev = devices_[di];
        for (int pi = 0; pi < static_cast<int>(dev->pins().size()); ++pi) {
            int ni = dev->pin_node(dev->pins()[pi].id);
            if (ni >= 0 && ni < static_cast<int>(nodes_.size())) {
                Node::PinRef pr;
                pr.device_index = di;
                pr.pin_index = pi;
                nodes_[ni].pin_refs.push_back(pr);
                // If this pin is an output, it's a driver
                if (dev->pins()[pi].is_output() || dev->pins()[pi].is_bidirectional()) {
                    nodes_[ni].driver_indices.push_back(
                        nodes_[ni].pin_refs.size() - 1);
                }
            }
        }
    }

    // Build node_listeners: for each node, which (device_index, pin_id) inputs are connected
    node_listeners_.clear();
    node_listeners_.resize(nodes_.size());
    for (int di = 0; di < static_cast<int>(devices_.size()); ++di) {
        auto& dev = devices_[di];
        for (auto& p : dev->pins()) {
            if (p.is_input()) {
                int ni = dev->pin_node(p.id);
                if (ni >= 0 && ni < static_cast<int>(nodes_.size())) {
                    node_listeners_[ni].emplace_back(di, p.id);
                }
            }
        }
    }
}

// ---- Internal ----

void Circuit::merge_nodes(int from_idx, int to_idx) {
    if (from_idx == to_idx) return;
    // Move all pin references from 'from' to 'to'
    auto& from = nodes_[from_idx];
    auto& to = nodes_[to_idx];

    for (auto& pr : from.pin_refs) {
        auto* dev = device(pr.device_index);
        if (dev) {
            // Update the device's pin-to-node mapping
            auto& pin = dev->pins()[pr.pin_index];
            dev->connect_pin(pin.id, to_idx);
        }
    }

    // Mark the 'from' node as orphaned. It will remain in nodes_ as
    // an empty net (no drivers, value defaults to Z) after rebuild()
    // re-populates pin_refs from device pins. This wastes one slot
    // but avoids shifting all node indices.
    from.pin_refs.clear();
    from.driver_indices.clear();
}

}  // namespace logic_sim
