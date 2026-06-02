#include <gtest/gtest.h>
#include "logic_sim/devices/gates.hpp"
#include "logic_sim/circuit.hpp"

using namespace logic_sim;

// Helper: create a circuit with a gate device, wire all pins to nodes
static std::pair<std::unique_ptr<Circuit>, Device*>
setup_gate(std::unique_ptr<Device> dev) {
    auto circuit = std::make_unique<Circuit>();
    Device* d = dev.get();
    int di = circuit->add_device(std::move(dev));

    // Create a node for each pin
    for (auto& p : d->pins()) {
        int ni = circuit->add_node(p.name);
        d->connect_pin(p.id, ni);
        // Add pin_ref for each pin
        auto& node = circuit->nodes()[ni];
        Node::PinRef pr;
        pr.device_index = di;
        pr.pin_index = static_cast<int>(&p - &d->pins()[0]);
        node.pin_refs.push_back(pr);
        if (p.is_output()) {
            node.driver_indices.push_back(node.pin_refs.size() - 1);
        }
    }
    circuit->rebuild();

    return {std::move(circuit), d};
}

// Set a node's value (for driving inputs)
static void set_input(Circuit& c, Device* dev, const std::string& pin_id, SignalValue v) {
    int ni = dev->pin_node(pin_id);
    if (ni >= 0) {
        c.nodes()[ni].value = v;
        c.nodes()[ni].prev_value = v;
    }
}

// Get a node's value (to check outputs after eval)
static SignalValue get_output(Device* dev, const std::vector<Node>& nodes,
                               const std::string& pin_id) {
    int ni = dev->pin_node(pin_id);
    if (ni >= 0) return nodes[ni].value;
    return SignalValue::X;
}

// ============================================================
// AND gate tests
// ============================================================

TEST(AndGate, TruthTable2Input) {
    auto [circuit, dev] = setup_gate(std::make_unique<AndGate>("u1", 2));

    struct { SignalValue a; SignalValue b; SignalValue expected; } cases[] = {
        {SignalValue::ZERO, SignalValue::ZERO, SignalValue::ZERO},
        {SignalValue::ZERO, SignalValue::ONE,  SignalValue::ZERO},
        {SignalValue::ONE,  SignalValue::ZERO, SignalValue::ZERO},
        {SignalValue::ONE,  SignalValue::ONE,  SignalValue::ONE},
        {SignalValue::X,    SignalValue::ZERO, SignalValue::ZERO},
        {SignalValue::X,    SignalValue::ONE,  SignalValue::X},
        {SignalValue::Z,    SignalValue::ZERO, SignalValue::ZERO},
        {SignalValue::Z,    SignalValue::ONE,  SignalValue::X},
    };

    for (auto& tc : cases) {
        set_input(*circuit, dev, "I0", tc.a);
        set_input(*circuit, dev, "I1", tc.b);
        auto result = dev->eval(circuit->nodes(), "I0");
        EXPECT_EQ(result["Y"], tc.expected)
            << "AND(" << signal_to_char(tc.a) << "," << signal_to_char(tc.b) << ")";
    }
}

TEST(AndGate, MultiInput) {
    auto [circuit, dev] = setup_gate(std::make_unique<AndGate>("u1", 4));

    set_input(*circuit, dev, "I0", SignalValue::ONE);
    set_input(*circuit, dev, "I1", SignalValue::ONE);
    set_input(*circuit, dev, "I2", SignalValue::ONE);
    set_input(*circuit, dev, "I3", SignalValue::ONE);
    auto result = dev->eval(circuit->nodes(), "");
    EXPECT_EQ(result["Y"], SignalValue::ONE);

    set_input(*circuit, dev, "I2", SignalValue::ZERO);
    result = dev->eval(circuit->nodes(), "");
    EXPECT_EQ(result["Y"], SignalValue::ZERO);
}

// ============================================================
// OR gate tests
// ============================================================

TEST(OrGate, TruthTable2Input) {
    auto [circuit, dev] = setup_gate(std::make_unique<OrGate>("u1", 2));

    struct { SignalValue a; SignalValue b; SignalValue expected; } cases[] = {
        {SignalValue::ZERO, SignalValue::ZERO, SignalValue::ZERO},
        {SignalValue::ZERO, SignalValue::ONE,  SignalValue::ONE},
        {SignalValue::ONE,  SignalValue::ZERO, SignalValue::ONE},
        {SignalValue::ONE,  SignalValue::ONE,  SignalValue::ONE},
        {SignalValue::X,    SignalValue::ONE,  SignalValue::ONE},
        {SignalValue::X,    SignalValue::ZERO, SignalValue::X},
        {SignalValue::Z,    SignalValue::ONE,  SignalValue::ONE},
        {SignalValue::Z,    SignalValue::ZERO, SignalValue::X},
    };

    for (auto& tc : cases) {
        set_input(*circuit, dev, "I0", tc.a);
        set_input(*circuit, dev, "I1", tc.b);
        auto result = dev->eval(circuit->nodes(), "I0");
        EXPECT_EQ(result["Y"], tc.expected)
            << "OR(" << signal_to_char(tc.a) << "," << signal_to_char(tc.b) << ")";
    }
}

// ============================================================
// NOT gate tests
// ============================================================

TEST(NotGate, TruthTable) {
    auto [circuit, dev] = setup_gate(std::make_unique<NotGate>("u1"));

    struct { SignalValue in; SignalValue expected; } cases[] = {
        {SignalValue::ZERO, SignalValue::ONE},
        {SignalValue::ONE,  SignalValue::ZERO},
        {SignalValue::X,    SignalValue::X},
        {SignalValue::Z,    SignalValue::X},
    };

    for (auto& tc : cases) {
        set_input(*circuit, dev, "I", tc.in);
        auto result = dev->eval(circuit->nodes(), "I");
        EXPECT_EQ(result["Y"], tc.expected)
            << "NOT(" << signal_to_char(tc.in) << ")";
    }
}

