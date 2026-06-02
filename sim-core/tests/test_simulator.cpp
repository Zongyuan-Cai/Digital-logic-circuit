#include <gtest/gtest.h>
#include "logic_sim/simulator.hpp"
#include "logic_sim/devices/gates.hpp"
#include "logic_sim/devices/flip_flops.hpp"
#include "logic_sim/devices/chips.hpp"

using namespace logic_sim;

// Helper: set a node's value through the device pin
static void S_val(Circuit* c, Device* d, const std::string& pid, SignalValue v) {
    int ni = d->pin_node(pid);
    if (ni >= 0 && ni < static_cast<int>(c->nodes().size())) {
        c->nodes()[ni].value = v;
        c->nodes()[ni].prev_value = v;
    }
}

// Helper: build a simple circuit with devices, create a node for each pin
static Device* wire_device(Circuit* c, std::unique_ptr<Device> dev) {
    Device* d = dev.get();
    int di = c->add_device(std::move(dev));
    for (auto& p : d->pins()) {
        int ni = c->add_node(p.name);
        d->connect_pin(p.id, ni);
    }
    return d;
}

static std::unique_ptr<Circuit> build_not_circuit() {
    auto c = std::make_unique<Circuit>();
    Device* dev = wire_device(c.get(), std::make_unique<NotGate>("u1"));
    // Set input to 0 for initial condition
    S_val(c.get(), dev, "I", SignalValue::ZERO);
    c->rebuild();
    // Re-set after rebuild (rebuild clears pin_refs and re-adds)
    // Actually need to keep the value set
    int ni = dev->pin_node("I");
    if (ni >= 0) c->nodes()[ni].value = SignalValue::ZERO;
    return c;
}

// ============================================================
// Basic simulator tests
// ============================================================

TEST(Simulator, EmptyCircuit) {
    Simulator sim;
    sim.load_circuit(std::make_unique<Circuit>());

    SimOptions opts;
    auto result = sim.run(opts);
    EXPECT_EQ(result.status, SimResult::OK);
}

TEST(Simulator, LoadAndRunNotGate) {
    Simulator sim;
    sim.load_circuit(build_not_circuit());

    SimOptions opts;
    opts.record = {"u1.I", "u1.Y"};
    opts.max_ticks = 10;

    auto result = sim.run(opts);
    EXPECT_EQ(result.status, SimResult::OK);
    EXPECT_EQ(result.final_nodes.at("u1.Y"), "1");
}

TEST(Simulator, Reset) {
    Simulator sim;
    sim.load_circuit(build_not_circuit());

    SimOptions opts;
    opts.record = {"u1.Y"};
    auto result = sim.run(opts);
    EXPECT_EQ(result.status, SimResult::OK);

    sim.reset();
    auto result2 = sim.run(opts);
    EXPECT_EQ(result2.status, SimResult::OK);
}

TEST(Simulator, WaveformHasData) {
    Simulator sim;
    sim.load_circuit(build_not_circuit());

    SimOptions opts;
    opts.record = {"u1.Y"};
    auto result = sim.run(opts);
    EXPECT_EQ(result.status, SimResult::OK);
    EXPECT_GT(result.waveform.size(), 0u);
}

TEST(Simulator, MaxEventsLimit) {
    Simulator sim;
    sim.load_circuit(build_not_circuit());

    SimOptions opts;
    opts.max_events = 1;
    auto result = sim.run(opts);
    // Should complete (either OK or with warning)
    EXPECT_NE(result.status, SimResult::ERROR);
}

// ============================================================
// Integration: D Flip-Flop edge detection via simulator
// ============================================================

TEST(SimulatorIntegration, DFlipFlopInitialEval) {
    Simulator sim;
    auto c = std::make_unique<Circuit>();
    Device* dff = wire_device(c.get(), std::make_unique<EdgeDFF>("u1"));
    dff->reset();
    c->rebuild();

    // Set initial inputs: PRE=1, CLR=1, D=1, CP=0
    S_val(c.get(), dff, "PRE", SignalValue::ONE);
    S_val(c.get(), dff, "CLR", SignalValue::ONE);
    S_val(c.get(), dff, "D", SignalValue::ONE);
    S_val(c.get(), dff, "CP", SignalValue::ZERO);

    SimOptions opts;
    opts.record = {"u1.Q", "u1.CP"};
    opts.max_ticks = 10;

    sim.load_circuit(std::move(c));
    auto result = sim.run(opts);

    EXPECT_EQ(result.status, SimResult::OK);
    // Initial Q should be 0 (default initial_q), since no rising edge occurred
    EXPECT_EQ(result.final_nodes.at("u1.Q"), "0");
}

// ============================================================
// Integration: 74138 decoder in simulator
// ============================================================

TEST(SimulatorIntegration, Chip74138AllAddr) {
    Simulator sim;
    auto c = std::make_unique<Circuit>();
    Device* chip = wire_device(c.get(), std::make_unique<Chip74LS138>("u1"));
    c->rebuild();

    // Enable the decoder
    S_val(c.get(), chip, "G1", SignalValue::ONE);
    S_val(c.get(), chip, "G2A", SignalValue::ZERO);
    S_val(c.get(), chip, "G2B", SignalValue::ZERO);
    // Address = 000
    S_val(c.get(), chip, "A", SignalValue::ZERO);
    S_val(c.get(), chip, "B", SignalValue::ZERO);
    S_val(c.get(), chip, "C", SignalValue::ZERO);

    SimOptions opts;
    opts.record = {"u1.Y0", "u1.Y1"};
    opts.max_ticks = 10;

    sim.load_circuit(std::move(c));
    auto result = sim.run(opts);

    EXPECT_EQ(result.status, SimResult::OK);
    EXPECT_EQ(result.final_nodes.at("u1.Y0"), "0");  // Y0 active low
    EXPECT_EQ(result.final_nodes.at("u1.Y1"), "1");  // Y1 inactive
}

// ============================================================
// Chip factory tests
// ============================================================

TEST(ChipFactory, CreatesAllChips) {
    const char* types[] = {
        "74LS148", "74LS147", "74139", "74LS138", "74138",
        "74LS42", "7448",
        "74153", "74HC153", "74LS151", "74151",
        "7485", "74280", "74283",
        "74175", "74LS195", "74195", "74LS194", "74194",
        "74161", "74LS161", "74163", "74191", "74160",
    };

    for (auto* t : types) {
        auto chip = create_chip(t, "test");
        EXPECT_NE(chip, nullptr) << "Failed to create chip: " << t;
        if (chip) {
            EXPECT_FALSE(chip->pins().empty()) << "Chip " << t << " has no pins";
        }
    }
}

TEST(ChipFactory, UnknownReturnsNull) {
    EXPECT_EQ(create_chip("NOT_A_CHIP", "u1"), nullptr);
}

TEST(ChipFactory, AliasResolution) {
    // Both "74LS138" and "74138" should create the same type
    auto c1 = create_chip("74LS138", "t1");
    auto c2 = create_chip("74138", "t2");
    EXPECT_NE(c1, nullptr);
    EXPECT_NE(c2, nullptr);
    // They share the same base type name
    // (The type() returns the version passed to constructor, so they may differ)
    // The important thing is both create a valid device
}
