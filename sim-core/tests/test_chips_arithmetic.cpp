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

static void set_4bit(Circuit& c, Device* d, const std::string& prefix, uint32_t val) {
    for (int i = 0; i < 4; ++i)
        S(c, d, prefix + std::to_string(i), (val & (1u << i)) ? SignalValue::ONE : SignalValue::ZERO);
}

static uint32_t read_4bit(const std::map<std::string, SignalValue>& out, const std::string& prefix) {
    uint32_t v = 0;
    for (int i = 0; i < 4; ++i) {
        auto it = out.find(prefix + std::to_string(i));
        if (it != out.end() && it->second == SignalValue::ONE) v |= (1u << i);
    }
    return v;
}

// ============================================================
// 7485: 4-bit magnitude comparator tests
// ============================================================

TEST(Chip7485, AGreaterThanB) {
    auto [c, d] = setup(std::make_unique<Chip7485>("u1"));
    set_4bit(*c, d, "A", 5);
    set_4bit(*c, d, "B", 3);
    S(*c, d, "AGTB", SignalValue::ZERO);
    S(*c, d, "AEQB", SignalValue::ONE);
    S(*c, d, "ALTB", SignalValue::ZERO);
    auto out = d->eval(c->nodes(), "");
    EXPECT_EQ(out["AGTBO"], SignalValue::ONE);
    EXPECT_EQ(out["AEQBO"], SignalValue::ZERO);
    EXPECT_EQ(out["ALTBO"], SignalValue::ZERO);
}

TEST(Chip7485, ALessThanB) {
    auto [c, d] = setup(std::make_unique<Chip7485>("u1"));
    set_4bit(*c, d, "A", 3);
    set_4bit(*c, d, "B", 7);
    auto out = d->eval(c->nodes(), "");
    EXPECT_EQ(out["AGTBO"], SignalValue::ZERO);
    EXPECT_EQ(out["AEQBO"], SignalValue::ZERO);
    EXPECT_EQ(out["ALTBO"], SignalValue::ONE);
}

TEST(Chip7485, AEqualB) {
    auto [c, d] = setup(std::make_unique<Chip7485>("u1"));
    set_4bit(*c, d, "A", 10);
    set_4bit(*c, d, "B", 10);
    S(*c, d, "AGTB", SignalValue::ZERO);
    S(*c, d, "AEQB", SignalValue::ONE);   // cascade equal
    S(*c, d, "ALTB", SignalValue::ZERO);
    auto out = d->eval(c->nodes(), "");
    EXPECT_EQ(out["AGTBO"], SignalValue::ZERO);
    EXPECT_EQ(out["AEQBO"], SignalValue::ONE);
    EXPECT_EQ(out["ALTBO"], SignalValue::ZERO);
}

TEST(Chip7485, CascadeInputs) {
    auto [c, d] = setup(std::make_unique<Chip7485>("u1"));
    set_4bit(*c, d, "A", 5);
    set_4bit(*c, d, "B", 5);
    S(*c, d, "AGTB", SignalValue::ONE);   // cascade says A>B
    S(*c, d, "AEQB", SignalValue::ZERO);
    S(*c, d, "ALTB", SignalValue::ZERO);
    auto out = d->eval(c->nodes(), "");
    EXPECT_EQ(out["AGTBO"], SignalValue::ONE);  // cascaded greater-than passes through
}

TEST(Chip7485, BoundaryValues) {
    // A=0000, B=0000, cascade eq → EQ
    {
        auto [c, d] = setup(std::make_unique<Chip7485>("u1"));
        set_4bit(*c, d, "A", 0);
        set_4bit(*c, d, "B", 0);
        S(*c, d, "AEQB", SignalValue::ONE);
        auto out = d->eval(c->nodes(), "");
        EXPECT_EQ(out["AEQBO"], SignalValue::ONE);
    }
    // A=1111, B=1111
    {
        auto [c, d] = setup(std::make_unique<Chip7485>("u1"));
        set_4bit(*c, d, "A", 15);
        set_4bit(*c, d, "B", 15);
        S(*c, d, "AEQB", SignalValue::ONE);
        auto out = d->eval(c->nodes(), "");
        EXPECT_EQ(out["AEQBO"], SignalValue::ONE);
    }
}

// ============================================================
// 74280: 9-bit parity generator/checker tests
// ============================================================

TEST(Chip74280, EvenParity) {
    auto [c, d] = setup(std::make_unique<Chip74280>("u1"));
    // Set B=1, all others 0 → 1 one → odd
    for (char ch = 'A'; ch <= 'I'; ++ch)
        S(*c, d, std::string(1, ch), SignalValue::ZERO);
    S(*c, d, "B", SignalValue::ONE);
    auto out = d->eval(c->nodes(), "");
    EXPECT_EQ(out["EVEN"], SignalValue::ZERO);
    EXPECT_EQ(out["ODD"], SignalValue::ONE);
}

TEST(Chip74280, OddParity) {
    auto [c, d] = setup(std::make_unique<Chip74280>("u1"));
    for (char ch = 'A'; ch <= 'I'; ++ch)
        S(*c, d, std::string(1, ch), SignalValue::ZERO);
    // Even number: 0 ones → even
    auto out = d->eval(c->nodes(), "");
    EXPECT_EQ(out["EVEN"], SignalValue::ONE);
    EXPECT_EQ(out["ODD"], SignalValue::ZERO);
}

