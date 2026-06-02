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
// 74139: Dual 2-to-4 decoder tests
// ============================================================

TEST(Chip74139, Decoder1Disabled) {
    auto [c, d] = setup(std::make_unique<Chip74139>("u1"));
    S(*c, d, "1E", SignalValue::ONE);  // disabled
    auto out = d->eval(c->nodes(), "");
    EXPECT_EQ(out["1Y0"], SignalValue::ONE);
    EXPECT_EQ(out["1Y1"], SignalValue::ONE);
    EXPECT_EQ(out["1Y2"], SignalValue::ONE);
    EXPECT_EQ(out["1Y3"], SignalValue::ONE);
}

TEST(Chip74139, Decoder1Addr00) {
    auto [c, d] = setup(std::make_unique<Chip74139>("u1"));
    S(*c, d, "1E", SignalValue::ZERO);
    S(*c, d, "1A0", SignalValue::ZERO);
    S(*c, d, "1A1", SignalValue::ZERO);
    auto out = d->eval(c->nodes(), "");
    EXPECT_EQ(out["1Y0"], SignalValue::ZERO);  // selected
    EXPECT_EQ(out["1Y1"], SignalValue::ONE);
    EXPECT_EQ(out["1Y2"], SignalValue::ONE);
    EXPECT_EQ(out["1Y3"], SignalValue::ONE);
}

TEST(Chip74139, Decoder1Addr11) {
    auto [c, d] = setup(std::make_unique<Chip74139>("u1"));
    S(*c, d, "1E", SignalValue::ZERO);
    S(*c, d, "1A0", SignalValue::ONE);
    S(*c, d, "1A1", SignalValue::ONE);
    auto out = d->eval(c->nodes(), "");
    EXPECT_EQ(out["1Y0"], SignalValue::ONE);
    EXPECT_EQ(out["1Y1"], SignalValue::ONE);
    EXPECT_EQ(out["1Y2"], SignalValue::ONE);
    EXPECT_EQ(out["1Y3"], SignalValue::ZERO);  // selected
}

TEST(Chip74139, IndependentDecoders) {
    auto [c, d] = setup(std::make_unique<Chip74139>("u1"));
    S(*c, d, "1E", SignalValue::ZERO);  // decoder 1 enabled
    S(*c, d, "2E", SignalValue::ONE);   // decoder 2 disabled
    S(*c, d, "1A0", SignalValue::ZERO);
    S(*c, d, "1A1", SignalValue::ZERO);
    auto out = d->eval(c->nodes(), "");
    EXPECT_EQ(out["1Y0"], SignalValue::ZERO);  // dec1: Y0 active
    EXPECT_EQ(out["2Y0"], SignalValue::ONE);   // dec2: all inactive
}

// ============================================================
// 74LS138: 3-to-8 decoder tests
// ============================================================

TEST(Chip74LS138, DisabledG1Low) {
    auto [c, d] = setup(std::make_unique<Chip74LS138>("u1"));
    S(*c, d, "G1", SignalValue::ZERO);   // G1 inactive
    S(*c, d, "G2A", SignalValue::ZERO);
    S(*c, d, "G2B", SignalValue::ZERO);
    auto out = d->eval(c->nodes(), "");
    for (int i = 0; i < 8; ++i)
        EXPECT_EQ(out["Y" + std::to_string(i)], SignalValue::ONE);
}

TEST(Chip74LS138, DisabledG2AHigh) {
    auto [c, d] = setup(std::make_unique<Chip74LS138>("u1"));
    S(*c, d, "G1", SignalValue::ONE);
    S(*c, d, "G2A", SignalValue::ONE);   // G2A inactive
    S(*c, d, "G2B", SignalValue::ZERO);
    auto out = d->eval(c->nodes(), "");
    for (int i = 0; i < 8; ++i)
        EXPECT_EQ(out["Y" + std::to_string(i)], SignalValue::ONE);
}

TEST(Chip74LS138, DisabledG2BHigh) {
    auto [c, d] = setup(std::make_unique<Chip74LS138>("u1"));
    S(*c, d, "G1", SignalValue::ONE);
    S(*c, d, "G2A", SignalValue::ZERO);
    S(*c, d, "G2B", SignalValue::ONE);   // G2B inactive
    auto out = d->eval(c->nodes(), "");
    for (int i = 0; i < 8; ++i)
        EXPECT_EQ(out["Y" + std::to_string(i)], SignalValue::ONE);
}

