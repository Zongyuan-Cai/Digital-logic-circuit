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

// ============================================================
// 74LS148: 8-to-3 priority encoder
// I7 has highest priority. Outputs A2,A1,A0 are active-low.
// I0 active → code 0 → A2A1A0 = 111 (all high, since inverted)
// I7 active → code 7 → A2A1A0 = 000 (all low, since inverted)
// ============================================================

TEST(Chip74LS148, Disabled) {
    auto [c, d] = setup(std::make_unique<Chip74LS148>("u1"));
    S(*c, d, "EI", SignalValue::ONE);
    for (int i = 0; i < 8; ++i) S(*c, d, "I" + std::to_string(i), SignalValue::ONE);
    auto out = d->eval(c->nodes(), "");
    EXPECT_EQ(out["GS"], SignalValue::ONE);
    EXPECT_EQ(out["EO"], SignalValue::ONE);
    EXPECT_EQ(out["A2"], SignalValue::ONE);
    EXPECT_EQ(out["A1"], SignalValue::ONE);
    EXPECT_EQ(out["A0"], SignalValue::ONE);
}

TEST(Chip74LS148, NoInputActive) {
    auto [c, d] = setup(std::make_unique<Chip74LS148>("u1"));
    S(*c, d, "EI", SignalValue::ZERO);
    for (int i = 0; i < 8; ++i) S(*c, d, "I" + std::to_string(i), SignalValue::ONE);
    auto out = d->eval(c->nodes(), "");
    EXPECT_EQ(out["GS"], SignalValue::ONE);
    EXPECT_EQ(out["EO"], SignalValue::ZERO);
}

TEST(Chip74LS148, I0Active) {
    auto [c, d] = setup(std::make_unique<Chip74LS148>("u1"));
    S(*c, d, "EI", SignalValue::ZERO);
    for (int i = 0; i < 8; ++i) S(*c, d, "I" + std::to_string(i), SignalValue::ONE);
    S(*c, d, "I0", SignalValue::ZERO);
    auto out = d->eval(c->nodes(), "");
    EXPECT_EQ(out["GS"], SignalValue::ZERO);
    EXPECT_EQ(out["EO"], SignalValue::ONE);
    // I0 active → code=0 → A2A1A0=111 (all inactive high)
    EXPECT_EQ(out["A2"], SignalValue::ONE);
    EXPECT_EQ(out["A1"], SignalValue::ONE);
    EXPECT_EQ(out["A0"], SignalValue::ONE);
}

TEST(Chip74LS148, I7Active) {
    auto [c, d] = setup(std::make_unique<Chip74LS148>("u1"));
    S(*c, d, "EI", SignalValue::ZERO);
    for (int i = 0; i < 8; ++i) S(*c, d, "I" + std::to_string(i), SignalValue::ONE);
    S(*c, d, "I7", SignalValue::ZERO);
    auto out = d->eval(c->nodes(), "");
    EXPECT_EQ(out["GS"], SignalValue::ZERO);
    // I7 active → code=7 → A2A1A0=000 (all active low)
    EXPECT_EQ(out["A2"], SignalValue::ZERO);
    EXPECT_EQ(out["A1"], SignalValue::ZERO);
    EXPECT_EQ(out["A0"], SignalValue::ZERO);
}

TEST(Chip74LS148, I5OverI3) {
    auto [c, d] = setup(std::make_unique<Chip74LS148>("u1"));
    S(*c, d, "EI", SignalValue::ZERO);
    for (int i = 0; i < 8; ++i) S(*c, d, "I" + std::to_string(i), SignalValue::ONE);
    S(*c, d, "I3", SignalValue::ZERO);
    S(*c, d, "I5", SignalValue::ZERO);  // I5 has higher priority
    auto out = d->eval(c->nodes(), "");
    EXPECT_EQ(out["GS"], SignalValue::ZERO);
    // I5 active → code=5=101 → A2 active(ZERO), A1 inactive(ONE), A0 active(ZERO)
    EXPECT_EQ(out["A2"], SignalValue::ZERO);
    EXPECT_EQ(out["A1"], SignalValue::ONE);
    EXPECT_EQ(out["A0"], SignalValue::ZERO);
}

TEST(Chip74LS148, I4Active) {
    auto [c, d] = setup(std::make_unique<Chip74LS148>("u1"));
    S(*c, d, "EI", SignalValue::ZERO);
    for (int i = 0; i < 8; ++i) S(*c, d, "I" + std::to_string(i), SignalValue::ONE);
    S(*c, d, "I4", SignalValue::ZERO);
    auto out = d->eval(c->nodes(), "");
    EXPECT_EQ(out["GS"], SignalValue::ZERO);
    // I4 → code=4=100 → A2 active(ZERO), A1 inactive(ONE), A0 inactive(ONE)
    EXPECT_EQ(out["A2"], SignalValue::ZERO);
    EXPECT_EQ(out["A1"], SignalValue::ONE);
    EXPECT_EQ(out["A0"], SignalValue::ONE);
}

