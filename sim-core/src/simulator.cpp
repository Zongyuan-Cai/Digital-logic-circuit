#include "logic_sim/simulator.hpp"
#include <algorithm>
#include <sstream>

namespace logic_sim {

void Simulator::load_circuit(std::unique_ptr<Circuit> circuit) {
    circuit_ = std::move(circuit);
}

void Simulator::load_circuit(Circuit&& circuit) {
    circuit_ = std::make_unique<Circuit>(std::move(circuit));
}

void Simulator::reset() {
    event_queue_.clear();
    wave_recorder_.clear();
    current_time_ = 0;
    events_processed_ = 0;

    if (circuit_) {
        circuit_->rebuild();
        // Reset device states and output nodes. Input-only nodes (no drivers)
        // retain their user-set values.
        for (auto& n : circuit_->nodes()) {
            if (n.has_driver()) {
                n.value = SignalValue::Z;
                n.prev_value = SignalValue::Z;
            }
        }
        for (auto& dev : circuit_->devices()) {
            dev->reset();
        }
    }
}

SimResult Simulator::run(const SimOptions& opts) {
    SimResult result;
    options_ = opts;

    if (!circuit_) {
        result.status = SimResult::ERROR;
        result.errors.push_back("No circuit loaded");
        return result;
    }

    try {
        reset();
        initialize();
        record_initial_waveform();

        // Main event loop
        while (!event_queue_.empty()) {
            Event ev;
            if (!event_queue_.pop_next(ev)) break;

            if (ev.time > opts.max_ticks) {
                result.warnings.push_back("Max ticks reached");
                break;
            }

            current_time_ = ev.time;
            ++events_processed_;

            if (events_processed_ > opts.max_events) {
                result.warnings.push_back("Max events reached");
                break;
            }

            process_event(ev);
        }

        result.status = SimResult::OK;
        result.ticks_elapsed = current_time_;
        result.events_processed = events_processed_;
        record_final_nodes(result);
        result.waveform = wave_recorder_.to_signals();

    } catch (const std::exception& ex) {
        result.status = SimResult::ERROR;
        result.errors.push_back(ex.what());
    }

    return result;
}

bool Simulator::step() {
    if (!circuit_) return false;
    if (event_queue_.empty()) return false;

    Event ev;
    if (!event_queue_.pop_next(ev)) return false;

    current_time_ = ev.time;
    ++events_processed_;
    process_event(ev);
    return true;
}

std::vector<WaveSignal> Simulator::get_waveform() const {
    return wave_recorder_.to_signals();
}

// ---- Private ----

void Simulator::initialize() {
    circuit_->rebuild();

    // Set up waveform recording
    std::vector<std::tuple<int, std::string>> nodes_to_watch;

    if (options_.record_all) {
        for (size_t i = 0; i < circuit_->node_count(); ++i) {
            auto& n = circuit_->nodes()[i];
            wave_recorder_.watch(static_cast<int>(i), n.name.empty()
                ? ("node_" + std::to_string(i)) : n.name);
        }
    } else {
        for (auto& spec : options_.record) {
            // spec format: "device_id.pin_id" or "node_name"
            auto dot = spec.find('.');
            if (dot != std::string::npos) {
                std::string dev_id = spec.substr(0, dot);
                std::string pin_id = spec.substr(dot + 1);
                Device* dev = circuit_->find_device(dev_id);
                if (dev) {
                    int ni = dev->pin_node(pin_id);
                    if (ni >= 0) {
                        wave_recorder_.watch(ni, spec);
                    }
                }
            }
        }
        // Also watch probed nodes
        for (size_t i = 0; i < circuit_->node_count(); ++i) {
            auto& n = circuit_->nodes()[i];
            if (n.probed) {
                wave_recorder_.watch(static_cast<int>(i),
                    n.name.empty() ? ("node_" + std::to_string(i)) : n.name);
            }
        }
    }

    // Compute initial outputs for all devices (in device order)
    // We do multiple passes to allow combinational settling
    for (int pass = 0; pass < 10; ++pass) {
        bool changed = false;
        for (int di = 0; di < static_cast<int>(circuit_->device_count()); ++di) {
            auto* dev = circuit_->device(di);
            auto outputs = dev->eval(circuit_->nodes(), "");
            for (auto& [pid, new_val] : outputs) {
                int ni = dev->pin_node(pid);
                if (ni < 0) continue;
                auto& node = circuit_->nodes()[ni];
                SignalValue old_val = node.value;
                // Node resolution (per design doc Section 5.3):
                //   No driver → Z; Single strong driver → that value;
                //   Multiple consistent drivers → that value;
                //   Conflicting drivers → X (DRIVER_CONFLICT)
                if (node.driver_indices.size() > 1) {
                    // Collect resolved values from all drivers of this node
                    // after they have been evaluated this pass.
                    // Since we are iterating devices in order, we check
                    // whether any already-computed driver disagrees with new_val.
                    bool conflict = false;
                    for (size_t dri_idx : node.driver_indices) {
                        if (dri_idx >= node.pin_refs.size()) continue;
                        auto& pr = node.pin_refs[dri_idx];
                        if (pr.device_index == di) continue; // skip self
                        // Read the other driver's current output value
                        // (already settled from a previous device in this pass)
                        auto* other_dev = circuit_->device(pr.device_index);
                        if (!other_dev) continue;
                        int other_ni = other_dev->pin_node(
                            other_dev->pins()[pr.pin_index].id);
                        if (other_ni != ni) continue;
                        SignalValue other_val = circuit_->nodes()[ni].value;
                        if (is_strong(new_val) && is_strong(other_val) &&
                            new_val != other_val) {
                            conflict = true;
                            break;
                        }
                    }
                    if (conflict) {
                        new_val = SignalValue::X;
                    }
                }
                if (new_val != old_val) {
                    node.value = new_val;
                    event_queue_.push(Event{0, ni, new_val, di});
                    changed = true;
                }
            }
        }
        if (!changed) break;
    }

    // Record initial values
    wave_recorder_.record_initial(0, [this](int ni) -> SignalValue {
        return circuit_->nodes()[ni].value;
    });

    // Process initial events at time 0
    while (!event_queue_.empty()) {
        Event ev;
        if (!event_queue_.pop_next(ev)) break;
        if (ev.time > 0) {
            // Push back for later
            event_queue_.push(ev);
            break;
        }
        process_event(ev);
    }
}

void Simulator::process_event(const Event& ev) {
    auto& nodes = circuit_->nodes();
    if (ev.node_index < 0 || ev.node_index >= static_cast<int>(nodes.size()))
        return;

    auto& node = nodes[ev.node_index];

    // Save previous value, update to new
    node.prev_value = node.value;
    node.value = ev.new_value;

    // Record waveform
    wave_recorder_.record(ev.time, ev.node_index, ev.new_value);

    // Notify devices whose input pins are connected to this node
    for (auto& [device_index, pin_id] :
         circuit_->node_listeners(ev.node_index)) {
        auto* dev = circuit_->device(device_index);
        if (!dev) continue;

        // Evaluate the device
        auto outputs = dev->eval(nodes, pin_id);

        // Schedule output changes
        for (auto& [out_pin_id, new_val] : outputs) {
            int out_ni = dev->pin_node(out_pin_id);
            if (out_ni < 0) continue;

            auto& out_node = nodes[out_ni];

            // Skip if value hasn't changed
            if (new_val == out_node.value && new_val == out_node.prev_value)
                continue;

            uint64_t t = ev.time + dev->delay();
            event_queue_.push(Event{t, out_ni, new_val, device_index});
        }
    }
}

void Simulator::record_final_nodes(SimResult& result) {
    if (!circuit_) return;
    for (auto& dev : circuit_->devices()) {
        for (auto& p : dev->pins()) {
            if (p.is_output()) {
                int ni = dev->pin_node(p.id);
                if (ni >= 0) {
                    std::string key = dev->id() + "." + p.id;
                    result.final_nodes[key] =
                        signal_to_string(circuit_->nodes()[ni].value);
                }
            }
        }
    }
}

void Simulator::record_initial_waveform() {
    wave_recorder_.record_initial(0, [this](int ni) -> SignalValue {
        if (ni >= 0 && ni < static_cast<int>(circuit_->nodes().size()))
            return circuit_->nodes()[ni].value;
        return SignalValue::Z;
    });
}

void Simulator::warn(const std::string& msg) {
    if (warning_cb_) warning_cb_(msg);
}

}  // namespace logic_sim