TEST(Chip74280, MultipleOnes) {
    auto [c, d] = setup(std::make_unique<Chip74280>("u1"));
    for (char ch = 'A'; ch <= 'I'; ++ch)
        S(*c, d, std::string(1, ch), SignalValue::ZERO);
    S(*c, d, "A", SignalValue::ONE);
    S(*c, d, "B", SignalValue::ONE);
    S(*c, d, "C", SignalValue::ONE);  // 3 ones → odd
    auto out = d->eval(c->nodes(), "");
    EXPECT_EQ(out["EVEN"], SignalValue::ZERO);
    EXPECT_EQ(out["ODD"], SignalValue::ONE);
}

TEST(Chip74280, AllOnes) {
    auto [c, d] = setup(std::make_unique<Chip74280>("u1"));
    for (char ch = 'A'; ch <= 'I'; ++ch)
        S(*c, d, std::string(1, ch), SignalValue::ONE);  // 9 ones → odd
    auto out = d->eval(c->nodes(), "");
    EXPECT_EQ(out["EVEN"], SignalValue::ZERO);
    EXPECT_EQ(out["ODD"], SignalValue::ONE);
}

// ============================================================
// 74283: 4-bit binary adder tests
// ============================================================

TEST(Chip74283, BasicAddition) {
    auto [c, d] = setup(std::make_unique<Chip74283>("u1"));
    set_4bit(*c, d, "A", 3);     // 0011
    set_4bit(*c, d, "B", 5);     // 0101
    S(*c, d, "C0", SignalValue::ZERO);
    auto out = d->eval(c->nodes(), "");
    EXPECT_EQ(read_4bit(out, "S"), 8u);   // 3+5=8
    EXPECT_EQ(out["C4"], SignalValue::ZERO);
}

TEST(Chip74283, WithCarryIn) {
    auto [c, d] = setup(std::make_unique<Chip74283>("u1"));
    set_4bit(*c, d, "A", 3);
    set_4bit(*c, d, "B", 5);
    S(*c, d, "C0", SignalValue::ONE);  // Cin=1
    auto out = d->eval(c->nodes(), "");
    EXPECT_EQ(read_4bit(out, "S"), 9u);   // 3+5+1=9
    EXPECT_EQ(out["C4"], SignalValue::ZERO);
}

TEST(Chip74283, OverflowCarry) {
    auto [c, d] = setup(std::make_unique<Chip74283>("u1"));
    set_4bit(*c, d, "A", 15);   // 1111
    set_4bit(*c, d, "B", 1);    // 0001
    S(*c, d, "C0", SignalValue::ZERO);
    auto out = d->eval(c->nodes(), "");
    EXPECT_EQ(read_4bit(out, "S"), 0u);    // 15+1=0 mod 16
    EXPECT_EQ(out["C4"], SignalValue::ONE); // Cout=1
}

TEST(Chip74283, MaxValues) {
    auto [c, d] = setup(std::make_unique<Chip74283>("u1"));
    set_4bit(*c, d, "A", 15);
    set_4bit(*c, d, "B", 15);
    S(*c, d, "C0", SignalValue::ONE);
    auto out = d->eval(c->nodes(), "");
    EXPECT_EQ(read_4bit(out, "S"), 15u);    // 15+15+1=31 → 15 mod 16
    EXPECT_EQ(out["C4"], SignalValue::ONE); // Cout=1
}

TEST(Chip74283, ZeroPlusZero) {
    auto [c, d] = setup(std::make_unique<Chip74283>("u1"));
    set_4bit(*c, d, "A", 0);
    set_4bit(*c, d, "B", 0);
    S(*c, d, "C0", SignalValue::ZERO);
    auto out = d->eval(c->nodes(), "");
    EXPECT_EQ(read_4bit(out, "S"), 0u);
    EXPECT_EQ(out["C4"], SignalValue::ZERO);
}

TEST(Chip74283, FullCoverage) {
    // Test all 16 x 16 x 2 = 512 combinations.
    for (uint32_t a = 0; a < 16; ++a) {
        for (uint32_t b = 0; b < 16; ++b) {
            for (uint32_t cin = 0; cin <= 1; ++cin) {
                auto [c, d] = setup(std::make_unique<Chip74283>("u1"));
                set_4bit(*c, d, "A", a);
                set_4bit(*c, d, "B", b);
                S(*c, d, "C0", cin ? SignalValue::ONE : SignalValue::ZERO);
                auto out = d->eval(c->nodes(), "");
                uint32_t expected = a + b + cin;
                uint32_t expected_sum = expected & 0xF;
                uint32_t expected_cout = expected >> 4;
                EXPECT_EQ(read_4bit(out, "S"), expected_sum)
                    << a << " + " << b << " + " << cin;
                EXPECT_EQ(out["C4"], expected_cout ? SignalValue::ONE : SignalValue::ZERO)
                    << a << " + " << b << " + " << cin << " Cout";
            }
        }
    }
}