TEST(Chip74LS148, UnknownInput) {
    auto [c, d] = setup(std::make_unique<Chip74LS148>("u1"));
    S(*c, d, "EI", SignalValue::ZERO);
    for (int i = 0; i < 8; ++i) S(*c, d, "I" + std::to_string(i), SignalValue::ONE);
    S(*c, d, "I7", SignalValue::X);
    auto out = d->eval(c->nodes(), "I7");
    EXPECT_EQ(out["GS"], SignalValue::X);
}

TEST(Chip74LS148, HigherPriorityUnknownMasksLowerActive) {
    auto [c, d] = setup(std::make_unique<Chip74LS148>("u1"));
    S(*c, d, "EI", SignalValue::ZERO);
    for (int i = 0; i < 8; ++i) S(*c, d, "I" + std::to_string(i), SignalValue::ONE);
    S(*c, d, "I7", SignalValue::X);
    S(*c, d, "I3", SignalValue::ZERO);
    auto out = d->eval(c->nodes(), "");
    EXPECT_EQ(out["A2"], SignalValue::X);
    EXPECT_EQ(out["A1"], SignalValue::X);
    EXPECT_EQ(out["A0"], SignalValue::X);
    EXPECT_EQ(out["GS"], SignalValue::X);
    EXPECT_EQ(out["EO"], SignalValue::X);
}

// ============================================================
// 74LS147: BCD priority encoder
// Inputs I1-I9 active low, highest priority.
// Outputs D,C,B,A active low.
// No input → BCD 0 → inverted output 1111
// I9 active → BCD 9=1001 → inverted=0110
// ============================================================

TEST(Chip74LS147, NoInput) {
    auto [c, d] = setup(std::make_unique<Chip74LS147>("u1"));
    for (int i = 1; i <= 9; ++i)
        S(*c, d, "I" + std::to_string(i), SignalValue::ONE);
    auto out = d->eval(c->nodes(), "");
    EXPECT_EQ(out["D"], SignalValue::ONE);
    EXPECT_EQ(out["C"], SignalValue::ONE);
    EXPECT_EQ(out["B"], SignalValue::ONE);
    EXPECT_EQ(out["A"], SignalValue::ONE);
}

TEST(Chip74LS147, I9Active) {
    auto [c, d] = setup(std::make_unique<Chip74LS147>("u1"));
    for (int i = 1; i <= 9; ++i) S(*c, d, "I" + std::to_string(i), SignalValue::ONE);
    S(*c, d, "I9", SignalValue::ZERO);
    auto out = d->eval(c->nodes(), "");
    // 9=1001 → inverted=0110
    EXPECT_EQ(out["D"], SignalValue::ZERO);
    EXPECT_EQ(out["C"], SignalValue::ONE);
    EXPECT_EQ(out["B"], SignalValue::ONE);
    EXPECT_EQ(out["A"], SignalValue::ZERO);
}

TEST(Chip74LS147, I5OverI1) {
    auto [c, d] = setup(std::make_unique<Chip74LS147>("u1"));
    for (int i = 1; i <= 9; ++i) S(*c, d, "I" + std::to_string(i), SignalValue::ONE);
    S(*c, d, "I1", SignalValue::ZERO);
    S(*c, d, "I5", SignalValue::ZERO);
    auto out = d->eval(c->nodes(), "");
    // 5=0101 → inverted=1010
    EXPECT_EQ(out["D"], SignalValue::ONE);
    EXPECT_EQ(out["C"], SignalValue::ZERO);
    EXPECT_EQ(out["B"], SignalValue::ONE);
    EXPECT_EQ(out["A"], SignalValue::ZERO);
}

TEST(Chip74LS147, I3Active) {
    auto [c, d] = setup(std::make_unique<Chip74LS147>("u1"));
    for (int i = 1; i <= 9; ++i) S(*c, d, "I" + std::to_string(i), SignalValue::ONE);
    S(*c, d, "I3", SignalValue::ZERO);
    auto out = d->eval(c->nodes(), "");
    // 3=0011 → inverted=1100
    EXPECT_EQ(out["D"], SignalValue::ONE);
    EXPECT_EQ(out["C"], SignalValue::ONE);
    EXPECT_EQ(out["B"], SignalValue::ZERO);
    EXPECT_EQ(out["A"], SignalValue::ZERO);
}

TEST(Chip74LS147, HigherPriorityUnknownMasksLowerActive) {
    auto [c, d] = setup(std::make_unique<Chip74LS147>("u1"));
    for (int i = 1; i <= 9; ++i) S(*c, d, "I" + std::to_string(i), SignalValue::ONE);
    S(*c, d, "I9", SignalValue::X);
    S(*c, d, "I3", SignalValue::ZERO);
    auto out = d->eval(c->nodes(), "");
    EXPECT_EQ(out["D"], SignalValue::X);
    EXPECT_EQ(out["C"], SignalValue::X);
    EXPECT_EQ(out["B"], SignalValue::X);
    EXPECT_EQ(out["A"], SignalValue::X);
}
