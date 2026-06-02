#include "logic_sim/devices/flip_flops.hpp"

namespace logic_sim {

// ============================================================
// RSLatchNand
// S_N and R_N are active-low.
// S_N=0,R_N=1 → Q=1 (set)
// S_N=1,R_N=0 → Q=0 (reset)
// S_N=1,R_N=1 → hold
// S_N=0,R_N=0 → Q=X,QN=X (illegal)
// ============================================================

RSLatchNand::RSLatchNand(const std::string& id)
    : Device(id, "RS_LATCH_NAND")
{
    add_pin("S_N", "S_N", Pin::INPUT, Pin::ACTIVE_LOW, "set");
    add_pin("R_N", "R_N", Pin::INPUT, Pin::ACTIVE_LOW, "reset");
    add_pin("Q",   "Q",   Pin::OUTPUT);
    add_pin("QN",  "QN",  Pin::OUTPUT);
    set_param_int("initial_q", 0);
}

void RSLatchNand::reset() {
    int init = get_param_int("initial_q", 0);
    set_param("q", init ? SignalValue::ONE : SignalValue::ZERO);
    set_param("qn", init ? SignalValue::ZERO : SignalValue::ONE);
}

std::map<std::string, SignalValue> RSLatchNand::eval(
    const std::vector<Node>& nodes, const std::string& changed) {
    SignalValue s_n = input("S_N", nodes);
    SignalValue r_n = input("R_N", nodes);

    SignalValue q  = get_param("q", SignalValue::ZERO);
    SignalValue qn = get_param("qn", SignalValue::ONE);

    // Convert unknown inputs
    bool s_unknown = (s_n == SignalValue::X || s_n == SignalValue::Z);
    bool r_unknown = (r_n == SignalValue::X || r_n == SignalValue::Z);

    if (s_unknown || r_unknown) {
        // If either is unknown and not a strong 0, output goes X
        if (s_n != SignalValue::ZERO && r_n != SignalValue::ZERO) {
            // Both are non-zero - could be hold or illegal
            if (s_n == SignalValue::ONE && r_n == SignalValue::ONE) {
                // Hold: keep current
            } else {
                q = SignalValue::X;
                qn = SignalValue::X;
            }
        } else if (s_n == SignalValue::ZERO && r_n == SignalValue::ZERO) {
            q = SignalValue::X;
            qn = SignalValue::X;
        }
    } else if (s_n == SignalValue::ZERO && r_n == SignalValue::ZERO) {
        // Illegal
        q  = SignalValue::X;
        qn = SignalValue::X;
    } else if (s_n == SignalValue::ZERO && r_n == SignalValue::ONE) {
        q  = SignalValue::ONE;
        qn = SignalValue::ZERO;
    } else if (s_n == SignalValue::ONE && r_n == SignalValue::ZERO) {
        q  = SignalValue::ZERO;
        qn = SignalValue::ONE;
    }
    // else: S_N=1, R_N=1 → hold

    set_param("q", q);
    set_param("qn", qn);
    return {{"Q", q}, {"QN", qn}};
}

// ============================================================
// RSLatchNor
// S and R are active-high.
// S=1,R=0 → Q=1 (set)
// S=0,R=1 → Q=0 (reset)
// S=0,R=0 → hold
// S=1,R=1 → Q=X (illegal)
// ============================================================

RSLatchNor::RSLatchNor(const std::string& id)
    : Device(id, "RS_LATCH_NOR")
{
    add_pin("S", "S", Pin::INPUT, Pin::ACTIVE_HIGH, "set");
    add_pin("R", "R", Pin::INPUT, Pin::ACTIVE_HIGH, "reset");
    add_pin("Q", "Q", Pin::OUTPUT);
    add_pin("QN", "QN", Pin::OUTPUT);
    set_param_int("initial_q", 0);
}

void RSLatchNor::reset() {
    int init = get_param_int("initial_q", 0);
    set_param("q", init ? SignalValue::ONE : SignalValue::ZERO);
    set_param("qn", init ? SignalValue::ZERO : SignalValue::ONE);
}

std::map<std::string, SignalValue> RSLatchNor::eval(
    const std::vector<Node>& nodes, const std::string&) {
    SignalValue s = input("S", nodes);
    SignalValue r = input("R", nodes);

    SignalValue q  = get_param("q", SignalValue::ZERO);
    SignalValue qn = get_param("qn", SignalValue::ONE);

    if (s == SignalValue::X || s == SignalValue::Z ||
        r == SignalValue::X || r == SignalValue::Z) {
        // Unknown input handling
        if (s == SignalValue::ONE && r == SignalValue::ONE) {
            q = SignalValue::X; qn = SignalValue::X;
        } else if (s == SignalValue::ONE && r == SignalValue::ZERO) {
            q = SignalValue::ONE; qn = SignalValue::ZERO;
        } else if (s == SignalValue::ZERO && r == SignalValue::ONE) {
            q = SignalValue::ZERO; qn = SignalValue::ONE;
        }
        // Otherwise hold or indeterminate
        if (s != SignalValue::ZERO && s != SignalValue::ONE &&
            r != SignalValue::ZERO && r != SignalValue::ONE) {
            // Don't change on pure Z
        }
    } else if (s == SignalValue::ONE && r == SignalValue::ONE) {
        q = SignalValue::X;
        qn = SignalValue::X;
    } else if (s == SignalValue::ONE) {
        q  = SignalValue::ONE;
        qn = SignalValue::ZERO;
    } else if (r == SignalValue::ONE) {
        q  = SignalValue::ZERO;
        qn = SignalValue::ONE;
    }

    set_param("q", q);
    set_param("qn", qn);
    return {{"Q", q}, {"QN", qn}};
}

// ============================================================
// GatedRSLatch
// ============================================================

GatedRSLatch::GatedRSLatch(const std::string& id)
    : Device(id, "GATED_RS_LATCH")
{
    add_pin("S", "S", Pin::INPUT);
    add_pin("R", "R", Pin::INPUT);
    add_pin("EN", "EN", Pin::INPUT, Pin::ACTIVE_HIGH, "enable");
    add_pin("Q", "Q", Pin::OUTPUT);
    add_pin("QN", "QN", Pin::OUTPUT);
    set_param_int("initial_q", 0);
}

void GatedRSLatch::reset() {
    int init = get_param_int("initial_q", 0);
    set_param("q", init ? SignalValue::ONE : SignalValue::ZERO);
    set_param("qn", init ? SignalValue::ZERO : SignalValue::ONE);
}

std::map<std::string, SignalValue> GatedRSLatch::eval(
    const std::vector<Node>& nodes, const std::string&) {
    SignalValue en = input("EN", nodes);
    SignalValue s  = input("S", nodes);
    SignalValue r  = input("R", nodes);

    SignalValue q  = get_param("q", SignalValue::ZERO);
    SignalValue qn = get_param("qn", SignalValue::ONE);

    if (en == SignalValue::ONE) {
        // Enabled: behaves like RS latch
        if (s == SignalValue::X || s == SignalValue::Z ||
            r == SignalValue::X || r == SignalValue::Z) {
            if (s == SignalValue::ONE && r == SignalValue::ZERO) {
                q = SignalValue::ONE; qn = SignalValue::ZERO;
            } else if (s == SignalValue::ZERO && r == SignalValue::ONE) {
                q = SignalValue::ZERO; qn = SignalValue::ONE;
            } else if (s == SignalValue::ONE && r == SignalValue::ONE) {
                q = SignalValue::X; qn = SignalValue::X;
            }
            // else hold with some uncertainty
        } else if (s == SignalValue::ONE && r == SignalValue::ZERO) {
            q = SignalValue::ONE; qn = SignalValue::ZERO;
        } else if (s == SignalValue::ZERO && r == SignalValue::ONE) {
            q = SignalValue::ZERO; qn = SignalValue::ONE;
        } else if (s == SignalValue::ONE && r == SignalValue::ONE) {
            q = SignalValue::X; qn = SignalValue::X;
        }
        // S=0,R=0 → hold
    }
    // EN=0 → hold

    set_param("q", q);
    set_param("qn", qn);
    return {{"Q", q}, {"QN", qn}};
}

// ============================================================
// DLatch (transparent D latch)
// ============================================================

DLatch::DLatch(const std::string& id)
    : Device(id, "D_LATCH")
{
    add_pin("D", "D", Pin::INPUT, Pin::ACTIVE_HIGH, "data");
    add_pin("EN", "EN", Pin::INPUT, Pin::ACTIVE_HIGH, "enable");
    add_pin("Q", "Q", Pin::OUTPUT);
    add_pin("QN", "QN", Pin::OUTPUT);
    set_param_int("initial_q", 0);
}

void DLatch::reset() {
    int init = get_param_int("initial_q", 0);
    set_param("q", init ? SignalValue::ONE : SignalValue::ZERO);
    set_param("qn", init ? SignalValue::ZERO : SignalValue::ONE);
}

std::map<std::string, SignalValue> DLatch::eval(
    const std::vector<Node>& nodes, const std::string&) {
    SignalValue en = input("EN", nodes);
    SignalValue d  = input("D", nodes);

    SignalValue q  = get_param("q", SignalValue::ZERO);
    SignalValue qn = get_param("qn", SignalValue::ONE);

    if (en == SignalValue::ONE) {
        // Transparent: Q follows D
        q = d;
        if (d == SignalValue::X || d == SignalValue::Z) {
            q = SignalValue::X;
        }
        qn = signal_not(q);
    }
    // EN=0 or X/Z: hold

    set_param("q", q);
    set_param("qn", qn);
    return {{"Q", q}, {"QN", qn}};
}

// ============================================================
// Helper: evaluate JK function
// ============================================================
static std::pair<SignalValue, SignalValue> jk_eval(
    SignalValue j, SignalValue k, SignalValue q) {
    SignalValue q_next, qn_next;
    // Only use strong values for J/K logic
    bool j_high = (j == SignalValue::ONE);
    bool k_high = (k == SignalValue::ONE);
    bool j_low  = (j == SignalValue::ZERO);
    bool k_low  = (k == SignalValue::ZERO);
    bool j_bad  = (j == SignalValue::X || j == SignalValue::Z);
    bool k_bad  = (k == SignalValue::X || k == SignalValue::Z);

    if (j_bad || k_bad) {
        q_next = SignalValue::X;
        qn_next = SignalValue::X;
    } else if (j_low && k_low) {
        // Hold
        q_next = q;
        qn_next = signal_not(q);
    } else if (j_low && k_high) {
        // Reset
        q_next = SignalValue::ZERO;
        qn_next = SignalValue::ONE;
    } else if (j_high && k_low) {
        // Set
        q_next = SignalValue::ONE;
        qn_next = SignalValue::ZERO;
    } else {
        // j_high && k_high → Toggle
        if (q == SignalValue::ZERO) {
            q_next = SignalValue::ONE;
            qn_next = SignalValue::ZERO;
        } else if (q == SignalValue::ONE) {
            q_next = SignalValue::ZERO;
            qn_next = SignalValue::ONE;
        } else {
            q_next = SignalValue::X;
            qn_next = SignalValue::X;
        }
    }
    return {q_next, qn_next};
}

// ============================================================
// SyncRSFF
// ============================================================

SyncRSFF::SyncRSFF(const std::string& id)
    : Device(id, "SYNC_RS_FF")
{
    add_pin("S", "S", Pin::INPUT, Pin::ACTIVE_HIGH, "set");
    add_pin("R", "R", Pin::INPUT, Pin::ACTIVE_HIGH, "reset");
    add_pin("CP", "CP", Pin::INPUT, Pin::ACTIVE_HIGH, "clock");
    add_pin("Q", "Q", Pin::OUTPUT);
    add_pin("QN", "QN", Pin::OUTPUT);
    set_param_int("initial_q", 0);
    sig_params_["edge"] = SignalValue::ONE;  // rising edge
}

void SyncRSFF::reset() {
    int init = get_param_int("initial_q", 0);
    set_param("q", init ? SignalValue::ONE : SignalValue::ZERO);
    set_param("qn", init ? SignalValue::ZERO : SignalValue::ONE);
}

std::map<std::string, SignalValue> SyncRSFF::eval(
    const std::vector<Node>& nodes, const std::string& changed) {
    std::string edge_type = trigger_edge();
    bool edge = (edge_type == "falling") ? falling_edge("CP", nodes)
                                          : rising_edge("CP", nodes);
    SignalValue q = get_param("q", SignalValue::ZERO);

    if (edge) {
        SignalValue s = input("S", nodes);
        SignalValue r = input("R", nodes);

        if (s == SignalValue::X || s == SignalValue::Z ||
            r == SignalValue::X || r == SignalValue::Z) {
            q = SignalValue::X;
        } else if (s == SignalValue::ONE && r == SignalValue::ZERO) {
            q = SignalValue::ONE;
        } else if (s == SignalValue::ZERO && r == SignalValue::ONE) {
            q = SignalValue::ZERO;
        } else if (s == SignalValue::ONE && r == SignalValue::ONE) {
            q = SignalValue::X;  // illegal
        }
        // S=0,R=0 → hold
    }

    SignalValue qn = signal_not(q);
    set_param("q", q);
    set_param("qn", qn);
    return {{"Q", q}, {"QN", qn}};
}

// ============================================================
// SyncDFF
// ============================================================

SyncDFF::SyncDFF(const std::string& id)
    : Device(id, "SYNC_D_FF")
{
    add_pin("D", "D", Pin::INPUT, Pin::ACTIVE_HIGH, "data");
    add_pin("CP", "CP", Pin::INPUT, Pin::ACTIVE_HIGH, "clock");
    add_pin("Q", "Q", Pin::OUTPUT);
    add_pin("QN", "QN", Pin::OUTPUT);
    set_param_int("initial_q", 0);
    sig_params_["edge"] = SignalValue::ONE;  // rising edge
}

void SyncDFF::reset() {
    int init = get_param_int("initial_q", 0);
    set_param("q", init ? SignalValue::ONE : SignalValue::ZERO);
    set_param("qn", init ? SignalValue::ZERO : SignalValue::ONE);
}

std::map<std::string, SignalValue> SyncDFF::eval(
    const std::vector<Node>& nodes, const std::string& changed) {
    std::string edge_type = trigger_edge();
    bool edge = (edge_type == "falling") ? falling_edge("CP", nodes)
                                          : rising_edge("CP", nodes);
    SignalValue q = get_param("q", SignalValue::ZERO);

    if (edge) {
        SignalValue d = input("D", nodes);
        q = (d == SignalValue::X || d == SignalValue::Z) ? SignalValue::X : d;
    }

    SignalValue qn = signal_not(q);
    set_param("q", q);
    set_param("qn", qn);
    return {{"Q", q}, {"QN", qn}};
}

// ============================================================
// SyncJKFF
// ============================================================

SyncJKFF::SyncJKFF(const std::string& id)
    : Device(id, "SYNC_JK_FF")
{
    add_pin("J", "J", Pin::INPUT, Pin::ACTIVE_HIGH, "j_input");
    add_pin("K", "K", Pin::INPUT, Pin::ACTIVE_HIGH, "k_input");
    add_pin("CP", "CP", Pin::INPUT, Pin::ACTIVE_HIGH, "clock");
    add_pin("Q", "Q", Pin::OUTPUT);
    add_pin("QN", "QN", Pin::OUTPUT);
    set_param_int("initial_q", 0);
    sig_params_["edge"] = SignalValue::ONE;
}

void SyncJKFF::reset() {
    int init = get_param_int("initial_q", 0);
    set_param("q", init ? SignalValue::ONE : SignalValue::ZERO);
    set_param("qn", init ? SignalValue::ZERO : SignalValue::ONE);
}

std::map<std::string, SignalValue> SyncJKFF::eval(
    const std::vector<Node>& nodes, const std::string& changed) {
    std::string edge_type = trigger_edge();
    bool edge = (edge_type == "falling") ? falling_edge("CP", nodes)
                                          : rising_edge("CP", nodes);
    SignalValue q = get_param("q", SignalValue::ZERO);

    if (edge) {
        SignalValue j = input("J", nodes);
        SignalValue k = input("K", nodes);
        auto [q_next, qn_next] = jk_eval(j, k, q);
        q = q_next;
    }

    SignalValue qn = signal_not(q);
    set_param("q", q);
    set_param("qn", qn);
    return {{"Q", q}, {"QN", qn}};
}

// ============================================================
// SyncTFF
// ============================================================

SyncTFF::SyncTFF(const std::string& id)
    : Device(id, "SYNC_T_FF")
{
    add_pin("T", "T", Pin::INPUT, Pin::ACTIVE_HIGH, "toggle");
    add_pin("CP", "CP", Pin::INPUT, Pin::ACTIVE_HIGH, "clock");
    add_pin("Q", "Q", Pin::OUTPUT);
    add_pin("QN", "QN", Pin::OUTPUT);
    set_param_int("initial_q", 0);
    sig_params_["edge"] = SignalValue::ONE;
}

void SyncTFF::reset() {
    int init = get_param_int("initial_q", 0);
    set_param("q", init ? SignalValue::ONE : SignalValue::ZERO);
    set_param("qn", init ? SignalValue::ZERO : SignalValue::ONE);
}

std::map<std::string, SignalValue> SyncTFF::eval(
    const std::vector<Node>& nodes, const std::string& changed) {
    std::string edge_type = trigger_edge();
    bool edge = (edge_type == "falling") ? falling_edge("CP", nodes)
                                          : rising_edge("CP", nodes);
    SignalValue q = get_param("q", SignalValue::ZERO);

    if (edge) {
        SignalValue t = input("T", nodes);
        if (t == SignalValue::ONE) {
            // Toggle
            q = (q == SignalValue::ZERO) ? SignalValue::ONE :
                (q == SignalValue::ONE)  ? SignalValue::ZERO : SignalValue::X;
        } else if (t != SignalValue::ZERO) {
            q = SignalValue::X;  // X/Z input → X
        }
        // T=0 → hold
    }

    SignalValue qn = signal_not(q);
    set_param("q", q);
    set_param("qn", qn);
    return {{"Q", q}, {"QN", qn}};
}

// ============================================================
// MasterSlaveRSFF
// Master latches when CP=1, slave transfers when CP→0
// ============================================================

MasterSlaveRSFF::MasterSlaveRSFF(const std::string& id)
    : Device(id, "MASTER_SLAVE_RS_FF")
{
    add_pin("S", "S", Pin::INPUT);
    add_pin("R", "R", Pin::INPUT);
    add_pin("CP", "CP", Pin::INPUT, Pin::ACTIVE_HIGH, "clock");
    add_pin("Q", "Q", Pin::OUTPUT);
    add_pin("QN", "QN", Pin::OUTPUT);
    set_param_int("initial_q", 0);
}

void MasterSlaveRSFF::reset() {
    int init = get_param_int("initial_q", 0);
    set_param("q", init ? SignalValue::ONE : SignalValue::ZERO);
    set_param("qn", init ? SignalValue::ZERO : SignalValue::ONE);
    set_param("master_q", SignalValue::ZERO);
}

std::map<std::string, SignalValue> MasterSlaveRSFF::eval(
    const std::vector<Node>& nodes, const std::string& changed) {
    SignalValue cp = input("CP", nodes);
    SignalValue q = get_param("q", SignalValue::ZERO);

    // Master transparent when CP=1
    if (cp == SignalValue::ONE) {
        SignalValue s = input("S", nodes);
        SignalValue r = input("R", nodes);
        SignalValue mq = SignalValue::X;
        if (s == SignalValue::X || s == SignalValue::Z ||
            r == SignalValue::X || r == SignalValue::Z) {
            mq = SignalValue::X;
        } else if (s == SignalValue::ONE && r == SignalValue::ZERO) {
            mq = SignalValue::ONE;
        } else if (s == SignalValue::ZERO && r == SignalValue::ONE) {
            mq = SignalValue::ZERO;
        } else if (s == SignalValue::ONE && r == SignalValue::ONE) {
            mq = SignalValue::X;
        }
        // else hold: keep master_q
        if (mq != SignalValue::X) {
            set_param("master_q", mq);
        } else if (s == SignalValue::ONE || r == SignalValue::ONE) {
            set_param("master_q", mq);  // X
        }
    }

    // Slave transfers on CP falling edge (1→0)
    if (falling_edge("CP", nodes)) {
        q = get_param("master_q", SignalValue::ZERO);
    }

    SignalValue qn = signal_not(q);
    set_param("q", q);
    set_param("qn", qn);
    return {{"Q", q}, {"QN", qn}};
}

// ============================================================
// MasterSlaveDFF
// ============================================================

MasterSlaveDFF::MasterSlaveDFF(const std::string& id)
    : Device(id, "MASTER_SLAVE_D_FF")
{
    add_pin("D", "D", Pin::INPUT, Pin::ACTIVE_HIGH, "data");
    add_pin("CP", "CP", Pin::INPUT, Pin::ACTIVE_HIGH, "clock");
    add_pin("Q", "Q", Pin::OUTPUT);
    add_pin("QN", "QN", Pin::OUTPUT);
    set_param_int("initial_q", 0);
}

void MasterSlaveDFF::reset() {
    int init = get_param_int("initial_q", 0);
    set_param("q", init ? SignalValue::ONE : SignalValue::ZERO);
    set_param("qn", init ? SignalValue::ZERO : SignalValue::ONE);
    set_param("master_q", SignalValue::ZERO);
}

std::map<std::string, SignalValue> MasterSlaveDFF::eval(
    const std::vector<Node>& nodes, const std::string& changed) {
    SignalValue cp = input("CP", nodes);
    SignalValue q = get_param("q", SignalValue::ZERO);

    // Master transparent when CP=1
    if (cp == SignalValue::ONE) {
        SignalValue d = input("D", nodes);
        set_param("master_q",
                   (d == SignalValue::X || d == SignalValue::Z) ? SignalValue::X : d);
    }

    // Slave transfers on CP falling edge
    if (falling_edge("CP", nodes)) {
        q = get_param("master_q", SignalValue::ZERO);
    }

    SignalValue qn = signal_not(q);
    set_param("q", q);
    set_param("qn", qn);
    return {{"Q", q}, {"QN", qn}};
}

// ============================================================
// MasterSlaveJKFF
// ============================================================

MasterSlaveJKFF::MasterSlaveJKFF(const std::string& id)
    : Device(id, "MASTER_SLAVE_JK_FF")
{
    add_pin("J", "J", Pin::INPUT);
    add_pin("K", "K", Pin::INPUT);
    add_pin("CP", "CP", Pin::INPUT, Pin::ACTIVE_HIGH, "clock");
    add_pin("Q", "Q", Pin::OUTPUT);
    add_pin("QN", "QN", Pin::OUTPUT);
    set_param_int("initial_q", 0);
}

void MasterSlaveJKFF::reset() {
    int init = get_param_int("initial_q", 0);
    set_param("q", init ? SignalValue::ONE : SignalValue::ZERO);
    set_param("qn", init ? SignalValue::ZERO : SignalValue::ONE);
    set_param("master_q", SignalValue::ZERO);
}

std::map<std::string, SignalValue> MasterSlaveJKFF::eval(
    const std::vector<Node>& nodes, const std::string& changed) {
    SignalValue cp = input("CP", nodes);
    SignalValue q = get_param("q", SignalValue::ZERO);

    // Master transparent when CP=1
    if (cp == SignalValue::ONE) {
        SignalValue j = input("J", nodes);
        SignalValue k = input("K", nodes);
        auto [mq, mqn] = jk_eval(j, k, q);
        set_param("master_q", mq);
    }

    // Slave transfers on CP falling edge
    if (falling_edge("CP", nodes)) {
        q = get_param("master_q", SignalValue::ZERO);
    }

    SignalValue qn = signal_not(q);
    set_param("q", q);
    set_param("qn", qn);
    return {{"Q", q}, {"QN", qn}};
}

// ============================================================
// EdgeDFF — edge-triggered D flip-flop with optional async PRESET/CLEAR
// ============================================================

EdgeDFF::EdgeDFF(const std::string& id)
    : Device(id, "EDGE_D_FF")
{
    add_pin("D", "D", Pin::INPUT, Pin::ACTIVE_HIGH, "data");
    add_pin("CP", "CP", Pin::INPUT, Pin::ACTIVE_HIGH, "clock");
    add_pin("PRE", "PRE", Pin::INPUT, Pin::ACTIVE_LOW, "preset");
    add_pin("CLR", "CLR", Pin::INPUT, Pin::ACTIVE_LOW, "clear");
    add_pin("Q", "Q", Pin::OUTPUT);
    add_pin("QN", "QN", Pin::OUTPUT);
    set_param_int("initial_q", 0);
    sig_params_["edge"] = SignalValue::ONE;  // rising
}

void EdgeDFF::reset() {
    int init = get_param_int("initial_q", 0);
    set_param("q", init ? SignalValue::ONE : SignalValue::ZERO);
    set_param("qn", init ? SignalValue::ZERO : SignalValue::ONE);
}

std::map<std::string, SignalValue> EdgeDFF::eval(
    const std::vector<Node>& nodes, const std::string& changed) {
    SignalValue pre = input("PRE", nodes);
    SignalValue clr = input("CLR", nodes);
    SignalValue q = get_param("q", SignalValue::ZERO);

    // Async controls have priority
    if (pre == SignalValue::ZERO && clr == SignalValue::ZERO) {
        q = SignalValue::X;
    } else if (clr == SignalValue::ZERO) {
        q = SignalValue::ZERO;
    } else if (pre == SignalValue::ZERO) {
        q = SignalValue::ONE;
    } else {
        std::string edge_type = trigger_edge();
        bool edge = (edge_type == "falling") ? falling_edge("CP", nodes)
                                              : rising_edge("CP", nodes);
        if (edge) {
            SignalValue d = input("D", nodes);
            q = (d == SignalValue::X || d == SignalValue::Z) ? SignalValue::X : d;
        }
    }

    SignalValue qn = signal_not(q);
    set_param("q", q);
    set_param("qn", qn);
    return {{"Q", q}, {"QN", qn}};
}

// ============================================================
// EdgeJKFF — edge-triggered JK flip-flop with optional async PRESET/CLEAR
// ============================================================

EdgeJKFF::EdgeJKFF(const std::string& id)
    : Device(id, "EDGE_JK_FF")
{
    add_pin("J", "J", Pin::INPUT);
    add_pin("K", "K", Pin::INPUT);
    add_pin("CP", "CP", Pin::INPUT, Pin::ACTIVE_HIGH, "clock");
    add_pin("PRE", "PRE", Pin::INPUT, Pin::ACTIVE_LOW, "preset");
    add_pin("CLR", "CLR", Pin::INPUT, Pin::ACTIVE_LOW, "clear");
    add_pin("Q", "Q", Pin::OUTPUT);
    add_pin("QN", "QN", Pin::OUTPUT);
    set_param_int("initial_q", 0);
    sig_params_["edge"] = SignalValue::ONE;
}

void EdgeJKFF::reset() {
    int init = get_param_int("initial_q", 0);
    set_param("q", init ? SignalValue::ONE : SignalValue::ZERO);
    set_param("qn", init ? SignalValue::ZERO : SignalValue::ONE);
}

std::map<std::string, SignalValue> EdgeJKFF::eval(
    const std::vector<Node>& nodes, const std::string& changed) {
    SignalValue pre = input("PRE", nodes);
    SignalValue clr = input("CLR", nodes);
    SignalValue q = get_param("q", SignalValue::ZERO);

    // Async controls have priority
    if (pre == SignalValue::ZERO && clr == SignalValue::ZERO) {
        q = SignalValue::X;
    } else if (clr == SignalValue::ZERO) {
        q = SignalValue::ZERO;
    } else if (pre == SignalValue::ZERO) {
        q = SignalValue::ONE;
    } else {
        std::string edge_type = trigger_edge();
        bool edge = (edge_type == "falling") ? falling_edge("CP", nodes)
                                              : rising_edge("CP", nodes);
        if (edge) {
            SignalValue j = input("J", nodes);
            SignalValue k = input("K", nodes);
            auto [q_next, qn_next] = jk_eval(j, k, q);
            q = q_next;
        }
    }

    SignalValue qn = signal_not(q);
    set_param("q", q);
    set_param("qn", qn);
    return {{"Q", q}, {"QN", qn}};
}

// ============================================================
// BlockingJKFF — maintenance-blocking JK (behaviorally edge-triggered JK)
// ============================================================

BlockingJKFF::BlockingJKFF(const std::string& id)
    : Device(id, "BLOCKING_JK_FF")
{
    add_pin("J", "J", Pin::INPUT);
    add_pin("K", "K", Pin::INPUT);
    add_pin("CP", "CP", Pin::INPUT, Pin::ACTIVE_HIGH, "clock");
    add_pin("Q", "Q", Pin::OUTPUT);
    add_pin("QN", "QN", Pin::OUTPUT);
    set_param_int("initial_q", 0);
    sig_params_["edge"] = SignalValue::ONE;  // positive edge
}

void BlockingJKFF::reset() {
    int init = get_param_int("initial_q", 0);
    set_param("q", init ? SignalValue::ONE : SignalValue::ZERO);
    set_param("qn", init ? SignalValue::ZERO : SignalValue::ONE);
}

std::map<std::string, SignalValue> BlockingJKFF::eval(
    const std::vector<Node>& nodes, const std::string& changed) {
    // Behaviorally identical to positive-edge-triggered JK
    bool edge = rising_edge("CP", nodes);
    SignalValue q = get_param("q", SignalValue::ZERO);

    if (edge) {
        SignalValue j = input("J", nodes);
        SignalValue k = input("K", nodes);
        auto [q_next, qn_next] = jk_eval(j, k, q);
        q = q_next;
    }

    SignalValue qn = signal_not(q);
    set_param("q", q);
    set_param("qn", qn);
    return {{"Q", q}, {"QN", qn}};
}

// ---- Factory ----

std::unique_ptr<Device> create_flip_flop(const std::string& type,
                                          const std::string& id) {
    if (type == "RS_LATCH_NAND")       return std::make_unique<RSLatchNand>(id);
    if (type == "RS_LATCH_NOR")        return std::make_unique<RSLatchNor>(id);
    if (type == "GATED_RS_LATCH")      return std::make_unique<GatedRSLatch>(id);
    if (type == "D_LATCH")             return std::make_unique<DLatch>(id);
    if (type == "SYNC_RS_FF")          return std::make_unique<SyncRSFF>(id);
    if (type == "SYNC_D_FF")           return std::make_unique<SyncDFF>(id);
    if (type == "SYNC_JK_FF")          return std::make_unique<SyncJKFF>(id);
    if (type == "SYNC_T_FF")           return std::make_unique<SyncTFF>(id);
    if (type == "MASTER_SLAVE_RS_FF")  return std::make_unique<MasterSlaveRSFF>(id);
    if (type == "MASTER_SLAVE_D_FF")   return std::make_unique<MasterSlaveDFF>(id);
    if (type == "MASTER_SLAVE_JK_FF")  return std::make_unique<MasterSlaveJKFF>(id);
    if (type == "EDGE_D_FF")           return std::make_unique<EdgeDFF>(id);
    if (type == "EDGE_JK_FF")          return std::make_unique<EdgeJKFF>(id);
    if (type == "BLOCKING_JK_FF")      return std::make_unique<BlockingJKFF>(id);
    return nullptr;
}

}  // namespace logic_sim
