#include <gtest/gtest.h>
#include "logic_sim/devices/flip_flops.hpp"
#include "logic_sim/circuit.hpp"

using namespace logic_sim;

// Helper: create a circuit with one device, wire all pins to nodes
static std::pair<std::unique_ptr<Circuit>, Device*>
setup_device(std::unique_ptr<Device> dev) {
    auto circuit = std::make_unique<Circuit>();
    Device* d = dev.get();
    int di = circuit->add_device(std::move(dev));
    for (auto& p : d->pins()) {
        int ni = circuit->add_node(p.name);
        d->connect_pin(p.id, ni);
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

static void set_val(Circuit& c, Device* dev, const std::string& pin_id, SignalValue v) {
    int ni = dev->pin_node(pin_id);
    if (ni >= 0) {
        c.nodes()[ni].prev_value = c.nodes()[ni].value;
        c.nodes()[ni].value = v;
    }
}

static SignalValue get_val(Device* dev, const std::vector<Node>& nodes,
                            const std::string& pin_id) {
    int ni = dev->pin_node(pin_id);
    return (ni >= 0) ? nodes[ni].value : SignalValue::X;
}

// ============================================================
// RS Latch NAND tests
// ============================================================

TEST(RSLatchNand, Set) {
    auto [c, d] = setup_device(std::make_unique<RSLatchNand>("u1"));
    d->reset();

    set_val(*c, d, "S_N", SignalValue::ZERO);  // active set
    set_val(*c, d, "R_N", SignalValue::ONE);
    auto out = d->eval(c->nodes(), "S_N");
    EXPECT_EQ(out["Q"], SignalValue::ONE);
    EXPECT_EQ(out["QN"], SignalValue::ZERO);
}

TEST(RSLatchNand, Reset) {
    auto [c, d] = setup_device(std::make_unique<RSLatchNand>("u1"));
    d->reset();

    set_val(*c, d, "S_N", SignalValue::ONE);
    set_val(*c, d, "R_N", SignalValue::ZERO);  // active reset
    auto out = d->eval(c->nodes(), "R_N");
    EXPECT_EQ(out["Q"], SignalValue::ZERO);
    EXPECT_EQ(out["QN"], SignalValue::ONE);
}

TEST(RSLatchNand, Hold) {
    auto [c, d] = setup_device(std::make_unique<RSLatchNand>("u1"));
    d->reset();

    // First set
    set_val(*c, d, "S_N", SignalValue::ZERO);
    set_val(*c, d, "R_N", SignalValue::ONE);
    d->eval(c->nodes(), "S_N");

    // Then hold
    set_val(*c, d, "S_N", SignalValue::ONE);
    set_val(*c, d, "R_N", SignalValue::ONE);
    auto out = d->eval(c->nodes(), "S_N");
    EXPECT_EQ(out["Q"], SignalValue::ONE);  // remains set
    EXPECT_EQ(out["QN"], SignalValue::ZERO);
}

TEST(RSLatchNand, Illegal) {
    auto [c, d] = setup_device(std::make_unique<RSLatchNand>("u1"));
    d->reset();

    set_val(*c, d, "S_N", SignalValue::ZERO);
    set_val(*c, d, "R_N", SignalValue::ZERO);  // both active → illegal
    auto out = d->eval(c->nodes(), "S_N");
    EXPECT_EQ(out["Q"], SignalValue::X);
    EXPECT_EQ(out["QN"], SignalValue::X);
}

TEST(RSLatchNand, InitialState) {
    auto dev = std::make_unique<RSLatchNand>("u1");
    dev->set_param_int("initial_q", 1);
    dev->reset();
    auto [c, d] = setup_device(std::move(dev));

    set_val(*c, d, "S_N", SignalValue::ONE);
    set_val(*c, d, "R_N", SignalValue::ONE);
    auto out = d->eval(c->nodes(), "");
    EXPECT_EQ(out["Q"], SignalValue::ONE);
    EXPECT_EQ(out["QN"], SignalValue::ZERO);
}

// ============================================================
// RS Latch NOR tests
// ============================================================

TEST(RSLatchNor, Set) {
    auto [c, d] = setup_device(std::make_unique<RSLatchNor>("u1"));
    d->reset();

    set_val(*c, d, "S", SignalValue::ONE);  // active set
    set_val(*c, d, "R", SignalValue::ZERO);
    auto out = d->eval(c->nodes(), "S");
    EXPECT_EQ(out["Q"], SignalValue::ONE);
}

TEST(RSLatchNor, Reset) {
    auto [c, d] = setup_device(std::make_unique<RSLatchNor>("u1"));
    d->reset();

    set_val(*c, d, "S", SignalValue::ZERO);
    set_val(*c, d, "R", SignalValue::ONE);  // active reset
    auto out = d->eval(c->nodes(), "R");
    EXPECT_EQ(out["Q"], SignalValue::ZERO);
}

TEST(RSLatchNor, Hold) {
    auto [c, d] = setup_device(std::make_unique<RSLatchNor>("u1"));
    d->reset();

    set_val(*c, d, "S", SignalValue::ZERO);
    set_val(*c, d, "R", SignalValue::ZERO);
    auto out = d->eval(c->nodes(), "");
    EXPECT_EQ(out["Q"], SignalValue::ZERO);  // initial was 0
}

TEST(RSLatchNor, Illegal) {
    auto [c, d] = setup_device(std::make_unique<RSLatchNor>("u1"));
    d->reset();

    set_val(*c, d, "S", SignalValue::ONE);
    set_val(*c, d, "R", SignalValue::ONE);  // illegal
    auto out = d->eval(c->nodes(), "S");
    EXPECT_EQ(out["Q"], SignalValue::X);
}

// ============================================================
// Gated RS Latch tests
// ============================================================

TEST(GatedRSLatch, Disabled) {
    auto [c, d] = setup_device(std::make_unique<GatedRSLatch>("u1"));
    d->reset();

    set_val(*c, d, "EN", SignalValue::ZERO);
    set_val(*c, d, "S", SignalValue::ONE);
    set_val(*c, d, "R", SignalValue::ZERO);
    auto out = d->eval(c->nodes(), "S");
    EXPECT_EQ(out["Q"], SignalValue::ZERO);  // initial Q=0, holds
}

TEST(GatedRSLatch, EnabledSet) {
    auto [c, d] = setup_device(std::make_unique<GatedRSLatch>("u1"));
    d->reset();

    set_val(*c, d, "EN", SignalValue::ONE);
    set_val(*c, d, "S", SignalValue::ONE);
    set_val(*c, d, "R", SignalValue::ZERO);
    auto out = d->eval(c->nodes(), "EN");
    EXPECT_EQ(out["Q"], SignalValue::ONE);
}

TEST(GatedRSLatch, EnabledReset) {
    auto [c, d] = setup_device(std::make_unique<GatedRSLatch>("u1"));
    d->reset();

    set_val(*c, d, "EN", SignalValue::ONE);
    set_val(*c, d, "S", SignalValue::ZERO);
    set_val(*c, d, "R", SignalValue::ONE);
    auto out = d->eval(c->nodes(), "EN");
    EXPECT_EQ(out["Q"], SignalValue::ZERO);
}

// ============================================================
// D Latch tests
// ============================================================

TEST(DLatch, TransparentHigh) {
    auto [c, d] = setup_device(std::make_unique<DLatch>("u1"));
    d->reset();

    set_val(*c, d, "EN", SignalValue::ONE);
    set_val(*c, d, "D", SignalValue::ONE);
    auto out = d->eval(c->nodes(), "D");
    EXPECT_EQ(out["Q"], SignalValue::ONE);

    set_val(*c, d, "D", SignalValue::ZERO);
    out = d->eval(c->nodes(), "D");
    EXPECT_EQ(out["Q"], SignalValue::ZERO);
}

TEST(DLatch, HoldWhenDisabled) {
    auto [c, d] = setup_device(std::make_unique<DLatch>("u1"));
    d->reset();

    // Set Q=1 when transparent
    set_val(*c, d, "EN", SignalValue::ONE);
    set_val(*c, d, "D", SignalValue::ONE);
    d->eval(c->nodes(), "D");

    // Disable and change D
    set_val(*c, d, "EN", SignalValue::ZERO);
    set_val(*c, d, "D", SignalValue::ZERO);
    auto out = d->eval(c->nodes(), "EN");
    EXPECT_EQ(out["Q"], SignalValue::ONE);  // holds value
}

// ============================================================
// Sync D Flip-Flop tests
// ============================================================

TEST(SyncDFF, RisingEdge) {
    auto [c, d] = setup_device(std::make_unique<SyncDFF>("u1"));
    d->reset();

    // Set D, but no edge yet
    set_val(*c, d, "D", SignalValue::ONE);
    set_val(*c, d, "CP", SignalValue::ZERO);
    auto out = d->eval(c->nodes(), "D");
    EXPECT_EQ(out["Q"], SignalValue::ZERO);  // initial

    // Rising edge
    set_val(*c, d, "CP", SignalValue::ONE);  // 0→1
    out = d->eval(c->nodes(), "CP");
    EXPECT_EQ(out["Q"], SignalValue::ONE);  // Q gets D
}

TEST(SyncDFF, NoChangeOnFallingEdge) {
    auto [c, d] = setup_device(std::make_unique<SyncDFF>("u1"));
    d->reset();

    // Rising edge to set Q=1
    set_val(*c, d, "D", SignalValue::ONE);
    set_val(*c, d, "CP", SignalValue::ZERO);
    d->eval(c->nodes(), "");
    set_val(*c, d, "CP", SignalValue::ONE);
    d->eval(c->nodes(), "CP");

    // Change D then falling edge
    set_val(*c, d, "D", SignalValue::ZERO);
    set_val(*c, d, "CP", SignalValue::ZERO);
    auto out = d->eval(c->nodes(), "CP");
    EXPECT_EQ(out["Q"], SignalValue::ONE);  // no change on falling edge
}

TEST(SyncDFF, InitialState) {
    auto dev = std::make_unique<SyncDFF>("u1");
    dev->set_param_int("initial_q", 1);
    auto [c, d] = setup_device(std::move(dev));
    d->reset();

    set_val(*c, d, "D", SignalValue::ZERO);
    set_val(*c, d, "CP", SignalValue::ZERO);
    auto out = d->eval(c->nodes(), "");
    EXPECT_EQ(out["Q"], SignalValue::ONE);  // initial Q=1
}

TEST(SyncDFF, UnknownInput) {
    auto [c, d] = setup_device(std::make_unique<SyncDFF>("u1"));
    d->reset();

    set_val(*c, d, "D", SignalValue::X);
    set_val(*c, d, "CP", SignalValue::ZERO);
    d->eval(c->nodes(), "");
    set_val(*c, d, "CP", SignalValue::ONE);
    auto out = d->eval(c->nodes(), "CP");
    EXPECT_EQ(out["Q"], SignalValue::X);
}

// ============================================================
// Sync JK Flip-Flop tests
// ============================================================

TEST(SyncJKFF, JK00Hold) {
    auto [c, d] = setup_device(std::make_unique<SyncJKFF>("u1"));
    d->reset();

    set_val(*c, d, "J", SignalValue::ZERO);
    set_val(*c, d, "K", SignalValue::ZERO);
    set_val(*c, d, "CP", SignalValue::ZERO);
    d->eval(c->nodes(), "");
    set_val(*c, d, "CP", SignalValue::ONE);
    auto out = d->eval(c->nodes(), "CP");
    EXPECT_EQ(out["Q"], SignalValue::ZERO);  // hold
}

TEST(SyncJKFF, JK01Reset) {
    auto [c, d] = setup_device(std::make_unique<SyncJKFF>("u1"));
    d->reset();
    d->set_param_int("initial_q", 1);
    d->reset();

    set_val(*c, d, "J", SignalValue::ZERO);
    set_val(*c, d, "K", SignalValue::ONE);
    set_val(*c, d, "CP", SignalValue::ZERO);
    d->eval(c->nodes(), "");
    set_val(*c, d, "CP", SignalValue::ONE);
    auto out = d->eval(c->nodes(), "CP");
    EXPECT_EQ(out["Q"], SignalValue::ZERO);  // reset
}

TEST(SyncJKFF, JK10Set) {
    auto [c, d] = setup_device(std::make_unique<SyncJKFF>("u1"));
    d->reset();

    set_val(*c, d, "J", SignalValue::ONE);
    set_val(*c, d, "K", SignalValue::ZERO);
    set_val(*c, d, "CP", SignalValue::ZERO);
    d->eval(c->nodes(), "");
    set_val(*c, d, "CP", SignalValue::ONE);
    auto out = d->eval(c->nodes(), "CP");
    EXPECT_EQ(out["Q"], SignalValue::ONE);  // set
}

TEST(SyncJKFF, JK11Toggle) {
    auto [c, d] = setup_device(std::make_unique<SyncJKFF>("u1"));
    d->reset();

    set_val(*c, d, "J", SignalValue::ONE);
    set_val(*c, d, "K", SignalValue::ONE);
    set_val(*c, d, "CP", SignalValue::ZERO);
    d->eval(c->nodes(), "");
    set_val(*c, d, "CP", SignalValue::ONE);
    auto out = d->eval(c->nodes(), "CP");
    EXPECT_EQ(out["Q"], SignalValue::ONE);  // toggle from 0→1

    set_val(*c, d, "CP", SignalValue::ZERO);
    d->eval(c->nodes(), "CP");
    set_val(*c, d, "CP", SignalValue::ONE);
    out = d->eval(c->nodes(), "CP");
    EXPECT_EQ(out["Q"], SignalValue::ZERO);  // toggle from 1→0
}

// ============================================================
// Sync T Flip-Flop tests
// ============================================================

TEST(SyncTFF, T0Hold) {
    auto [c, d] = setup_device(std::make_unique<SyncTFF>("u1"));
    d->reset();

    set_val(*c, d, "T", SignalValue::ZERO);
    set_val(*c, d, "CP", SignalValue::ZERO);
    d->eval(c->nodes(), "");
    set_val(*c, d, "CP", SignalValue::ONE);
    auto out = d->eval(c->nodes(), "CP");
    EXPECT_EQ(out["Q"], SignalValue::ZERO);  // hold
}

TEST(SyncTFF, T1Toggle) {
    auto [c, d] = setup_device(std::make_unique<SyncTFF>("u1"));
    d->reset();

    set_val(*c, d, "T", SignalValue::ONE);
    set_val(*c, d, "CP", SignalValue::ZERO);
    d->eval(c->nodes(), "");
    set_val(*c, d, "CP", SignalValue::ONE);
    auto out = d->eval(c->nodes(), "CP");
    EXPECT_EQ(out["Q"], SignalValue::ONE);  // toggle 0→1

    set_val(*c, d, "CP", SignalValue::ZERO);
    d->eval(c->nodes(), "CP");
    set_val(*c, d, "CP", SignalValue::ONE);
    out = d->eval(c->nodes(), "CP");
    EXPECT_EQ(out["Q"], SignalValue::ZERO);  // toggle 1→0
}

// ============================================================
// Master-Slave D Flip-Flop tests
// ============================================================

TEST(MasterSlaveDFF, FallingEdgeTransfer) {
    auto [c, d] = setup_device(std::make_unique<MasterSlaveDFF>("u1"));
    d->reset();

    // Master transparent when CP=1
    set_val(*c, d, "D", SignalValue::ONE);
    set_val(*c, d, "CP", SignalValue::ZERO);
    d->eval(c->nodes(), "");
    set_val(*c, d, "CP", SignalValue::ONE);
    d->eval(c->nodes(), "CP");  // master captures

    // Slave transfers on falling edge
    set_val(*c, d, "CP", SignalValue::ZERO);
    auto out = d->eval(c->nodes(), "CP");
    EXPECT_EQ(out["Q"], SignalValue::ONE);  // slave gets master value
}

// ============================================================
// Edge D Flip-Flop tests (with PRE/CLR)
// ============================================================

TEST(EdgeDFF, AsyncClear) {
    auto [c, d] = setup_device(std::make_unique<EdgeDFF>("u1"));
    d->reset();

    // Set Q=1 first
    set_val(*c, d, "PRE", SignalValue::ONE);
    set_val(*c, d, "CLR", SignalValue::ONE);
    set_val(*c, d, "D", SignalValue::ONE);
    set_val(*c, d, "CP", SignalValue::ZERO);
    d->eval(c->nodes(), "");
    set_val(*c, d, "CP", SignalValue::ONE);
    d->eval(c->nodes(), "CP");

    // Async clear
    set_val(*c, d, "CLR", SignalValue::ZERO);
    auto out = d->eval(c->nodes(), "CLR");
    EXPECT_EQ(out["Q"], SignalValue::ZERO);
}

TEST(EdgeDFF, AsyncPreset) {
    auto [c, d] = setup_device(std::make_unique<EdgeDFF>("u1"));
    d->reset();

    set_val(*c, d, "PRE", SignalValue::ZERO);  // async preset
    set_val(*c, d, "CLR", SignalValue::ONE);
    auto out = d->eval(c->nodes(), "PRE");
    EXPECT_EQ(out["Q"], SignalValue::ONE);
}

TEST(EdgeDFF, AsyncPriority) {
    auto [c, d] = setup_device(std::make_unique<EdgeDFF>("u1"));
    d->reset();

    // CLR overrides clock
    set_val(*c, d, "CLR", SignalValue::ZERO);
    set_val(*c, d, "D", SignalValue::ONE);
    set_val(*c, d, "CP", SignalValue::ZERO);
    d->eval(c->nodes(), "");
    set_val(*c, d, "CP", SignalValue::ONE);
    auto out = d->eval(c->nodes(), "CLR");
    EXPECT_EQ(out["Q"], SignalValue::ZERO);  // clear wins
}

// ============================================================
// Edge JK Flip-Flop tests
// ============================================================

TEST(EdgeJKFF, AsyncClearOverride) {
    auto [c, d] = setup_device(std::make_unique<EdgeJKFF>("u1"));
    d->reset();

    set_val(*c, d, "PRE", SignalValue::ONE);
    set_val(*c, d, "CLR", SignalValue::ZERO);
    auto out = d->eval(c->nodes(), "CLR");
    EXPECT_EQ(out["Q"], SignalValue::ZERO);
}

TEST(EdgeJKFF, ToggleOnRisingEdge) {
    auto [c, d] = setup_device(std::make_unique<EdgeJKFF>("u1"));
    d->reset();

    set_val(*c, d, "PRE", SignalValue::ONE);
    set_val(*c, d, "CLR", SignalValue::ONE);
    set_val(*c, d, "J", SignalValue::ONE);
    set_val(*c, d, "K", SignalValue::ONE);
    set_val(*c, d, "CP", SignalValue::ZERO);
    d->eval(c->nodes(), "");
    set_val(*c, d, "CP", SignalValue::ONE);
    auto out = d->eval(c->nodes(), "CP");
    EXPECT_EQ(out["Q"], SignalValue::ONE);  // toggle
}

// ============================================================
// Blocking JK Flip-Flop tests
// ============================================================

TEST(BlockingJKFF, PositiveEdgeJK) {
    auto [c, d] = setup_device(std::make_unique<BlockingJKFF>("u1"));
    d->reset();

    set_val(*c, d, "J", SignalValue::ONE);
    set_val(*c, d, "K", SignalValue::ZERO);
    set_val(*c, d, "CP", SignalValue::ZERO);
    d->eval(c->nodes(), "");
    set_val(*c, d, "CP", SignalValue::ONE);
    auto out = d->eval(c->nodes(), "CP");
    EXPECT_EQ(out["Q"], SignalValue::ONE);  // set on rising edge
}

// ============================================================
// Factory tests
// ============================================================

TEST(FlipFlopFactory, CreatesCorrectType) {
    auto ff = create_flip_flop("D_LATCH", "u1");
    EXPECT_EQ(ff->type(), "D_LATCH");

    auto ff2 = create_flip_flop("EDGE_JK_FF", "u2");
    EXPECT_EQ(ff2->type(), "EDGE_JK_FF");

    auto ff3 = create_flip_flop("NONEXISTENT", "u3");
    EXPECT_EQ(ff3, nullptr);
}

TEST(FlipFlopFactory, AllTypesCreatable) {
    const char* types[] = {
        "RS_LATCH_NAND", "RS_LATCH_NOR", "GATED_RS_LATCH", "D_LATCH",
        "SYNC_RS_FF", "SYNC_D_FF", "SYNC_JK_FF", "SYNC_T_FF",
        "MASTER_SLAVE_RS_FF", "MASTER_SLAVE_D_FF", "MASTER_SLAVE_JK_FF",
        "EDGE_D_FF", "EDGE_JK_FF", "BLOCKING_JK_FF"
    };
    for (auto t : types) {
        auto ff = create_flip_flop(t, "test");
        EXPECT_NE(ff, nullptr) << "Failed to create " << t;
    }
}