// ============================================================
// NAND gate tests
// ============================================================

TEST(NandGate, TruthTable2Input) {
    auto [circuit, dev] = setup_gate(std::make_unique<NandGate>("u1", 2));

    struct { SignalValue a; SignalValue b; SignalValue expected; } cases[] = {
        {SignalValue::ZERO, SignalValue::ZERO, SignalValue::ONE},
        {SignalValue::ZERO, SignalValue::ONE,  SignalValue::ONE},
        {SignalValue::ONE,  SignalValue::ZERO, SignalValue::ONE},
        {SignalValue::ONE,  SignalValue::ONE,  SignalValue::ZERO},
        {SignalValue::X,    SignalValue::ZERO, SignalValue::ONE},
        {SignalValue::X,    SignalValue::ONE,  SignalValue::X},
    };

    for (auto& tc : cases) {
        set_input(*circuit, dev, "I0", tc.a);
        set_input(*circuit, dev, "I1", tc.b);
        auto result = dev->eval(circuit->nodes(), "I0");
        EXPECT_EQ(result["Y"], tc.expected)
            << "NAND(" << signal_to_char(tc.a) << "," << signal_to_char(tc.b) << ")";
    }
}

// ============================================================
// NOR gate tests
// ============================================================

TEST(NorGate, TruthTable2Input) {
    auto [circuit, dev] = setup_gate(std::make_unique<NorGate>("u1", 2));

    struct { SignalValue a; SignalValue b; SignalValue expected; } cases[] = {
        {SignalValue::ZERO, SignalValue::ZERO, SignalValue::ONE},
        {SignalValue::ZERO, SignalValue::ONE,  SignalValue::ZERO},
        {SignalValue::ONE,  SignalValue::ZERO, SignalValue::ZERO},
        {SignalValue::ONE,  SignalValue::ONE,  SignalValue::ZERO},
        {SignalValue::X,    SignalValue::ZERO, SignalValue::X},
        {SignalValue::X,    SignalValue::ONE,  SignalValue::ZERO},
    };

    for (auto& tc : cases) {
        set_input(*circuit, dev, "I0", tc.a);
        set_input(*circuit, dev, "I1", tc.b);
        auto result = dev->eval(circuit->nodes(), "I0");
        EXPECT_EQ(result["Y"], tc.expected)
            << "NOR(" << signal_to_char(tc.a) << "," << signal_to_char(tc.b) << ")";
    }
}

// ============================================================
// XOR gate tests
// ============================================================

TEST(XorGate, TruthTable2Input) {
    auto [circuit, dev] = setup_gate(std::make_unique<XorGate>("u1", 2));

    struct { SignalValue a; SignalValue b; SignalValue expected; } cases[] = {
        {SignalValue::ZERO, SignalValue::ZERO, SignalValue::ZERO},
        {SignalValue::ZERO, SignalValue::ONE,  SignalValue::ONE},
        {SignalValue::ONE,  SignalValue::ZERO, SignalValue::ONE},
        {SignalValue::ONE,  SignalValue::ONE,  SignalValue::ZERO},
        {SignalValue::X,    SignalValue::ZERO, SignalValue::X},
        {SignalValue::X,    SignalValue::ONE,  SignalValue::X},
        {SignalValue::Z,    SignalValue::ONE,  SignalValue::X},
    };

    for (auto& tc : cases) {
        set_input(*circuit, dev, "I0", tc.a);
        set_input(*circuit, dev, "I1", tc.b);
        auto result = dev->eval(circuit->nodes(), "I0");
        EXPECT_EQ(result["Y"], tc.expected)
            << "XOR(" << signal_to_char(tc.a) << "," << signal_to_char(tc.b) << ")";
    }
}

// ============================================================
// XNOR gate tests
// ============================================================

TEST(XnorGate, TruthTable2Input) {
    auto [circuit, dev] = setup_gate(std::make_unique<XnorGate>("u1", 2));

    struct { SignalValue a; SignalValue b; SignalValue expected; } cases[] = {
        {SignalValue::ZERO, SignalValue::ZERO, SignalValue::ONE},
        {SignalValue::ZERO, SignalValue::ONE,  SignalValue::ZERO},
        {SignalValue::ONE,  SignalValue::ZERO, SignalValue::ZERO},
        {SignalValue::ONE,  SignalValue::ONE,  SignalValue::ONE},
        {SignalValue::X,    SignalValue::ZERO, SignalValue::X},
        {SignalValue::X,    SignalValue::ONE,  SignalValue::X},
    };

    for (auto& tc : cases) {
        set_input(*circuit, dev, "I0", tc.a);
        set_input(*circuit, dev, "I1", tc.b);
        auto result = dev->eval(circuit->nodes(), "I0");
        EXPECT_EQ(result["Y"], tc.expected)
            << "XNOR(" << signal_to_char(tc.a) << "," << signal_to_char(tc.b) << ")";
    }
}

// ============================================================
// Factory tests
// ============================================================

TEST(GateFactory, CreatesCorrectType) {
    auto g = create_gate("AND", "u1", 3);
    EXPECT_EQ(g->type(), "AND");
    EXPECT_EQ(g->id(), "u1");
    EXPECT_EQ(g->pins().size(), 4u);  // 3 inputs + 1 output

    auto g2 = create_gate("NOT", "u2");
    EXPECT_EQ(g2->type(), "NOT");
    EXPECT_EQ(g2->pins().size(), 2u);  // 1 input + 1 output

    auto g3 = create_gate("UNKNOWN", "u3");
    EXPECT_EQ(g3, nullptr);
}
