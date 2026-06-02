#pragma once

#include <memory>
#include "../device.hpp"

namespace logic_sim {

/// AND gate: output = AND of all inputs.
/// Output: 0 if any input 0; X if no 0 and any X/Z; 1 if all 1.
class AndGate : public Device {
public:
    explicit AndGate(const std::string& id, size_t num_inputs = 2);
    std::map<std::string, SignalValue> eval(
        const std::vector<Node>& nodes, const std::string& changed) override;
    void reset() override {}
};

/// OR gate: output = OR of all inputs.
class OrGate : public Device {
public:
    explicit OrGate(const std::string& id, size_t num_inputs = 2);
    std::map<std::string, SignalValue> eval(
        const std::vector<Node>& nodes, const std::string& changed) override;
    void reset() override {}
};

/// NOT gate (inverter): output = NOT(input).
class NotGate : public Device {
public:
    explicit NotGate(const std::string& id);
    std::map<std::string, SignalValue> eval(
        const std::vector<Node>& nodes, const std::string& changed) override;
    void reset() override {}
};

/// NAND gate: output = NOT(AND of all inputs).
class NandGate : public Device {
public:
    explicit NandGate(const std::string& id, size_t num_inputs = 2);
    std::map<std::string, SignalValue> eval(
        const std::vector<Node>& nodes, const std::string& changed) override;
    void reset() override {}
};

/// NOR gate: output = NOT(OR of all inputs).
class NorGate : public Device {
public:
    explicit NorGate(const std::string& id, size_t num_inputs = 2);
    std::map<std::string, SignalValue> eval(
        const std::vector<Node>& nodes, const std::string& changed) override;
    void reset() override {}
};

/// XOR gate: output = XOR of all inputs (parity).
class XorGate : public Device {
public:
    explicit XorGate(const std::string& id, size_t num_inputs = 2);
    std::map<std::string, SignalValue> eval(
        const std::vector<Node>& nodes, const std::string& changed) override;
    void reset() override {}
};

/// XNOR gate: output = NOT(XOR of all inputs).
class XnorGate : public Device {
public:
    explicit XnorGate(const std::string& id, size_t num_inputs = 2);
    std::map<std::string, SignalValue> eval(
        const std::vector<Node>& nodes, const std::string& changed) override;
    void reset() override {}
};

/// Factory: create a gate by type string.
std::unique_ptr<Device> create_gate(const std::string& type, const std::string& id,
                                     size_t num_inputs = 2);

}  // namespace logic_sim
