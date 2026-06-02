#include <gtest/gtest.h>
#include "logic_sim/devices/chips.hpp"
#include "logic_sim/circuit.hpp"

using namespace logic_sim;

static std::pair<std::unique_ptr<Circuit>, Device*>
setup(std::unique_ptr<Device> dev) {
    auto c = std::make_unique<Circuit>();
    Device* d = dev.get();
    int di = c->add_device(std::move(dev));
    for (auto& p : d->pins()) {
        int ni = c->add_node(p.name);
        d->connect_pin(p.id, ni);
        auto& node = c->nodes()[ni];
        Node::PinRef pr;
        pr.device_index = di;
        pr.pin_index = static_cast<int>(&p - &d->pins()[0]);
        node.pin_refs.push_back(pr);
        if (p.is_output()) node.driver_indices.push_back(node.pin_refs.size() - 1);
    }
    c->rebuild();
    return {std::move(c), d};
}

static void S(Circuit& c, Device* d, const std::string& pid, SignalValue v) {
    int ni = d->pin_node(pid);
    if (ni >= 0) { c.nodes()[ni].prev_value = c.nodes()[ni].value; c.nodes()[ni].value = v; }
}

// Returns the output on the rising edge.
// After eval, syncs CP prev_value to avoid re-detecting the same edge.
static std::map<std::string, SignalValue> pulse_cp(Circuit& c, Device* d) {
    S(c, d, "CP", SignalValue::ZERO);
    d->eval(c.nodes(), "");
    S(c, d, "CP", SignalValue::ONE);
    auto out = d->eval(c.nodes(), "CP");
    int ni = d->pin_node("CP");
    if (ni >= 0) c.nodes()[ni].prev_value = c.nodes()[ni].value;
    return out;
}

static uint32_t read_q(const std::map<std::string, SignalValue>& out) {
    uint32_t v = 0;
    for (int i = 0; i < 4; ++i) {
        auto it = out.find("Q" + std::to_string(i));
        if (it != out.end() && it->second == SignalValue::ONE) v |= (1u << i);
    }
    return v;
}

static void set_d(Circuit& c, Device* d, uint32_t val) {
    for (int i = 0; i < 4; ++i)
        S(c, d, "D" + std::to_string(i), (val & (1u << i)) ? SignalValue::ONE : SignalValue::ZERO);
}

// ============================================================
// 74175: 4-bit register tests
// ============================================================

TEST(Chip74175, AsyncClear) {
    auto [c, d] = setup(std::make_unique<Chip74175>("u1"));
    d->reset();
    S(*c, d, "CLR", SignalValue::ONE);
    set_d(*c, d, 5);
    pulse_cp(*c, d);

    // Now clear — async, takes effect immediately
    S(*c, d, "CLR", SignalValue::ZERO);
    auto out = d->eval(c->nodes(), "CLR");
    EXPECT_EQ(read_q(out), 0u);
    EXPECT_EQ(out["Q0N"], SignalValue::ONE);
}

TEST(Chip74175, LoadOnRisingEdge) {
    auto [c, d] = setup(std::make_unique<Chip74175>("u1"));
    d->reset();
    S(*c, d, "CLR", SignalValue::ONE);
    set_d(*c, d, 10);  // 1010
    auto out = pulse_cp(*c, d);
    EXPECT_EQ(read_q(out), 10u);
}

TEST(Chip74175, HoldWithoutEdge) {
    auto [c, d] = setup(std::make_unique<Chip74175>("u1"));
    d->reset();
    S(*c, d, "CLR", SignalValue::ONE);
    set_d(*c, d, 3);
    pulse_cp(*c, d);

    // Change D without clock edge — should hold
    set_d(*c, d, 12);
    auto out = d->eval(c->nodes(), "");  // CP unchanged, no edge
    EXPECT_EQ(read_q(out), 3u);  // should still be 3
}

// ============================================================
// 74LS195: 4-bit right-shift register tests
// ============================================================

TEST(Chip74LS195, AsyncClear) {
    auto [c, d] = setup(std::make_unique<Chip74LS195>("u1"));
    d->reset();
    S(*c, d, "CLR", SignalValue::ZERO);
    auto out = d->eval(c->nodes(), "CLR");
    EXPECT_EQ(read_q(out), 0u);
}

