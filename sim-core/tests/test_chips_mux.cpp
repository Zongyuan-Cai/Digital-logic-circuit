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
// 74153 / 74HC153: Dual 4-to-1 MUX tests
// ============================================================

TEST(Chip74153, Mux1Disabled) {
    auto [c, d] = setup(std::make_unique<Chip74153>("u1"));
    S(*c, d, "1G", SignalValue::ONE);  // disabled
    S(*c, d, "1C0", SignalValue::ONE);
    S(*c, d, "1C1", SignalValue::ZERO);
    auto out = d->eval(c->nodes(), "");
    EXPECT_EQ(out["1Y"], SignalValue::ZERO);  // disabled output is 0
}

TEST(Chip74153, Mux1Select00) {
    auto [c, d] = setup(std::make_unique<Chip74153>("u1"));
    S(*c, d, "1G", SignalValue::ZERO);
    S(*c, d, "A", SignalValue::ZERO);
    S(*c, d, "B", SignalValue::ZERO);
    S(*c, d, "1C0", SignalValue::ONE);
    S(*c, d, "1C1", SignalValue::ZERO);
    S(*c, d, "1C2", SignalValue::ZERO);
    S(*c, d, "1C3", SignalValue::ZERO);
    auto out = d->eval(c->nodes(), "");
    EXPECT_EQ(out["1Y"], SignalValue::ONE);  // 00 selects C0
}

TEST(Chip74153, Mux1Select11) {
    auto [c, d] = setup(std::make_unique<Chip74153>("u1"));
    S(*c, d, "1G", SignalValue::ZERO);
    S(*c, d, "A", SignalValue::ONE);
    S(*c, d, "B", SignalValue::ONE);
    S(*c, d, "1C0", SignalValue::ZERO);
    S(*c, d, "1C1", SignalValue::ZERO);
    S(*c, d, "1C2", SignalValue::ZERO);
    S(*c, d, "1C3", SignalValue::ONE);
    auto out = d->eval(c->nodes(), "");
    EXPECT_EQ(out["1Y"], SignalValue::ONE);  // 11 selects C3
}

TEST(Chip74153, AllSelectCombinations) {
    for (int sel = 0; sel < 4; ++sel) {
        auto [c, d] = setup(std::make_unique<Chip74153>("u1"));
        S(*c, d, "1G", SignalValue::ZERO);
        S(*c, d, "A", (sel & 1) ? SignalValue::ONE : SignalValue::ZERO);
        S(*c, d, "B", (sel & 2) ? SignalValue::ONE : SignalValue::ZERO);
        // Set only the selected input to 1
        S(*c, d, "1C0", (sel == 0) ? SignalValue::ONE : SignalValue::ZERO);
        S(*c, d, "1C1", (sel == 1) ? SignalValue::ONE : SignalValue::ZERO);
        S(*c, d, "1C2", (sel == 2) ? SignalValue::ONE : SignalValue::ZERO);
        S(*c, d, "1C3", (sel == 3) ? SignalValue::ONE : SignalValue::ZERO);
        auto out = d->eval(c->nodes(), "");
        EXPECT_EQ(out["1Y"], SignalValue::ONE) << "sel=" << sel;
    }
}

TEST(Chip74153, IndependentMuxes) {
    auto [c, d] = setup(std::make_unique<Chip74153>("u1"));
    S(*c, d, "1G", SignalValue::ZERO);
    S(*c, d, "2G", SignalValue::ZERO);
    S(*c, d, "A", SignalValue::ZERO);
    S(*c, d, "B", SignalValue::ZERO);
    S(*c, d, "1C0", SignalValue::ONE);
    S(*c, d, "2C0", SignalValue::ZERO);
    auto out = d->eval(c->nodes(), "");
    EXPECT_EQ(out["1Y"], SignalValue::ONE);
    EXPECT_EQ(out["2Y"], SignalValue::ZERO);
}

// ============================================================
// 74LS151 / 74151: 8-to-1 MUX tests
// ============================================================

TEST(Chip74LS151, Disabled) {
    auto [c, d] = setup(std::make_unique<Chip74LS151>("u1"));
    S(*c, d, "G", SignalValue::ONE);  // disabled/strobe inactive
    S(*c, d, "D0", SignalValue::ONE);
    auto out = d->eval(c->nodes(), "");
    EXPECT_EQ(out["Y"], SignalValue::ZERO);
    EXPECT_EQ(out["W"], SignalValue::ONE);  // W is complement: NOT 0 = 1
}

TEST(Chip74LS151, SelectD0) {
    auto [c, d] = setup(std::make_unique<Chip74LS151>("u1"));
    S(*c, d, "G", SignalValue::ZERO);
    S(*c, d, "A", SignalValue::ZERO);
    S(*c, d, "B", SignalValue::ZERO);
    S(*c, d, "C", SignalValue::ZERO);
    for (int i = 0; i < 8; ++i)
        S(*c, d, "D" + std::to_string(i), (i == 0) ? SignalValue::ONE : SignalValue::ZERO);
    auto out = d->eval(c->nodes(), "");
    EXPECT_EQ(out["Y"], SignalValue::ONE);
    EXPECT_EQ(out["W"], SignalValue::ZERO);
}

TEST(Chip74LS151, SelectD7) {
    auto [c, d] = setup(std::make_unique<Chip74LS151>("u1"));
    S(*c, d, "G", SignalValue::ZERO);
    S(*c, d, "A", SignalValue::ONE);
    S(*c, d, "B", SignalValue::ONE);
    S(*c, d, "C", SignalValue::ONE);
    for (int i = 0; i < 8; ++i)
        S(*c, d, "D" + std::to_string(i), (i == 7) ? SignalValue::ONE : SignalValue::ZERO);
    auto out = d->eval(c->nodes(), "");
    EXPECT_EQ(out["Y"], SignalValue::ONE);
    EXPECT_EQ(out["W"], SignalValue::ZERO);
}

TEST(Chip74LS151, All8SelectCombinations) {
    for (int sel = 0; sel < 8; ++sel) {
        auto [c, d] = setup(std::make_unique<Chip74LS151>("u1"));
        S(*c, d, "G", SignalValue::ZERO);
        S(*c, d, "A", (sel & 1) ? SignalValue::ONE : SignalValue::ZERO);
        S(*c, d, "B", (sel & 2) ? SignalValue::ONE : SignalValue::ZERO);
        S(*c, d, "C", (sel & 4) ? SignalValue::ONE : SignalValue::ZERO);
        for (int i = 0; i < 8; ++i)
            S(*c, d, "D" + std::to_string(i), (i == sel) ? SignalValue::ONE : SignalValue::ZERO);
        auto out = d->eval(c->nodes(), "");
        EXPECT_EQ(out["Y"], SignalValue::ONE) << "sel=" << sel;
        EXPECT_EQ(out["W"], SignalValue::ZERO) << "sel=" << sel;
    }
}

TEST(Chip74LS151, UnknownSelect) {
    auto [c, d] = setup(std::make_unique<Chip74LS151>("u1"));
    S(*c, d, "G", SignalValue::ZERO);
    S(*c, d, "A", SignalValue::X);  // unknown → output X
    auto out = d->eval(c->nodes(), "");
    EXPECT_EQ(out["Y"], SignalValue::X);
    EXPECT_EQ(out["W"], SignalValue::X);
}
