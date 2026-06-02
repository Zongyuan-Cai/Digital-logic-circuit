#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "circuit.hpp"
#include "event_queue.hpp"
#include "signal.hpp"
#include "wave_recorder.hpp"

namespace logic_sim {

/// Simulation configuration / options.
struct SimOptions {
    uint64_t max_ticks   = 1000;       ///< Max simulation ticks before stopping
    uint64_t max_events  = 100000;     ///< Max events processed before stopping
    bool     record_all  = false;      ///< Record all nodes (if true, ignores record list)
    std::vector<std::string> record;   ///< Specific nodes to record (device_id.pin_id)
    uint64_t default_delay = 1;        ///< Default gate delay in ticks
    std::string floating_input = "X";  ///< How to treat floating inputs ("X" or "Z")
};

/// Result returned from a simulation run.
struct SimResult {
    enum Status { OK, ERROR, TIMEOUT };
    Status status = OK;

    std::vector<std::string> errors;
    std::vector<std::string> warnings;

    /// Final node values, keyed by description e.g. "u1.Q0".
    std::map<std::string, std::string> final_nodes;

    /// Waveform data (value-change points).
    std::vector<WaveSignal> waveform;

    /// Simulation statistics.
    uint64_t ticks_elapsed  = 0;
    uint64_t events_processed = 0;
};

/// The main event-driven simulator.
///
/// Usage:
///   Simulator sim;
///   sim.load_circuit(circuit);   // circuit already built programmatically
///   SimResult r = sim.run(options);
class Simulator {
public:
    Simulator() = default;

    /// Load a pre-built circuit into the simulator.
    void load_circuit(std::unique_ptr<Circuit> circuit);
    void load_circuit(Circuit&& circuit);

    /// Reset the simulator state (clears event queue, resets all devices).
    void reset();

    /// Run the simulation with the given options.
    SimResult run(const SimOptions& opts = SimOptions{});

    /// Run a single simulation step (process one event or one tick).
    /// Returns false if no more events.
    bool step();

    /// Get the waveform data from the last run.
    std::vector<WaveSignal> get_waveform() const;

    /// Set a callback for warnings / errors during simulation.
    using WarningCallback = std::function<void(const std::string&)>;
    void set_warning_callback(WarningCallback cb) { warning_cb_ = std::move(cb); }

    /// Access the underlying circuit.
    Circuit* circuit() { return circuit_.get(); }
    const Circuit* circuit() const { return circuit_.get(); }

private:
    std::unique_ptr<Circuit> circuit_;
    EventQueue event_queue_;
    WaveRecorder wave_recorder_;
    SimOptions options_;
    uint64_t current_time_ = 0;
    uint64_t events_processed_ = 0;

    WarningCallback warning_cb_;

    /// Compute initial outputs for all devices and schedule events.
    void initialize();

    /// Process a single event.
    void process_event(const Event& ev);

    /// Record final node values into the result.
    void record_final_nodes(SimResult& result);

    /// Record the initial waveform snapshot for all watched nodes.
    void record_initial_waveform();

    /// Emit a warning.
    void warn(const std::string& msg);
};

}  // namespace logic_sim