TEST(Chip74LS138, EnabledAddr000) {
    auto [c, d] = setup(std::make_unique<Chip74LS138>("u1"));
    S(*c, d, "G1", SignalValue::ONE);
    S(*c, d, "G2A", SignalValue::ZERO);
    S(*c, d, "G2B", SignalValue::ZERO);
    S(*c, d, "A", SignalValue::ZERO);
    S(*c, d, "B", SignalValue::ZERO);
    S(*c, d, "C", SignalValue::ZERO);
    auto out = d->eval(c->nodes(), "");
    EXPECT_EQ(out["Y0"], SignalValue::ZERO);
    for (int i = 1; i < 8; ++i)
        EXPECT_EQ(out["Y" + std::to_string(i)], SignalValue::ONE);
}

TEST(Chip74LS138, EnabledAddr111) {
    auto [c, d] = setup(std::make_unique<Chip74LS138>("u1"));
    S(*c, d, "G1", SignalValue::ONE);
    S(*c, d, "G2A", SignalValue::ZERO);
    S(*c, d, "G2B", SignalValue::ZERO);
    S(*c, d, "A", SignalValue::ONE);
    S(*c, d, "B", SignalValue::ONE);
    S(*c, d, "C", SignalValue::ONE);
    auto out = d->eval(c->nodes(), "");
    EXPECT_EQ(out["Y7"], SignalValue::ZERO);
    for (int i = 0; i < 7; ++i)
        EXPECT_EQ(out["Y" + std::to_string(i)], SignalValue::ONE);
}

TEST(Chip74LS138, AllAddressCombinations) {
    for (int addr = 0; addr < 8; ++addr) {
        auto [c, d] = setup(std::make_unique<Chip74LS138>("u1"));
        S(*c, d, "G1", SignalValue::ONE);
        S(*c, d, "G2A", SignalValue::ZERO);
        S(*c, d, "G2B", SignalValue::ZERO);
        S(*c, d, "A", (addr & 1) ? SignalValue::ONE : SignalValue::ZERO);
        S(*c, d, "B", (addr & 2) ? SignalValue::ONE : SignalValue::ZERO);
        S(*c, d, "C", (addr & 4) ? SignalValue::ONE : SignalValue::ZERO);
        auto out = d->eval(c->nodes(), "");
        for (int i = 0; i < 8; ++i) {
            if (i == addr)
                EXPECT_EQ(out["Y" + std::to_string(i)], SignalValue::ZERO)
                    << "Addr " << addr << ": Y" << i << " should be active low";
            else
                EXPECT_EQ(out["Y" + std::to_string(i)], SignalValue::ONE)
                    << "Addr " << addr << ": Y" << i << " should be inactive high";
        }
    }
}

// ============================================================
// 74LS42: BCD to decimal decoder tests
// ============================================================

TEST(Chip74LS42, BCD0to9) {
    for (int val = 0; val < 10; ++val) {
        auto [c, d] = setup(std::make_unique<Chip74LS42>("u1"));
        S(*c, d, "A", (val & 1) ? SignalValue::ONE : SignalValue::ZERO);
        S(*c, d, "B", (val & 2) ? SignalValue::ONE : SignalValue::ZERO);
        S(*c, d, "C", (val & 4) ? SignalValue::ONE : SignalValue::ZERO);
        S(*c, d, "D", (val & 8) ? SignalValue::ONE : SignalValue::ZERO);
        auto out = d->eval(c->nodes(), "");
        for (int i = 0; i < 10; ++i) {
            if (i == val)
                EXPECT_EQ(out["Y" + std::to_string(i)], SignalValue::ZERO)
                    << "BCD " << val << ": Y" << i;
            else
                EXPECT_EQ(out["Y" + std::to_string(i)], SignalValue::ONE)
                    << "BCD " << val << ": Y" << i;
        }
    }
}

TEST(Chip74LS42, InvalidBCD) {
    for (int val = 10; val < 16; ++val) {
        auto [c, d] = setup(std::make_unique<Chip74LS42>("u1"));
        S(*c, d, "A", (val & 1) ? SignalValue::ONE : SignalValue::ZERO);
        S(*c, d, "B", (val & 2) ? SignalValue::ONE : SignalValue::ZERO);
        S(*c, d, "C", (val & 4) ? SignalValue::ONE : SignalValue::ZERO);
        S(*c, d, "D", (val & 8) ? SignalValue::ONE : SignalValue::ZERO);
        auto out = d->eval(c->nodes(), "");
        // All outputs should be inactive (high) for invalid BCD
        for (int i = 0; i < 10; ++i)
            EXPECT_EQ(out["Y" + std::to_string(i)], SignalValue::ONE)
                << "Invalid BCD " << val << ": Y" << i;
    }
}

