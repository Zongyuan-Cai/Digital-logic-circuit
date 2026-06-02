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

// clock_cycle: brings CP from 0→1, evaluates, returns the output on the rising edge.
// After eval, syncs CP prev_value to avoid re-detecting the same edge.
static std::map<std::string, SignalValue> clock_cycle(Circuit& c, Device* d) {
    S(c, d, "CP", SignalValue::ZERO);
    d->eval(c.nodes(), "CP");
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
// 74161 / 74LS161: 4-bit binary counter, async clear
// ============================================================

TEST(Chip74161, AsyncClear) {
    auto [c, d] = setup(std::make_unique<Chip74161>("u1"));
    d->reset();
    S(*c, d, "CR", SignalValue::ONE);
    S(*c, d, "LD", SignalValue::ONE);
    S(*c, d, "EP", SignalValue::ONE);
    S(*c, d, "ET", SignalValue::ONE);
    // Count up a few
    clock_cycle(*c, d);
    clock_cycle(*c, d);
    auto out = clock_cycle(*c, d);
    EXPECT_GT(read_q(out), 0u) << "Should have counted up";

    // Async clear — immediately resets
    S(*c, d, "CR", SignalValue::ZERO);
    out = d->eval(c->nodes(), "CR");
    EXPECT_EQ(read_q(out), 0u) << "Async clear should reset to 0";
}

TEST(Chip74161, SyncLoad) {
    auto [c, d] = setup(std::make_unique<Chip74161>("u1"));
    d->reset();
    S(*c, d, "CR", SignalValue::ONE);
    set_d(*c, d, 7);
    S(*c, d, "LD", SignalValue::ZERO);  // load active
    S(*c, d, "EP", SignalValue::ONE);
    S(*c, d, "ET", SignalValue::ONE);
    auto out = clock_cycle(*c, d);
    EXPECT_EQ(read_q(out), 7u) << "Sync load should set Q to D";
}

TEST(Chip74161, Count0000to1111) {
    auto [c, d] = setup(std::make_unique<Chip74161>("u1"));
    d->reset();
    S(*c, d, "CR", SignalValue::ONE);
    S(*c, d, "LD", SignalValue::ONE);
    S(*c, d, "EP", SignalValue::ONE);
    S(*c, d, "ET", SignalValue::ONE);

    for (int expected = 1; expected <= 15; ++expected) {
        auto out = clock_cycle(*c, d);
        EXPECT_EQ(read_q(out), static_cast<uint32_t>(expected))
            << "After " << expected << " clocks";
    }

    // 16th clock → wraps to 0
    auto out = clock_cycle(*c, d);
    EXPECT_EQ(read_q(out), 0u) << "Should wrap to 0 after 16 clocks";
}

TEST(Chip74161, DisabledCount) {
    auto [c, d] = setup(std::make_unique<Chip74161>("u1"));
    d->reset();
    S(*c, d, "CR", SignalValue::ONE);
    S(*c, d, "LD", SignalValue::ONE);
    S(*c, d, "EP", SignalValue::ZERO);  // count disabled
    S(*c, d, "ET", SignalValue::ONE);

    auto out = clock_cycle(*c, d);
    EXPECT_EQ(read_q(out), 0u) << "Should hold when EP=0";
}

TEST(Chip74161, CarryOut) {
    auto [c, d] = setup(std::make_unique<Chip74161>("u1"));
    d->reset();
    S(*c, d, "CR", SignalValue::ONE);
    S(*c, d, "LD", SignalValue::ONE);
    S(*c, d, "EP", SignalValue::ONE);
    S(*c, d, "ET", SignalValue::ONE);

    // Count to 15
    for (int i = 0; i < 15; ++i)
        clock_cycle(*c, d);

    auto out = d->eval(c->nodes(), "");  // non-clock eval for checking outputs
    EXPECT_EQ(read_q(out), 15u);
    EXPECT_EQ(out["CO"], SignalValue::ONE) << "CO should be 1 at count 15";

    // Wrap to 0
    out = clock_cycle(*c, d);
    EXPECT_EQ(read_q(out), 0u);
    EXPECT_EQ(out["CO"], SignalValue::ZERO);
}

// ============================================================
// 74163: 4-bit synchronous binary counter, sync clear
// ============================================================

TEST(Chip74163, SyncClearOnClockEdge) {
    auto [c, d] = setup(std::make_unique<Chip74163>("u1"));
    d->reset();
    S(*c, d, "CR", SignalValue::ONE);
    S(*c, d, "LD", SignalValue::ONE);
    S(*c, d, "EP", SignalValue::ONE);
    S(*c, d, "ET", SignalValue::ONE);

    // Count up a few
    clock_cycle(*c, d);
    clock_cycle(*c, d);
    auto out = clock_cycle(*c, d);
    uint32_t val = read_q(out);
    EXPECT_GT(val, 0u) << "Should have counted up";

    // Assert CR low — should NOT clear immediately (sync)
    S(*c, d, "CR", SignalValue::ZERO);
    out = d->eval(c->nodes(), "CR");
    EXPECT_EQ(read_q(out), val) << "Sync clear should not take effect until clock edge";

    // Clock edge → now it clears
    out = clock_cycle(*c, d);
    EXPECT_EQ(read_q(out), 0u) << "Sync clear should work on clock edge";
}

TEST(Chip74163, Mod16Count) {
    auto [c, d] = setup(std::make_unique<Chip74163>("u1"));
    d->reset();
    S(*c, d, "CR", SignalValue::ONE);
    S(*c, d, "LD", SignalValue::ONE);
    S(*c, d, "EP", SignalValue::ONE);
    S(*c, d, "ET", SignalValue::ONE);

    for (int i = 1; i <= 16; ++i) {
        auto out = clock_cycle(*c, d);
        EXPECT_EQ(read_q(out), static_cast<uint32_t>(i % 16)) << "Tick " << i;
    }
}

TEST(Chip74163, CarryOut) {
    auto [c, d] = setup(std::make_unique<Chip74163>("u1"));
    d->reset();
    S(*c, d, "CR", SignalValue::ONE);
    S(*c, d, "LD", SignalValue::ONE);
    S(*c, d, "EP", SignalValue::ONE);
    S(*c, d, "ET", SignalValue::ONE);

    for (int i = 0; i < 15; ++i) clock_cycle(*c, d);
    auto out = d->eval(c->nodes(), "");
    EXPECT_EQ(out["CO"], SignalValue::ONE) << "CO=1 at count 15";
    EXPECT_EQ(read_q(out), 15u);
}

// ============================================================
// 74191: 4-bit up/down counter, async load
// ============================================================

TEST(Chip74191, CountUp) {
    auto [c, d] = setup(std::make_unique<Chip74191>("u1"));
    d->reset();
    S(*c, d, "LD", SignalValue::ONE);
    S(*c, d, "CTEN", SignalValue::ZERO);  // count enabled
    S(*c, d, "DU", SignalValue::ZERO);    // count up

    for (int i = 1; i <= 15; ++i) {
        auto out = clock_cycle(*c, d);
        EXPECT_EQ(read_q(out), static_cast<uint32_t>(i)) << "Up count " << i;
    }

    // Wrap: 15→0
    auto out = clock_cycle(*c, d);
    EXPECT_EQ(read_q(out), 0u) << "Wrap from 15 to 0";
}

TEST(Chip74191, CountDown) {
    auto [c, d] = setup(std::make_unique<Chip74191>("u1"));
    d->reset();
    S(*c, d, "LD", SignalValue::ONE);
    S(*c, d, "CTEN", SignalValue::ZERO);
    S(*c, d, "DU", SignalValue::ONE);  // count down

    // From 0 down → wraps to 15
    auto out = clock_cycle(*c, d);
    EXPECT_EQ(read_q(out), 15u) << "Down from 0 wraps to 15";

    out = clock_cycle(*c, d);
    EXPECT_EQ(read_q(out), 14u) << "Down from 15 to 14";
}

TEST(Chip74191, AsyncLoad) {
    auto [c, d] = setup(std::make_unique<Chip74191>("u1"));
    d->reset();
    S(*c, d, "CTEN", SignalValue::ZERO);
    S(*c, d, "DU", SignalValue::ZERO);

    clock_cycle(*c, d);

    // Async load overrides
    S(*c, d, "LD", SignalValue::ZERO);
    set_d(*c, d, 9);
    auto out = d->eval(c->nodes(), "LD");
    EXPECT_EQ(read_q(out), 9u) << "Async load should set Q immediately";
}

TEST(Chip74191, TerminalCount) {
    auto [c, d] = setup(std::make_unique<Chip74191>("u1"));
    d->reset();
    S(*c, d, "LD", SignalValue::ONE);
    S(*c, d, "CTEN", SignalValue::ZERO);
    S(*c, d, "DU", SignalValue::ZERO);

    // Count up to 15
    for (int i = 0; i < 15; ++i) clock_cycle(*c, d);

    // Now at count 15, CP was last set to 1. Check outputs without clock edge.
    S(*c, d, "CP", SignalValue::ZERO);  // CP goes low so MAXMIN works (looking for CP=0)
    auto out = d->eval(c->nodes(), "");
    EXPECT_EQ(read_q(out), 15u);
    EXPECT_EQ(out["MAXMIN"], SignalValue::ONE) << "MAXMIN should be 1 at terminal count";
}

// ============================================================
// 74160: BCD decade counter (mod 10)
// ============================================================

TEST(Chip74160, AsyncClear) {
    auto [c, d] = setup(std::make_unique<Chip74160>("u1"));
    d->reset();
    S(*c, d, "CR", SignalValue::ONE);
    S(*c, d, "LD", SignalValue::ONE);
    S(*c, d, "EP", SignalValue::ONE);
    S(*c, d, "ET", SignalValue::ONE);

    clock_cycle(*c, d);
    clock_cycle(*c, d);

    S(*c, d, "CR", SignalValue::ZERO);
    auto out = d->eval(c->nodes(), "CR");
    EXPECT_EQ(read_q(out), 0u) << "Async clear resets to 0";
}

TEST(Chip74160, BCDFrom0to9) {
    auto [c, d] = setup(std::make_unique<Chip74160>("u1"));
    d->reset();
    S(*c, d, "CR", SignalValue::ONE);
    S(*c, d, "LD", SignalValue::ONE);
    S(*c, d, "EP", SignalValue::ONE);
    S(*c, d, "ET", SignalValue::ONE);

    for (int expected = 1; expected <= 9; ++expected) {
        auto out = clock_cycle(*c, d);
        EXPECT_EQ(read_q(out), static_cast<uint32_t>(expected))
            << "After " << expected << " clocks";
    }

    // 10th clock → wraps to 0
    auto out = clock_cycle(*c, d);
    EXPECT_EQ(read_q(out), 0u) << "BCD should wrap from 9 to 0";
}

TEST(Chip74160, SyncLoad) {
    auto [c, d] = setup(std::make_unique<Chip74160>("u1"));
    d->reset();
    S(*c, d, "CR", SignalValue::ONE);
    S(*c, d, "EP", SignalValue::ONE);
    S(*c, d, "ET", SignalValue::ONE);

    set_d(*c, d, 7);
    S(*c, d, "LD", SignalValue::ZERO);
    auto out = clock_cycle(*c, d);
    EXPECT_EQ(read_q(out), 7u) << "Sync load";
}

TEST(Chip74160, CarryOut) {
    auto [c, d] = setup(std::make_unique<Chip74160>("u1"));
    d->reset();
    S(*c, d, "CR", SignalValue::ONE);
    S(*c, d, "LD", SignalValue::ONE);
    S(*c, d, "EP", SignalValue::ONE);
    S(*c, d, "ET", SignalValue::ONE);

    // Count to 9
    for (int i = 0; i < 9; ++i) clock_cycle(*c, d);

    auto out = d->eval(c->nodes(), "");
    EXPECT_EQ(read_q(out), 9u);
    EXPECT_EQ(out["CO"], SignalValue::ONE) << "CO should be 1 at count 9";

    // Wrap to 0
    out = clock_cycle(*c, d);
    EXPECT_EQ(read_q(out), 0u);
    EXPECT_EQ(out["CO"], SignalValue::ZERO);
}

TEST(Chip74160, DisabledCount) {
    auto [c, d] = setup(std::make_unique<Chip74160>("u1"));
    d->reset();
    S(*c, d, "CR", SignalValue::ONE);
    S(*c, d, "LD", SignalValue::ONE);
    S(*c, d, "EP", SignalValue::ZERO);
    S(*c, d, "ET", SignalValue::ONE);

    auto out = clock_cycle(*c, d);
    EXPECT_EQ(read_q(out), 0u) << "Hold when EP=0";
}
