#include "logic_sim/device.hpp"
#include <stdexcept>

namespace logic_sim {

const Pin* Device::pin(const std::string& pid) const {
    for (auto& p : pins_) {
        if (p.id == pid) return &p;
    }
    return nullptr;
}

Pin* Device::pin(const std::string& pid) {
    for (auto& p : pins_) {
        if (p.id == pid) return &p;
    }
    return nullptr;
}

void Device::set_param(const std::string& key, SignalValue val) {
    sig_params_[key] = val;
}

void Device::set_param_int(const std::string& key, int val) {
    int_params_[key] = val;
}

SignalValue Device::get_param(const std::string& key, SignalValue def) const {
    auto it = sig_params_.find(key);
    return (it != sig_params_.end()) ? it->second : def;
}

int Device::get_param_int(const std::string& key, int def) const {
    auto it = int_params_.find(key);
    return (it != int_params_.end()) ? it->second : def;
}

void Device::connect_pin(const std::string& pid, int node_index) {
    pin_node_map_[pid] = node_index;
}

int Device::pin_node(const std::string& pid) const {
    auto it = pin_node_map_.find(pid);
    return (it != pin_node_map_.end()) ? it->second : -1;
}

SignalValue Device::input(const std::string& pid,
                           const std::vector<Node>& nodes) const {
    int ni = pin_node(pid);
    if (ni < 0) return SignalValue::X;
    return nodes[ni].value;
}

SignalValue Device::prev_input(const std::string& pid,
                                const std::vector<Node>& nodes) const {
    int ni = pin_node(pid);
    if (ni < 0) return SignalValue::X;
    return nodes[ni].prev_value;
}

std::vector<SignalValue> Device::input_bus(
    const std::vector<std::string>& pids,
    const std::vector<Node>& nodes) const {
    std::vector<SignalValue> result;
    result.reserve(pids.size());
    for (auto& pid : pids) {
        result.push_back(input(pid, nodes));
    }
    return result;
}

void Device::add_pin(const std::string& pid, const std::string& name,
                     Pin::Direction dir, Pin::ActiveLevel al,
                     const std::string& role) {
    pins_.push_back(Pin(pid, name, dir, al, role));
}

bool Device::rising_edge(const std::string& pid,
                          const std::vector<Node>& nodes) const {
    SignalValue prev = prev_input(pid, nodes);
    SignalValue curr = input(pid, nodes);
    return prev == SignalValue::ZERO && curr == SignalValue::ONE;
}

bool Device::falling_edge(const std::string& pid,
                           const std::vector<Node>& nodes) const {
    SignalValue prev = prev_input(pid, nodes);
    SignalValue curr = input(pid, nodes);
    return prev == SignalValue::ONE && curr == SignalValue::ZERO;
}

bool Device::any_edge(const std::string& pid,
                       const std::vector<Node>& nodes) const {
    SignalValue prev = prev_input(pid, nodes);
    SignalValue curr = input(pid, nodes);
    return is_strong(prev) && is_strong(curr) && prev != curr;
}

std::string Device::trigger_edge() const {
    // Default is "rising"; check for "edge" param
    auto it = sig_params_.find("edge");
    if (it != sig_params_.end()) {
        if (it->second == SignalValue::ZERO) return "falling";
    }
    auto it2 = int_params_.find("edge");
    if (it2 != int_params_.end() && it2->second == 0) return "falling";
    return "rising";
}

}  // namespace logic_sim