// ============================================================
// 7448: BCD to 7-segment decoder tests
// ============================================================

// Segment order: a(bit0), b(bit1), c(bit2), d(bit3), e(bit4), f(bit5), g(bit6)
static uint8_t segment_bits(const std::map<std::string, SignalValue>& out) {
    uint8_t pat = 0;
    const char* segs = "abcdefg";
    for (int i = 0; i < 7; ++i) {
        auto it = out.find(std::string(1, segs[i]));
        if (it != out.end() && it->second == SignalValue::ONE)
            pat |= (1u << i);
    }
    return pat;
}

TEST(Chip7448, LampTest) {
    auto [c, d] = setup(std::make_unique<Chip7448>("u1"));
    S(*c, d, "LT", SignalValue::ZERO);     // lamp test active
    S(*c, d, "RBI", SignalValue::ONE);
    S(*c, d, "BIRBO", SignalValue::ONE);   // not blanking
    auto out = d->eval(c->nodes(), "");
    // All segments should be ON
    EXPECT_EQ(segment_bits(out), 0x7Fu);  // all 7 bits
}

TEST(Chip7448, Digit0) {
    auto [c, d] = setup(std::make_unique<Chip7448>("u1"));
    S(*c, d, "LT", SignalValue::ONE);
    S(*c, d, "RBI", SignalValue::ONE);
    S(*c, d, "BIRBO", SignalValue::ONE);
    S(*c, d, "A", SignalValue::ZERO);
    S(*c, d, "B", SignalValue::ZERO);
    S(*c, d, "C", SignalValue::ZERO);
    S(*c, d, "D", SignalValue::ZERO);
    auto out = d->eval(c->nodes(), "");
    // 0: segments a,b,c,d,e,f (no g)
    EXPECT_EQ(segment_bits(out), 0b0111111u);
}

TEST(Chip7448, Digit8) {
    auto [c, d] = setup(std::make_unique<Chip7448>("u1"));
    S(*c, d, "LT", SignalValue::ONE);
    S(*c, d, "RBI", SignalValue::ONE);
    S(*c, d, "BIRBO", SignalValue::ONE);
    S(*c, d, "A", SignalValue::ZERO);  // 8 = 1000 → A=0
    S(*c, d, "B", SignalValue::ZERO);
    S(*c, d, "C", SignalValue::ZERO);
    S(*c, d, "D", SignalValue::ONE);
    auto out = d->eval(c->nodes(), "");
    // 8: all segments on
    EXPECT_EQ(segment_bits(out), 0x7Fu);
}

TEST(Chip7448, RippleBlankingInput) {
    auto [c, d] = setup(std::make_unique<Chip7448>("u1"));
    S(*c, d, "LT", SignalValue::ONE);
    S(*c, d, "RBI", SignalValue::ZERO);    // RBI active
    S(*c, d, "BIRBO", SignalValue::ONE);
    S(*c, d, "A", SignalValue::ZERO);  // digit 0
    S(*c, d, "B", SignalValue::ZERO);
    S(*c, d, "C", SignalValue::ZERO);
    S(*c, d, "D", SignalValue::ZERO);
    auto out = d->eval(c->nodes(), "");
    // Should blank digit 0 when RBI active
    EXPECT_EQ(segment_bits(out), 0u);
}

TEST(Chip7448, AllDigits) {
    // Expected patterns for digits 0-9
    const uint8_t expected[10] = {
        0b0111111, // 0
        0b0000110, // 1
        0b1011011, // 2
        0b1001111, // 3
        0b1100110, // 4
        0b1101101, // 5
        0b1111101, // 6
        0b0000111, // 7
        0b1111111, // 8
        0b1101111, // 9
    };

    for (int digit = 0; digit < 10; ++digit) {
        auto [c, d] = setup(std::make_unique<Chip7448>("u1"));
        S(*c, d, "LT", SignalValue::ONE);
        S(*c, d, "RBI", SignalValue::ONE);
        S(*c, d, "BIRBO", SignalValue::ONE);
        S(*c, d, "A", (digit & 1) ? SignalValue::ONE : SignalValue::ZERO);
        S(*c, d, "B", (digit & 2) ? SignalValue::ONE : SignalValue::ZERO);
        S(*c, d, "C", (digit & 4) ? SignalValue::ONE : SignalValue::ZERO);
        S(*c, d, "D", (digit & 8) ? SignalValue::ONE : SignalValue::ZERO);
        auto out = d->eval(c->nodes(), "");
        EXPECT_EQ(segment_bits(out), expected[digit])
            << "Digit " << digit;
    }
}
