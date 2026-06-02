#include "logic_sim/devices/gates.hpp"
#include <sstream>

namespace logic_sim {

// ---- AND Gate ----

AndGate::AndGate(const std::string& id, size_t num_inputs)
    : Device(id, "AND")
{
    for (size_t i = 0; i < num_inputs; ++i) {
        add_pin("I" + std::to_string(i), "I" + std::to_string(i), Pin::INPUT);
    }
    add_pin("Y", "Y", Pin::OUTPUT);
}

std::map<std::string, SignalValue> AndGate::eval(
    const std::vector<Node>& nodes, const std::string&) {
    std::vector<SignalValue> inputs;
    for (auto& p : pins_) {
        if (p.is_input()) {
            inputs.push_back(input(p.id, nodes));
        }
    }
    SignalValue out = signal_and(inputs);
    return {{"Y", out}};
}

// ---- OR Gate ----

OrGate::OrGate(const std::string& id, size_t num_inputs)
    : Device(id, "OR")
{
    for (size_t i = 0; i < num_inputs; ++i) {
        add_pin("I" + std::to_string(i), "I" + std::to_string(i), Pin::INPUT);
    }
    add_pin("Y", "Y", Pin::OUTPUT);
}

std::map<std::string, SignalValue> OrGate::eval(
    const std::vector<Node>& nodes, const std::string&) {
    std::vector<SignalValue> inputs;
    for (auto& p : pins_) {
        if (p.is_input()) {
            inputs.push_back(input(p.id, nodes));
        }
    }
    return {{"Y", signal_or(inputs)}};
}

// ---- NOT Gate ----

NotGate::NotGate(const std::string& id)
    : Device(id, "NOT")
{
    add_pin("I", "I", Pin::INPUT);
    add_pin("Y", "Y", Pin::OUTPUT);
}

std::map<std::string, SignalValue> NotGate::eval(
    const std::vector<Node>& nodes, const std::string&) {
    return {{"Y", signal_not(input("I", nodes))}};
}

// ---- NAND Gate ----

NandGate::NandGate(const std::string& id, size_t num_inputs)
    : Device(id, "NAND")
{
    for (size_t i = 0; i < num_inputs; ++i) {
        add_pin("I" + std::to_string(i), "I" + std::to_string(i), Pin::INPUT);
    }
    add_pin("Y", "Y", Pin::OUTPUT);
}

std::map<std::string, SignalValue> NandGate::eval(
    const std::vector<Node>& nodes, const std::string&) {
    std::vector<SignalValue> inputs;
    for (auto& p : pins_) {
        if (p.is_input()) {
            inputs.push_back(input(p.id, nodes));
        }
    }
    return {{"Y", signal_nand(inputs)}};
}

// ---- NOR Gate ----

NorGate::NorGate(const std::string& id, size_t num_inputs)
    : Device(id, "NOR")
{
    for (size_t i = 0; i < num_inputs; ++i) {
        add_pin("I" + std::to_string(i), "I" + std::to_string(i), Pin::INPUT);
    }
    add_pin("Y", "Y", Pin::OUTPUT);
}

std::map<std::string, SignalValue> NorGate::eval(
    const std::vector<Node>& nodes, const std::string&) {
    std::vector<SignalValue> inputs;
    for (auto& p : pins_) {
        if (p.is_input()) {
            inputs.push_back(input(p.id, nodes));
        }
    }
    return {{"Y", signal_nor(inputs)}};
}

// ---- XOR Gate ----

XorGate::XorGate(const std::string& id, size_t num_inputs)
    : Device(id, "XOR")
{
    for (size_t i = 0; i < num_inputs; ++i) {
        add_pin("I" + std::to_string(i), "I" + std::to_string(i), Pin::INPUT);
    }
    add_pin("Y", "Y", Pin::OUTPUT);
}

std::map<std::string, SignalValue> XorGate::eval(
    const std::vector<Node>& nodes, const std::string&) {
    std::vector<SignalValue> inputs;
    for (auto& p : pins_) {
        if (p.is_input()) {
            inputs.push_back(input(p.id, nodes));
        }
    }
    return {{"Y", signal_xor(inputs)}};
}

// ---- XNOR Gate ----

XnorGate::XnorGate(const std::string& id, size_t num_inputs)
    : Device(id, "XNOR")
{
    for (size_t i = 0; i < num_inputs; ++i) {
        add_pin("I" + std::to_string(i), "I" + std::to_string(i), Pin::INPUT);
    }
    add_pin("Y", "Y", Pin::OUTPUT);
}

std::map<std::string, SignalValue> XnorGate::eval(
    const std::vector<Node>& nodes, const std::string&) {
    std::vector<SignalValue> inputs;
    for (auto& p : pins_) {
        if (p.is_input()) {
            inputs.push_back(input(p.id, nodes));
        }
    }
    return {{"Y", signal_xnor(inputs)}};
}

// ---- Factory ----

std::unique_ptr<Device> create_gate(const std::string& type, const std::string& id,
                                     size_t num_inputs) {
    if (type == "AND")   return std::make_unique<AndGate>(id, num_inputs);
    if (type == "OR")    return std::make_unique<OrGate>(id, num_inputs);
    if (type == "NOT")   return std::make_unique<NotGate>(id);
    if (type == "NAND")  return std::make_unique<NandGate>(id, num_inputs);
    if (type == "NOR")   return std::make_unique<NorGate>(id, num_inputs);
    if (type == "XOR")   return std::make_unique<XorGate>(id, num_inputs);
    if (type == "XNOR")  return std::make_unique<XnorGate>(id, num_inputs);
    return nullptr;
}

}  // namespace logic_sim
