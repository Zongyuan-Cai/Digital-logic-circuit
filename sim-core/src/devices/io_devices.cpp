#include "logic_sim/devices/io_devices.hpp"
#include <algorithm>

namespace logic_sim {

// ---- VCC ----

VCC::VCC(const std::string& id) : Device(id, "VCC") {
    add_pin("OUT", "OUT", Pin::OUTPUT, Pin::ACTIVE_HIGH, "power");
}

std::map<std::string, SignalValue> VCC::eval(
    const std::vector<Node>&, const std::string&) {
    return {{"OUT", SignalValue::ONE}};
}

// ---- GND ----

GND::GND(const std::string& id) : Device(id, "GND") {
    add_pin("OUT", "OUT", Pin::OUTPUT, Pin::ACTIVE_HIGH, "ground");
}

std::map<std::string, SignalValue> GND::eval(
    const std::vector<Node>&, const std::string&) {
    return {{"OUT", SignalValue::ZERO}};
}

// ---- Switch ----

Switch::Switch(const std::string& id) : Device(id, "SWITCH") {
    add_pin("OUT", "OUT", Pin::OUTPUT, Pin::ACTIVE_HIGH, "output");
    set_param_int("value", 0);
}

std::map<std::string, SignalValue> Switch::eval(
    const std::vector<Node>&, const std::string&) {
    int val = get_param_int("value", 0);
    return {{"OUT", val ? SignalValue::ONE : SignalValue::ZERO}};
}

// ---- Clock ----

Clock::Clock(const std::string& id) : Device(id, "CLOCK") {
    add_pin("OUT", "OUT", Pin::OUTPUT, Pin::ACTIVE_HIGH, "clock_out");
    set_param_int("period", 2);
    // duty stored as int percentage (0-100), default 50
    set_param_int("duty_pct", 50);
    set_param_int("initial", 0);
    set_param_int("_tick", 0);
}

void Clock::reset() {
    int init = get_param_int("initial", 0);
    set_param_int("_tick", 0);
    set_param("_state", init ? SignalValue::ONE : SignalValue::ZERO);
}

std::map<std::string, SignalValue> Clock::eval(
    const std::vector<Node>&, const std::string& changed_pin) {
    // Only advance tick on real event-driven calls (self-trigger from
    // process_event), not during initialization's broadcast passes.
    if (!changed_pin.empty()) {
        int tick = get_param_int("_tick", 0);
        set_param_int("_tick", tick + 1);
    }

    int tick = get_param_int("_tick", 0);
    int period = get_param_int("period", 2);
    int duty_pct = get_param_int("duty_pct", 50);
    int init = get_param_int("initial", 0);
    if (period < 1) period = 2;
    if (duty_pct < 0) duty_pct = 0;
    if (duty_pct > 100) duty_pct = 100;

    int high_ticks = std::max(1, (period * duty_pct) / 100);
    int t_in_cycle = tick % period;
    SignalValue out = (t_in_cycle < high_ticks)
        ? SignalValue::ONE : SignalValue::ZERO;

    // If initial is 1, invert the output
    if (init) out = (out == SignalValue::ONE) ? SignalValue::ZERO : SignalValue::ONE;

    return {{"OUT", out}};
}

// ---- LED ----

LED::LED(const std::string& id) : Device(id, "LED") {
    add_pin("IN", "IN", Pin::INPUT, Pin::ACTIVE_HIGH, "input");
}

std::map<std::string, SignalValue> LED::eval(
    const std::vector<Node>& nodes, const std::string&) {
    // LED is output-only in simulation — no output pins, just reflects input
    return {};
}

// ---- Probe ----

Probe::Probe(const std::string& id) : Device(id, "PROBE") {
    add_pin("IN", "IN", Pin::INPUT, Pin::ACTIVE_HIGH, "probe");
}

std::map<std::string, SignalValue> Probe::eval(
    const std::vector<Node>& nodes, const std::string&) {
    return {};
}

// ---- SevenSegment ----

SevenSegment::SevenSegment(const std::string& id) : Device(id, "SEVEN_SEGMENT") {
    add_pin("A", "A", Pin::INPUT, Pin::ACTIVE_HIGH, "segment");
    add_pin("B", "B", Pin::INPUT, Pin::ACTIVE_HIGH, "segment");
    add_pin("C", "C", Pin::INPUT, Pin::ACTIVE_HIGH, "segment");
    add_pin("D", "D", Pin::INPUT, Pin::ACTIVE_HIGH, "segment");
    add_pin("E", "E", Pin::INPUT, Pin::ACTIVE_HIGH, "segment");
    add_pin("F", "F", Pin::INPUT, Pin::ACTIVE_HIGH, "segment");
    add_pin("G", "G", Pin::INPUT, Pin::ACTIVE_HIGH, "segment");
}

std::map<std::string, SignalValue> SevenSegment::eval(
    const std::vector<Node>& nodes, const std::string&) {
    // 7-segment is display-only — no output pins
    return {};
}

// ---- Factory ----

std::unique_ptr<Device> create_io_device(const std::string& type, const std::string& id) {
    if (type == "VCC")           return std::make_unique<VCC>(id);
    if (type == "GND")           return std::make_unique<GND>(id);
    if (type == "SWITCH")        return std::make_unique<Switch>(id);
    if (type == "CLOCK")         return std::make_unique<Clock>(id);
    if (type == "LED")           return std::make_unique<LED>(id);
    if (type == "PROBE")         return std::make_unique<Probe>(id);
    if (type == "SEVEN_SEGMENT") return std::make_unique<SevenSegment>(id);
    return nullptr;
}

}  // namespace logic_sim