TEST(Chip74LS195, ParallelLoad) {
    auto [c, d] = setup(std::make_unique<Chip74LS195>("u1"));
    d->reset();
    S(*c, d, "CLR", SignalValue::ONE);
    S(*c, d, "SHLD", SignalValue::ZERO);  // load mode
    set_d(*c, d, 5);  // 0101
    auto out = pulse_cp(*c, d);
    EXPECT_EQ(read_q(out), 5u);
}

TEST(Chip74LS195, ShiftRight) {
    auto [c, d] = setup(std::make_unique<Chip74LS195>("u1"));
    d->reset();
    S(*c, d, "CLR", SignalValue::ONE);

    // First: load 0001
    S(*c, d, "SHLD", SignalValue::ZERO);
    set_d(*c, d, 1);
    pulse_cp(*c, d);

    // Shift right with J=1,K=1 (sets Q0=1)
    S(*c, d, "SHLD", SignalValue::ONE);
    S(*c, d, "J", SignalValue::ONE);
    S(*c, d, "K", SignalValue::ONE);
    auto out = pulse_cp(*c, d);
    // Q3Q2Q1Q0 was 0001 → shift right: Q0←JK_result=1, Q1←Q0(1), Q2←Q1(0), Q3←Q2(0) = 0011
    EXPECT_EQ(read_q(out), 3u);
}

// ============================================================
// 74LS194: 4-bit bidirectional shift register tests
// ============================================================

TEST(Chip74LS194, AsyncClear) {
    auto [c, d] = setup(std::make_unique<Chip74LS194>("u1"));
    d->reset();
    S(*c, d, "CR", SignalValue::ZERO);
    auto out = d->eval(c->nodes(), "CR");
    EXPECT_EQ(read_q(out), 0u);
}

TEST(Chip74LS194, Hold) {
    auto [c, d] = setup(std::make_unique<Chip74LS194>("u1"));
    d->reset();
    S(*c, d, "CR", SignalValue::ONE);
    S(*c, d, "S0", SignalValue::ZERO);
    S(*c, d, "S1", SignalValue::ZERO);  // hold mode
    auto out = pulse_cp(*c, d);
    EXPECT_EQ(read_q(out), 0u);  // initial value held
}

TEST(Chip74LS194, ShiftRight) {
    auto [c, d] = setup(std::make_unique<Chip74LS194>("u1"));
    d->reset();
    S(*c, d, "CR", SignalValue::ONE);

    // Load 0001
    S(*c, d, "S0", SignalValue::ONE);
    S(*c, d, "S1", SignalValue::ONE);  // parallel load
    set_d(*c, d, 1);
    pulse_cp(*c, d);

    // Shift right with DSR=1
    S(*c, d, "S1", SignalValue::ZERO);
    S(*c, d, "S0", SignalValue::ONE);   // S1S0=01: shift right
    S(*c, d, "DSR", SignalValue::ONE);  // serial right input = 1
    auto out = pulse_cp(*c, d);
    // Q3Q2Q1Q0=0001 → shift right: Q3←DSR=1, Q2←Q3=0, Q1←Q2=0, Q0←Q1=0 → 1000
    EXPECT_EQ(read_q(out), 8u);
}

TEST(Chip74LS194, ShiftLeft) {
    auto [c, d] = setup(std::make_unique<Chip74LS194>("u1"));
    d->reset();
    S(*c, d, "CR", SignalValue::ONE);

    // Load 1000
    S(*c, d, "S0", SignalValue::ONE);
    S(*c, d, "S1", SignalValue::ONE);
    set_d(*c, d, 8);  // 1000
    pulse_cp(*c, d);

    // Shift left with DSL=1
    S(*c, d, "S1", SignalValue::ONE);
    S(*c, d, "S0", SignalValue::ZERO);   // S1S0=10: shift left
    S(*c, d, "DSL", SignalValue::ONE);   // serial left input = 1
    auto out = pulse_cp(*c, d);
    // Q3Q2Q1Q0=1000 → shift left: Q0←DSL=1, Q1←Q0=0, Q2←Q1=0, Q3←Q2=0 → 0001
    EXPECT_EQ(read_q(out), 1u);
}

TEST(Chip74LS194, ParallelLoad) {
    auto [c, d] = setup(std::make_unique<Chip74LS194>("u1"));
    d->reset();
    S(*c, d, "CR", SignalValue::ONE);
    S(*c, d, "S0", SignalValue::ONE);
    S(*c, d, "S1", SignalValue::ONE);  // parallel load
    set_d(*c, d, 13);  // 1101
    auto out = pulse_cp(*c, d);
    EXPECT_EQ(read_q(out), 13u);
}
