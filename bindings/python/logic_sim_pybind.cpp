#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>

#include "logic_sim/signal.hpp"
#include "logic_sim/device.hpp"
#include "logic_sim/circuit.hpp"
#include "logic_sim/simulator.hpp"
#include "logic_sim/devices/gates.hpp"
#include "logic_sim/devices/flip_flops.hpp"
#include "logic_sim/devices/chips.hpp"

namespace py = pybind11;
using namespace logic_sim;

// Helper: build a circuit from a JSON-like dict
// The Python side passes circuit dict, we construct Circuit and run simulation.

namespace {

std::unique_ptr<Device> create_device(const std::string& type, const std::string& id,
                                       int num_inputs = 2) {
    // Try gates
    auto dev = create_gate(type, id, num_inputs > 0 ? num_inputs : 2);
    if (dev) return dev;
    // Try flip-flops
    dev = create_flip_flop(type, id);
    if (dev) return dev;
    // Try chips
    dev = create_chip(type, id);
    if (dev) return dev;

    throw std::runtime_error("Unknown device type: " + type);
}

}  // namespace

// Build a circuit from Python dict and run simulation
py::dict run_simulation(const py::dict& circuit_dict, const py::dict& options_dict) {
    auto circuit = std::make_unique<Circuit>();

    // Parse devices
    auto devices = circuit_dict["devices"].cast<py::list>();
    for (auto item : devices) {
        auto d = item.cast<py::dict>();
        std::string id = d["id"].cast<std::string>();
        std::string type = d["type"].cast<std::string>();

        auto dev = create_device(type, id);

        // Set params
        if (d.contains("params")) {
            auto params = d["params"].cast<py::dict>();
            for (auto [k, v] : params) {
                auto ks = k.cast<std::string>();
                if (py::isinstance<py::int_>(v)) {
                    dev->set_param_int(ks, v.cast<int>());
                } else if (py::isinstance<py::str>(v)) {
                    dev->set_param(ks, signal_from_string(v.cast<std::string>()));
                }
            }
        }

        // Create nodes for pins
        for (auto& p : dev->pins()) {
            int ni = circuit->add_node(p.name);
            dev->connect_pin(p.id, ni);
        }

        circuit->add_device(std::move(dev));
    }

    // Parse wires
    if (circuit_dict.contains("wires")) {
        auto wires = circuit_dict["wires"].cast<py::list>();
        for (auto item : wires) {
            auto w = item.cast<py::dict>();
            auto from_obj = w["from"].cast<py::dict>();
            auto to_obj = w["to"].cast<py::dict>();

            std::string from_dev = from_obj["device"].cast<std::string>();
            std::string from_pin = from_obj["pin"].cast<std::string>();
            std::string to_dev = to_obj["device"].cast<std::string>();
            std::string to_pin = to_obj["pin"].cast<std::string>();

            circuit->connect(from_dev, from_pin, to_dev, to_pin);
        }
    }

    circuit->rebuild();

    // Parse options
    SimOptions opts;
    if (options_dict.contains("max_ticks"))
        opts.max_ticks = options_dict["max_ticks"].cast<uint64_t>();
    if (options_dict.contains("max_events"))
        opts.max_events = options_dict["max_events"].cast<uint64_t>();
    if (options_dict.contains("record_all"))
        opts.record_all = options_dict["record_all"].cast<bool>();
    if (options_dict.contains("record"))
        opts.record = options_dict["record"].cast<std::vector<std::string>>();
    if (options_dict.contains("default_delay"))
        opts.default_delay = options_dict["default_delay"].cast<uint64_t>();

    Simulator sim;
    sim.load_circuit(std::move(circuit));
    auto result = sim.run(opts);

    // Build result dict
    py::dict out;
    out["status"] = (result.status == SimResult::OK) ? "ok" : "error";

    py::list errors;
    for (auto& e : result.errors) errors.append(e);
    out["errors"] = errors;

    py::list warnings;
    for (auto& w : result.warnings) warnings.append(w);
    out["warnings"] = warnings;

    py::dict final;
    for (auto& [k, v] : result.final_nodes) final[k.c_str()] = v;
    out["final_nodes"] = final;

    py::dict waveform;
    waveform["time_unit"] = "tick";
    py::list signals;
    for (auto& ws : result.waveform) {
        py::dict sig;
        sig["id"] = ws.id;
        sig["name"] = ws.name;
        py::list values;
        for (auto& wp : ws.values) {
            py::dict vp;
            vp["t"] = wp.t;
            vp["v"] = signal_to_string(wp.v);
            values.append(vp);
        }
        sig["values"] = values;
        signals.append(sig);
    }
    waveform["signals"] = signals;
    out["waveform"] = waveform;

    out["ticks_elapsed"] = result.ticks_elapsed;
    out["events_processed"] = result.events_processed;

    return out;
}

PYBIND11_MODULE(logic_sim, m) {
    m.doc() = "Digital Logic Circuit Simulation Core";

    m.def("run", &run_simulation,
          py::arg("circuit"), py::arg("options") = py::dict(),
          "Run a circuit simulation.\n\n"
          "Args:\n"
          "    circuit: dict with 'devices' and optional 'wires'.\n"
          "    options: dict with simulation options.\n\n"
          "Returns:\n"
          "    dict with status, final_nodes, waveform, etc.");

    // Signal value enum
    py::enum_<SignalValue>(m, "SignalValue")
        .value("ZERO", SignalValue::ZERO)
        .value("ONE",  SignalValue::ONE)
        .value("X",    SignalValue::X)
        .value("Z",    SignalValue::Z)
        .export_values();

    m.def("signal_from_char", &signal_from_char);
    m.def("signal_to_char", &signal_to_char);
    m.def("signal_from_string", &signal_from_string);
    m.def("signal_to_string", &signal_to_string);
}
