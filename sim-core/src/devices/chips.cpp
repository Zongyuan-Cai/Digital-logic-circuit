#include "logic_sim/devices/chips.hpp"
#include <algorithm>

namespace logic_sim {

// ============================================================
// 74LS148: 8-to-3 priority encoder
// Inputs (active low): EI, I0-I7
// Outputs (active low): A2,A1,A0, GS, EO
// I7 has highest priority.
// ============================================================

Chip74LS148::Chip74LS148(const std::string& id)
    : Device(id, "74LS148")
{
    add_pin("EI", "EI", Pin::INPUT, Pin::ACTIVE_LOW, "enable");
    for (int i = 0; i <= 7; ++i)
        add_pin("I" + std::to_string(i), "I" + std::to_string(i),
                Pin::INPUT, Pin::ACTIVE_LOW, "data");
    add_pin("A2", "A2", Pin::OUTPUT, Pin::ACTIVE_LOW, "address");
    add_pin("A1", "A1", Pin::OUTPUT, Pin::ACTIVE_LOW, "address");
    add_pin("A0", "A0", Pin::OUTPUT, Pin::ACTIVE_LOW, "address");
    add_pin("GS", "GS", Pin::OUTPUT, Pin::ACTIVE_LOW, "valid");
    add_pin("EO", "EO", Pin::OUTPUT, Pin::ACTIVE_LOW, "cascade");
}

std::map<std::string, SignalValue> Chip74LS148::eval(
    const std::vector<Node>& nodes, const std::string&) {
    SignalValue ei = input("EI", nodes);

    // EI invalid → all outputs inactive
    if (ei == SignalValue::X || ei == SignalValue::Z) {
        // Propagate unknown
        return {{"A2", SignalValue::X}, {"A1", SignalValue::X}, {"A0", SignalValue::X},
                {"GS", SignalValue::X}, {"EO", SignalValue::X}};
    }
    if (ei == SignalValue::ONE) {
        // Disabled
        return {{"A2", SignalValue::ONE}, {"A1", SignalValue::ONE}, {"A0", SignalValue::ONE},
                {"GS", SignalValue::ONE}, {"EO", SignalValue::ONE}};
    }

    // Check inputs from I7 down to I0 (priority). If a higher-priority
    // input is unknown, a lower active input cannot determine the result.
    int selected = -1;
    bool higher_unknown = false;
    for (int i = 7; i >= 0; --i) {
        SignalValue iv = input("I" + std::to_string(i), nodes);
        if (iv == SignalValue::X || iv == SignalValue::Z) {
            higher_unknown = true;
            continue;
        }
        if (iv == SignalValue::ZERO) {
            if (higher_unknown) {
                selected = -2;
                break;
            }
            selected = i;
            break;
        }
    }
    if (selected == -1 && higher_unknown) selected = -2;

    if (selected >= 0) {
        int code = selected;  // encode I0→0, I1→1, ..., I7→7 (inverted output)
        return {{"A2", (code & 4) ? SignalValue::ZERO : SignalValue::ONE},
                {"A1", (code & 2) ? SignalValue::ZERO : SignalValue::ONE},
                {"A0", (code & 1) ? SignalValue::ZERO : SignalValue::ONE},
                {"GS", SignalValue::ZERO},
                {"EO", SignalValue::ONE}};
    } else if (selected == -1) {
        // No input active
        return {{"A2", SignalValue::ONE}, {"A1", SignalValue::ONE}, {"A0", SignalValue::ONE},
                {"GS", SignalValue::ONE}, {"EO", SignalValue::ZERO}};
    } else {
        // Unknown
        return {{"A2", SignalValue::X}, {"A1", SignalValue::X}, {"A0", SignalValue::X},
                {"GS", SignalValue::X}, {"EO", SignalValue::X}};
    }
}

// ============================================================
// 74LS147: BCD priority encoder
// Inputs (active low): I1-I9
// Outputs (active low): D,C,B,A (4-bit BCD)
// I9 has highest priority.  No input → output 0 (1111 inverted).
// ============================================================

Chip74LS147::Chip74LS147(const std::string& id)
    : Device(id, "74LS147")
{
    for (int i = 1; i <= 9; ++i)
        add_pin("I" + std::to_string(i), "I" + std::to_string(i),
                Pin::INPUT, Pin::ACTIVE_LOW, "data");
    add_pin("D", "D", Pin::OUTPUT, Pin::ACTIVE_LOW, "bcd");
    add_pin("C", "C", Pin::OUTPUT, Pin::ACTIVE_LOW, "bcd");
    add_pin("B", "B", Pin::OUTPUT, Pin::ACTIVE_LOW, "bcd");
    add_pin("A", "A", Pin::OUTPUT, Pin::ACTIVE_LOW, "bcd");
}

std::map<std::string, SignalValue> Chip74LS147::eval(
    const std::vector<Node>& nodes, const std::string&) {
    int selected = -1;
    bool higher_unknown = false;
    for (int i = 9; i >= 1; --i) {
        SignalValue iv = input("I" + std::to_string(i), nodes);
        if (iv == SignalValue::X || iv == SignalValue::Z) {
            higher_unknown = true;
            continue;
        }
        if (iv == SignalValue::ZERO) {
            if (higher_unknown) {
                selected = -2;
                break;
            }
            selected = i;
            break;
        }
    }
    if (selected == -1 && higher_unknown) selected = -2;

    int code;
    if (selected == -1) {
        code = 0;  // No input → BCD 0 (1111 inverted)
    } else if (selected == -2) {
        return {{"D", SignalValue::X}, {"C", SignalValue::X},
                {"B", SignalValue::X}, {"A", SignalValue::X}};
    } else {
        code = selected;
    }

    // BCD output is inverted
    return {{"D", (code & 8) ? SignalValue::ZERO : SignalValue::ONE},
            {"C", (code & 4) ? SignalValue::ZERO : SignalValue::ONE},
            {"B", (code & 2) ? SignalValue::ZERO : SignalValue::ONE},
            {"A", (code & 1) ? SignalValue::ZERO : SignalValue::ONE}};
}

// ============================================================
// 74139: Dual 2-to-4 decoder
// Each half has: A0,A1 (address), E (active low enable), Y0-Y3 (active low)
// ============================================================

Chip74139::Chip74139(const std::string& id)
    : Device(id, "74139")
{
    // First decoder
    add_pin("1A0", "1A0", Pin::INPUT, Pin::ACTIVE_HIGH, "address");
    add_pin("1A1", "1A1", Pin::INPUT, Pin::ACTIVE_HIGH, "address");
    add_pin("1E",  "1E",  Pin::INPUT, Pin::ACTIVE_LOW, "enable");
    add_pin("1Y0", "1Y0", Pin::OUTPUT, Pin::ACTIVE_LOW, "data");
    add_pin("1Y1", "1Y1", Pin::OUTPUT, Pin::ACTIVE_LOW, "data");
    add_pin("1Y2", "1Y2", Pin::OUTPUT, Pin::ACTIVE_LOW, "data");
    add_pin("1Y3", "1Y3", Pin::OUTPUT, Pin::ACTIVE_LOW, "data");
    // Second decoder
    add_pin("2A0", "2A0", Pin::INPUT, Pin::ACTIVE_HIGH, "address");
    add_pin("2A1", "2A1", Pin::INPUT, Pin::ACTIVE_HIGH, "address");
    add_pin("2E",  "2E",  Pin::INPUT, Pin::ACTIVE_LOW, "enable");
    add_pin("2Y0", "2Y0", Pin::OUTPUT, Pin::ACTIVE_LOW, "data");
    add_pin("2Y1", "2Y1", Pin::OUTPUT, Pin::ACTIVE_LOW, "data");
    add_pin("2Y2", "2Y2", Pin::OUTPUT, Pin::ACTIVE_LOW, "data");
    add_pin("2Y3", "2Y3", Pin::OUTPUT, Pin::ACTIVE_LOW, "data");
}

static std::map<std::string, SignalValue> decode_2to4(
    const std::string& prefix,
    SignalValue a0, SignalValue a1, SignalValue en,
    const std::vector<Node>& nodes, Device* dev) {
    std::map<std::string, SignalValue> out;

    // Enable off?
    if (en == SignalValue::X || en == SignalValue::Z) {
        for (int i = 0; i < 4; ++i)
            out[prefix + "Y" + std::to_string(i)] = SignalValue::X;
        return out;
    }
    if (en == SignalValue::ONE) {
        for (int i = 0; i < 4; ++i)
            out[prefix + "Y" + std::to_string(i)] = SignalValue::ONE;
        return out;
    }

    // Determine address
    int addr = -1;
    if ((a0 == SignalValue::ZERO || a0 == SignalValue::ONE) &&
        (a1 == SignalValue::ZERO || a1 == SignalValue::ONE)) {
        addr = (a0 == SignalValue::ONE ? 1 : 0) |
               (a1 == SignalValue::ONE ? 2 : 0);
    } else {
        for (int i = 0; i < 4; ++i)
            out[prefix + "Y" + std::to_string(i)] = SignalValue::X;
        return out;
    }

    for (int i = 0; i < 4; ++i)
        out[prefix + "Y" + std::to_string(i)] =
            (i == addr) ? SignalValue::ZERO : SignalValue::ONE;
    return out;
}

std::map<std::string, SignalValue> Chip74139::eval(
    const std::vector<Node>& nodes, const std::string&) {
    auto out = decode_2to4("1",
        input("1A0", nodes), input("1A1", nodes), input("1E", nodes),
        nodes, this);
    auto out2 = decode_2to4("2",
        input("2A0", nodes), input("2A1", nodes), input("2E", nodes),
        nodes, this);
    out.insert(out2.begin(), out2.end());
    return out;
}

// ============================================================
// 74LS138 / 74138: 3-to-8 decoder
// A,B,C (address), G1 (active high enable), G2A,G2B (active low enable)
// Y0-Y7 (active low)
// ============================================================

Chip74LS138::Chip74LS138(const std::string& id)
    : Device(id, "74LS138")
{
    add_pin("A", "A", Pin::INPUT, Pin::ACTIVE_HIGH, "address");
    add_pin("B", "B", Pin::INPUT, Pin::ACTIVE_HIGH, "address");
    add_pin("C", "C", Pin::INPUT, Pin::ACTIVE_HIGH, "address");
    add_pin("G1", "G1", Pin::INPUT, Pin::ACTIVE_HIGH, "enable");
    add_pin("G2A", "G2A", Pin::INPUT, Pin::ACTIVE_LOW, "enable");
    add_pin("G2B", "G2B", Pin::INPUT, Pin::ACTIVE_LOW, "enable");
    for (int i = 0; i < 8; ++i)
        add_pin("Y" + std::to_string(i), "Y" + std::to_string(i),
                Pin::OUTPUT, Pin::ACTIVE_LOW, "data");
}

std::map<std::string, SignalValue> Chip74LS138::eval(
    const std::vector<Node>& nodes, const std::string&) {
    SignalValue g1  = input("G1", nodes);
    SignalValue g2a = input("G2A", nodes);
    SignalValue g2b = input("G2B", nodes);
    SignalValue a   = input("A", nodes);
    SignalValue b   = input("B", nodes);
    SignalValue c   = input("C", nodes);

    // Check enable: G1=1 AND G2A=0 AND G2B=0
    bool enabled = (g1 == SignalValue::ONE) &&
                   (g2a == SignalValue::ZERO) &&
                   (g2b == SignalValue::ZERO);

    bool unknown = (g1 == SignalValue::X || g1 == SignalValue::Z ||
                    g2a == SignalValue::X || g2a == SignalValue::Z ||
                    g2b == SignalValue::X || g2b == SignalValue::Z);

    std::map<std::string, SignalValue> out;
    if (unknown) {
        for (int i = 0; i < 8; ++i)
            out["Y" + std::to_string(i)] = SignalValue::X;
        return out;
    }
    if (!enabled) {
        for (int i = 0; i < 8; ++i)
            out["Y" + std::to_string(i)] = SignalValue::ONE;
        return out;
    }

    // Decode address C,B,A
    int addr = -1;
    if (a == SignalValue::X || a == SignalValue::Z ||
        b == SignalValue::X || b == SignalValue::Z ||
        c == SignalValue::X || c == SignalValue::Z) {
        for (int i = 0; i < 8; ++i)
            out["Y" + std::to_string(i)] = SignalValue::X;
        return out;
    }
    addr = (c == SignalValue::ONE ? 4 : 0) |
           (b == SignalValue::ONE ? 2 : 0) |
           (a == SignalValue::ONE ? 1 : 0);

    for (int i = 0; i < 8; ++i)
        out["Y" + std::to_string(i)] = (i == addr) ? SignalValue::ZERO : SignalValue::ONE;
    return out;
}

// ============================================================
// 74LS42: BCD to decimal decoder
// A,B,C,D (BCD, active high), Y0-Y9 (active low)
// Invalid 10-15 → all inactive (high)
// ============================================================

Chip74LS42::Chip74LS42(const std::string& id)
    : Device(id, "74LS42")
{
    add_pin("A", "A", Pin::INPUT, Pin::ACTIVE_HIGH, "bcd");
    add_pin("B", "B", Pin::INPUT, Pin::ACTIVE_HIGH, "bcd");
    add_pin("C", "C", Pin::INPUT, Pin::ACTIVE_HIGH, "bcd");
    add_pin("D", "D", Pin::INPUT, Pin::ACTIVE_HIGH, "bcd");
    for (int i = 0; i < 10; ++i)
        add_pin("Y" + std::to_string(i), "Y" + std::to_string(i),
                Pin::OUTPUT, Pin::ACTIVE_LOW, "data");
}

std::map<std::string, SignalValue> Chip74LS42::eval(
    const std::vector<Node>& nodes, const std::string&) {
    SignalValue a = input("A", nodes);
    SignalValue b = input("B", nodes);
    SignalValue c = input("C", nodes);
    SignalValue d = input("D", nodes);

    std::map<std::string, SignalValue> out;
    int val = -1;
    if (a == SignalValue::X || a == SignalValue::Z ||
        b == SignalValue::X || b == SignalValue::Z ||
        c == SignalValue::X || c == SignalValue::Z ||
        d == SignalValue::X || d == SignalValue::Z) {
        for (int i = 0; i < 10; ++i)
            out["Y" + std::to_string(i)] = SignalValue::X;
        return out;
    }
    val = (d == SignalValue::ONE ? 8 : 0) |
          (c == SignalValue::ONE ? 4 : 0) |
          (b == SignalValue::ONE ? 2 : 0) |
          (a == SignalValue::ONE ? 1 : 0);

    for (int i = 0; i < 10; ++i)
        out["Y" + std::to_string(i)] = (i == val) ? SignalValue::ZERO : SignalValue::ONE;
    return out;
}

// ============================================================
// 7448: BCD to 7-segment decoder/driver
// Inputs: A,B,C,D (BCD), LT (lamp test, active low),
//         RBI (ripple blanking input, active low),
//         BI/RBO (blanking input/output, bidirectional, active low)
// Outputs: a,b,c,d,e,f,g (active high segments)
// ============================================================

Chip7448::Chip7448(const std::string& id)
    : Device(id, "7448")
{
    add_pin("A", "A", Pin::INPUT, Pin::ACTIVE_HIGH, "bcd");
    add_pin("B", "B", Pin::INPUT, Pin::ACTIVE_HIGH, "bcd");
    add_pin("C", "C", Pin::INPUT, Pin::ACTIVE_HIGH, "bcd");
    add_pin("D", "D", Pin::INPUT, Pin::ACTIVE_HIGH, "bcd");
    add_pin("LT", "LT", Pin::INPUT, Pin::ACTIVE_LOW, "lamp_test");
    add_pin("RBI", "RBI", Pin::INPUT, Pin::ACTIVE_LOW, "ripple_blanking");
    add_pin("BIRBO", "BIRBO", Pin::BIDIRECTIONAL, Pin::ACTIVE_LOW, "blanking");
    add_pin("a", "a", Pin::OUTPUT, Pin::ACTIVE_HIGH, "segment");
    add_pin("b", "b", Pin::OUTPUT, Pin::ACTIVE_HIGH, "segment");
    add_pin("c", "c", Pin::OUTPUT, Pin::ACTIVE_HIGH, "segment");
    add_pin("d", "d", Pin::OUTPUT, Pin::ACTIVE_HIGH, "segment");
    add_pin("e", "e", Pin::OUTPUT, Pin::ACTIVE_HIGH, "segment");
    add_pin("f", "f", Pin::OUTPUT, Pin::ACTIVE_HIGH, "segment");
    add_pin("g", "g", Pin::OUTPUT, Pin::ACTIVE_HIGH, "segment");
}

// 7-segment patterns for digits 0-9 (a=bit0, g=bit6)
// Segment order: a,b,c,d,e,f,g
static const uint8_t SEG_PATTERNS[10] = {
    0b0111111, // 0: a b c d e f
    0b0000110, // 1: b c
    0b1011011, // 2: a b d e g
    0b1001111, // 3: a b c d g
    0b1100110, // 4: b c f g
    0b1101101, // 5: a c d f g
    0b1111101, // 6: a c d e f g
    0b0000111, // 7: a b c
    0b1111111, // 8: a b c d e f g
    0b1101111, // 9: a b c d f g
};

std::map<std::string, SignalValue> Chip7448::eval(
    const std::vector<Node>& nodes, const std::string&) {
    SignalValue lt    = input("LT", nodes);
    SignalValue rbi   = input("RBI", nodes);
    SignalValue birbo = input("BIRBO", nodes);
    SignalValue a_in  = input("A", nodes);
    SignalValue b_in  = input("B", nodes);
    SignalValue c_in  = input("C", nodes);
    SignalValue d_in  = input("D", nodes);

    // BI/RBO as input (blanking): when low, all segments off
    if (birbo == SignalValue::ZERO) {
        std::map<std::string, SignalValue> out;
        for (char seg = 'a'; seg <= 'g'; ++seg)
            out[std::string(1, seg)] = SignalValue::ZERO;
        return out;
    }

    // LT = 0: lamp test — all segments on
    if (lt == SignalValue::ZERO) {
        std::map<std::string, SignalValue> out;
        for (char seg = 'a'; seg <= 'g'; ++seg)
            out[std::string(1, seg)] = SignalValue::ONE;
        return out;
    }

    // Determine BCD digit
    int digit = -1;
    if (a_in == SignalValue::X || a_in == SignalValue::Z ||
        b_in == SignalValue::X || b_in == SignalValue::Z ||
        c_in == SignalValue::X || c_in == SignalValue::Z ||
        d_in == SignalValue::X || d_in == SignalValue::Z) {
        digit = -1;
    } else {
        digit = (d_in == SignalValue::ONE ? 8 : 0) |
                (c_in == SignalValue::ONE ? 4 : 0) |
                (b_in == SignalValue::ONE ? 2 : 0) |
                (a_in == SignalValue::ONE ? 1 : 0);
        if (digit > 9) digit = -1;  // invalid BCD
    }

    // RBI = 0 and digit = 0: blank (all segments off)
    if (rbi == SignalValue::ZERO && digit == 0) {
        std::map<std::string, SignalValue> out;
        for (char seg = 'a'; seg <= 'g'; ++seg)
            out[std::string(1, seg)] = SignalValue::ZERO;
        return out;
    }

    std::map<std::string, SignalValue> out;
    if (digit < 0) {
        for (char seg = 'a'; seg <= 'g'; ++seg)
            out[std::string(1, seg)] = SignalValue::X;
    } else {
        uint8_t pat = SEG_PATTERNS[digit];
        const char segs[] = {'a','b','c','d','e','f','g'};
        for (int i = 0; i < 7; ++i)
            out[std::string(1, segs[i])] =
                (pat & (1 << i)) ? SignalValue::ONE : SignalValue::ZERO;
    }
    return out;
}

// ============================================================
// 74153 / 74HC153: Dual 4-to-1 data selector
// Two muxes: each has C0-C3 (data), G (enable, active low), Y (output)
// Common select: A, B
// ============================================================

Chip74153::Chip74153(const std::string& id)
    : Device(id, "74153")
{
    add_pin("A", "A", Pin::INPUT, Pin::ACTIVE_HIGH, "select");
    add_pin("B", "B", Pin::INPUT, Pin::ACTIVE_HIGH, "select");
    add_pin("1G", "1G", Pin::INPUT, Pin::ACTIVE_LOW, "enable");
    add_pin("1C0","1C0",Pin::INPUT, Pin::ACTIVE_HIGH, "data");
    add_pin("1C1","1C1",Pin::INPUT, Pin::ACTIVE_HIGH, "data");
    add_pin("1C2","1C2",Pin::INPUT, Pin::ACTIVE_HIGH, "data");
    add_pin("1C3","1C3",Pin::INPUT, Pin::ACTIVE_HIGH, "data");
    add_pin("1Y", "1Y", Pin::OUTPUT);
    add_pin("2G", "2G", Pin::INPUT, Pin::ACTIVE_LOW, "enable");
    add_pin("2C0","2C0",Pin::INPUT, Pin::ACTIVE_HIGH, "data");
    add_pin("2C1","2C1",Pin::INPUT, Pin::ACTIVE_HIGH, "data");
    add_pin("2C2","2C2",Pin::INPUT, Pin::ACTIVE_HIGH, "data");
    add_pin("2C3","2C3",Pin::INPUT, Pin::ACTIVE_HIGH, "data");
    add_pin("2Y", "2Y", Pin::OUTPUT);
}

static SignalValue mux_4to1(SignalValue a, SignalValue b, SignalValue g,
                             SignalValue c0, SignalValue c1,
                             SignalValue c2, SignalValue c3) {
    if (g == SignalValue::X || g == SignalValue::Z) return SignalValue::X;
    if (g == SignalValue::ONE) return SignalValue::ZERO;  // disabled

    int sel = -1;
    if (a == SignalValue::X || a == SignalValue::Z ||
        b == SignalValue::X || b == SignalValue::Z) return SignalValue::X;
    sel = (b == SignalValue::ONE ? 2 : 0) |
          (a == SignalValue::ONE ? 1 : 0);

    SignalValue data[] = {c0, c1, c2, c3};
    return data[sel];
}

std::map<std::string, SignalValue> Chip74153::eval(
    const std::vector<Node>& nodes, const std::string&) {
    SignalValue a = input("A", nodes);
    SignalValue b = input("B", nodes);
    return {
        {"1Y", mux_4to1(a, b, input("1G", nodes),
                         input("1C0", nodes), input("1C1", nodes),
                         input("1C2", nodes), input("1C3", nodes))},
        {"2Y", mux_4to1(a, b, input("2G", nodes),
                         input("2C0", nodes), input("2C1", nodes),
                         input("2C2", nodes), input("2C3", nodes))}
    };
}

// ============================================================
// 74LS151 / 74151: 8-to-1 data selector
// D0-D7 (data), A,B,C (select), G (active low strobe)
// Y (active high), W (active low complement)
// ============================================================

Chip74LS151::Chip74LS151(const std::string& id)
    : Device(id, "74LS151")
{
    add_pin("A", "A", Pin::INPUT, Pin::ACTIVE_HIGH, "select");
    add_pin("B", "B", Pin::INPUT, Pin::ACTIVE_HIGH, "select");
    add_pin("C", "C", Pin::INPUT, Pin::ACTIVE_HIGH, "select");
    add_pin("G", "G", Pin::INPUT, Pin::ACTIVE_LOW, "strobe");
    for (int i = 0; i < 8; ++i)
        add_pin("D" + std::to_string(i), "D" + std::to_string(i),
                Pin::INPUT, Pin::ACTIVE_HIGH, "data");
    add_pin("Y", "Y", Pin::OUTPUT, Pin::ACTIVE_HIGH, "output");
    add_pin("W", "W", Pin::OUTPUT, Pin::ACTIVE_LOW, "output_n");
}

std::map<std::string, SignalValue> Chip74LS151::eval(
    const std::vector<Node>& nodes, const std::string&) {
    SignalValue g = input("G", nodes);
    SignalValue a = input("A", nodes);
    SignalValue b = input("B", nodes);
    SignalValue c = input("C", nodes);

    if (g == SignalValue::X || g == SignalValue::Z) {
        return {{"Y", SignalValue::X}, {"W", SignalValue::X}};
    }
    if (g == SignalValue::ONE) {
        return {{"Y", SignalValue::ZERO}, {"W", SignalValue::ONE}};
    }

    int sel = -1;
    if (a == SignalValue::X || a == SignalValue::Z ||
        b == SignalValue::X || b == SignalValue::Z ||
        c == SignalValue::X || c == SignalValue::Z) {
        return {{"Y", SignalValue::X}, {"W", SignalValue::X}};
    }
    sel = (c == SignalValue::ONE ? 4 : 0) |
          (b == SignalValue::ONE ? 2 : 0) |
          (a == SignalValue::ONE ? 1 : 0);

    SignalValue y = input("D" + std::to_string(sel), nodes);
    // If the selected data is Z or X, output X
    if (y == SignalValue::Z) y = SignalValue::X;
    SignalValue w = signal_not(y);
    return {{"Y", y}, {"W", w}};
}

// ============================================================
// 7485: 4-bit magnitude comparator
// A0-A3, B0-B3, cascade inputs (A>B, A=B, A<B)
// Outputs: A>B, A=B, A<B
// ============================================================

Chip7485::Chip7485(const std::string& id)
    : Device(id, "7485")
{
    for (int i = 0; i < 4; ++i) {
        add_pin("A" + std::to_string(i), "A" + std::to_string(i),
                Pin::INPUT, Pin::ACTIVE_HIGH, "data");
        add_pin("B" + std::to_string(i), "B" + std::to_string(i),
                Pin::INPUT, Pin::ACTIVE_HIGH, "data");
    }
    add_pin("AGTB", "AGTB", Pin::INPUT, Pin::ACTIVE_HIGH, "cascade_gt");
    add_pin("AEQB", "AEQB", Pin::INPUT, Pin::ACTIVE_HIGH, "cascade_eq");
    add_pin("ALTB", "ALTB", Pin::INPUT, Pin::ACTIVE_HIGH, "cascade_lt");
    add_pin("AGTBO", "AGTBO", Pin::OUTPUT, Pin::ACTIVE_HIGH, "gt");
    add_pin("AEQBO", "AEQBO", Pin::OUTPUT, Pin::ACTIVE_HIGH, "eq");
    add_pin("ALTBO", "ALTBO", Pin::OUTPUT, Pin::ACTIVE_HIGH, "lt");
}

std::map<std::string, SignalValue> Chip7485::eval(
    const std::vector<Node>& nodes, const std::string&) {
    // Read 4-bit values
    uint32_t a_val = 0, b_val = 0;
    bool a_ok = true, b_ok = true;
    for (int i = 0; i < 4; ++i) {
        SignalValue av = input("A" + std::to_string(i), nodes);
        SignalValue bv = input("B" + std::to_string(i), nodes);
        if (av == SignalValue::X || av == SignalValue::Z) a_ok = false;
        else if (av == SignalValue::ONE) a_val |= (1u << i);
        if (bv == SignalValue::X || bv == SignalValue::Z) b_ok = false;
        else if (bv == SignalValue::ONE) b_val |= (1u << i);
    }

    SignalValue agtb_i = input("AGTB", nodes);
    SignalValue aeqb_i = input("AEQB", nodes);
    SignalValue altb_i = input("ALTB", nodes);

    if (!a_ok || !b_ok) {
        return {{"AGTBO", SignalValue::X}, {"AEQBO", SignalValue::X}, {"ALTBO", SignalValue::X}};
    }

    // Compare MSB first
    if (a_val > b_val) {
        return {{"AGTBO", SignalValue::ONE}, {"AEQBO", SignalValue::ZERO}, {"ALTBO", SignalValue::ZERO}};
    } else if (a_val < b_val) {
        return {{"AGTBO", SignalValue::ZERO}, {"AEQBO", SignalValue::ZERO}, {"ALTBO", SignalValue::ONE}};
    } else {
        // Equal: pass cascade inputs
        SignalValue gt = (agtb_i == SignalValue::X || agtb_i == SignalValue::Z) ? SignalValue::X : agtb_i;
        SignalValue eq = (aeqb_i == SignalValue::X || aeqb_i == SignalValue::Z) ? SignalValue::X : aeqb_i;
        SignalValue lt = (altb_i == SignalValue::X || altb_i == SignalValue::Z) ? SignalValue::X : altb_i;
        return {{"AGTBO", gt}, {"AEQBO", eq}, {"ALTBO", lt}};
    }
}

// ============================================================
// 74280: 9-bit parity generator/checker
// A-I (9 inputs), EVEN, ODD outputs
// ============================================================

Chip74280::Chip74280(const std::string& id)
    : Device(id, "74280")
{
    for (char c = 'A'; c <= 'I'; ++c)
        add_pin(std::string(1, c), std::string(1, c), Pin::INPUT, Pin::ACTIVE_HIGH, "data");
    add_pin("EVEN", "EVEN", Pin::OUTPUT, Pin::ACTIVE_HIGH, "even");
    add_pin("ODD",  "ODD",  Pin::OUTPUT, Pin::ACTIVE_HIGH, "odd");
}

std::map<std::string, SignalValue> Chip74280::eval(
    const std::vector<Node>& nodes, const std::string&) {
    int ones = 0;
    bool unknown = false;
    for (char c = 'A'; c <= 'I'; ++c) {
        SignalValue v = input(std::string(1, c), nodes);
        if (v == SignalValue::ONE) ++ones;
        else if (v == SignalValue::X || v == SignalValue::Z) unknown = true;
    }

    if (unknown) return {{"EVEN", SignalValue::X}, {"ODD", SignalValue::X}};
    bool even = (ones % 2 == 0);
    return {{"EVEN", even ? SignalValue::ONE : SignalValue::ZERO},
            {"ODD",  even ? SignalValue::ZERO : SignalValue::ONE}};
}

// ============================================================
// 74283: 4-bit binary full adder
// A0-A3, B0-B3, C0 (carry in) → S0-S3, C4 (carry out)
// ============================================================

Chip74283::Chip74283(const std::string& id)
    : Device(id, "74283")
{
    for (int i = 0; i < 4; ++i) {
        add_pin("A" + std::to_string(i), "A" + std::to_string(i),
                Pin::INPUT, Pin::ACTIVE_HIGH, "data");
        add_pin("B" + std::to_string(i), "B" + std::to_string(i),
                Pin::INPUT, Pin::ACTIVE_HIGH, "data");
        add_pin("S" + std::to_string(i), "S" + std::to_string(i),
                Pin::OUTPUT, Pin::ACTIVE_HIGH, "sum");
    }
    add_pin("C0", "C0", Pin::INPUT, Pin::ACTIVE_HIGH, "carry_in");
    add_pin("C4", "C4", Pin::OUTPUT, Pin::ACTIVE_HIGH, "carry_out");
}

std::map<std::string, SignalValue> Chip74283::eval(
    const std::vector<Node>& nodes, const std::string&) {
    uint32_t a_val = 0, b_val = 0;
    SignalValue c0 = input("C0", nodes);
    bool a_ok = true, b_ok = true, c0_ok = true;

    for (int i = 0; i < 4; ++i) {
        SignalValue av = input("A" + std::to_string(i), nodes);
        SignalValue bv = input("B" + std::to_string(i), nodes);
        if (av == SignalValue::X || av == SignalValue::Z) a_ok = false;
        else if (av == SignalValue::ONE) a_val |= (1u << i);
        if (bv == SignalValue::X || bv == SignalValue::Z) b_ok = false;
        else if (bv == SignalValue::ONE) b_val |= (1u << i);
    }
    if (c0 == SignalValue::X || c0 == SignalValue::Z) c0_ok = false;

    std::map<std::string, SignalValue> out;
    if (!a_ok || !b_ok || !c0_ok) {
        for (int i = 0; i < 4; ++i)
            out["S" + std::to_string(i)] = SignalValue::X;
        out["C4"] = SignalValue::X;
        return out;
    }

    uint32_t sum = a_val + b_val + (c0 == SignalValue::ONE ? 1 : 0);
    for (int i = 0; i < 4; ++i)
        out["S" + std::to_string(i)] = (sum & (1u << i)) ? SignalValue::ONE : SignalValue::ZERO;
    out["C4"] = (sum & 0x10) ? SignalValue::ONE : SignalValue::ZERO;
    return out;
}

// ============================================================
// 74175: 4-bit register with async clear
// CLR (active low async clear), CP, D0-D3 → Q0-Q3, Q0N-Q3N
// ============================================================

Chip74175::Chip74175(const std::string& id)
    : Device(id, "74175")
{
    add_pin("CLR", "CLR", Pin::INPUT, Pin::ACTIVE_LOW, "clear");
    add_pin("CP", "CP", Pin::INPUT, Pin::ACTIVE_HIGH, "clock");
    for (int i = 0; i < 4; ++i)
        add_pin("D" + std::to_string(i), "D" + std::to_string(i),
                Pin::INPUT, Pin::ACTIVE_HIGH, "data");
    for (int i = 0; i < 4; ++i) {
        add_pin("Q" + std::to_string(i), "Q" + std::to_string(i),
                Pin::OUTPUT, Pin::ACTIVE_HIGH, "data");
        add_pin("Q" + std::to_string(i) + "N", "Q" + std::to_string(i) + "N",
                Pin::OUTPUT, Pin::ACTIVE_HIGH, "data_n");
    }
    set_param_int("initial", 0);
}

void Chip74175::reset() {
    int init = get_param_int("initial", 0);
    for (int i = 0; i < 4; ++i) {
        set_param("q" + std::to_string(i),
                   (init & (1 << i)) ? SignalValue::ONE : SignalValue::ZERO);
    }
}

std::map<std::string, SignalValue> Chip74175::eval(
    const std::vector<Node>& nodes, const std::string&) {
    SignalValue clr = input("CLR", nodes);
    std::map<std::string, SignalValue> out;

    if (clr == SignalValue::ZERO) {
        // Async clear
        for (int i = 0; i < 4; ++i) {
            set_param("q" + std::to_string(i), SignalValue::ZERO);
            out["Q" + std::to_string(i)] = SignalValue::ZERO;
            out["Q" + std::to_string(i) + "N"] = SignalValue::ONE;
        }
        return out;
    }

    // Rising edge of CP
    if (rising_edge("CP", nodes)) {
        for (int i = 0; i < 4; ++i) {
            SignalValue d = input("D" + std::to_string(i), nodes);
            SignalValue q = (d == SignalValue::X || d == SignalValue::Z)
                            ? SignalValue::X : d;
            set_param("q" + std::to_string(i), q);
        }
    }

    for (int i = 0; i < 4; ++i) {
        SignalValue q = get_param("q" + std::to_string(i), SignalValue::ZERO);
        out["Q" + std::to_string(i)] = q;
        out["Q" + std::to_string(i) + "N"] = signal_not(q);
    }
    return out;
}

// ============================================================
// 74LS195 / 74195: 4-bit right-shift register
// CLR (async), CP, J, K (serial, determine first bit),
// D0-D3 (parallel), SH/LD (shift/load: low=load, high=shift)
// Q0-Q3, Q3N
// ============================================================

Chip74LS195::Chip74LS195(const std::string& id)
    : Device(id, "74LS195")
{
    add_pin("CLR", "CLR", Pin::INPUT, Pin::ACTIVE_LOW, "clear");
    add_pin("CP", "CP", Pin::INPUT, Pin::ACTIVE_HIGH, "clock");
    add_pin("J", "J", Pin::INPUT, Pin::ACTIVE_HIGH, "serial_j");
    add_pin("K", "K", Pin::INPUT, Pin::ACTIVE_HIGH, "serial_k");
    add_pin("SHLD", "SHLD", Pin::INPUT, Pin::ACTIVE_LOW, "shift_load_n");
    for (int i = 0; i < 4; ++i)
        add_pin("D" + std::to_string(i), "D" + std::to_string(i),
                Pin::INPUT, Pin::ACTIVE_HIGH, "data");
    for (int i = 0; i < 4; ++i)
        add_pin("Q" + std::to_string(i), "Q" + std::to_string(i),
                Pin::OUTPUT, Pin::ACTIVE_HIGH, "data");
    add_pin("Q3N", "Q3N", Pin::OUTPUT, Pin::ACTIVE_HIGH, "data_n");
    set_param_int("initial", 0);
}

void Chip74LS195::reset() {
    int init = get_param_int("initial", 0);
    for (int i = 0; i < 4; ++i)
        set_param("q" + std::to_string(i),
                   (init & (1 << i)) ? SignalValue::ONE : SignalValue::ZERO);
}

std::map<std::string, SignalValue> Chip74LS195::eval(
    const std::vector<Node>& nodes, const std::string&) {
    SignalValue clr  = input("CLR", nodes);
    std::map<std::string, SignalValue> out;

    if (clr == SignalValue::ZERO) {
        for (int i = 0; i < 4; ++i) {
            set_param("q" + std::to_string(i), SignalValue::ZERO);
            out["Q" + std::to_string(i)] = SignalValue::ZERO;
        }
        out["Q3N"] = SignalValue::ONE;
        return out;
    }

    if (rising_edge("CP", nodes)) {
        SignalValue shld = input("SHLD", nodes);
        if (shld == SignalValue::ZERO) {
            // Parallel load
            for (int i = 0; i < 4; ++i) {
                SignalValue d = input("D" + std::to_string(i), nodes);
                set_param("q" + std::to_string(i),
                           (d == SignalValue::X || d == SignalValue::Z)
                           ? SignalValue::X : d);
            }
        } else {
            // Shift right
            SignalValue j = input("J", nodes);
            SignalValue k = input("K", nodes);
            SignalValue q0 = get_param("q0", SignalValue::ZERO);

            // First bit: JK function
            SignalValue q0_next;
            if (j == SignalValue::ZERO && k == SignalValue::ZERO) {
                q0_next = SignalValue::ZERO;
            } else if (j == SignalValue::ZERO && k == SignalValue::ONE) {
                q0_next = q0;  // hold
            } else if (j == SignalValue::ONE && k == SignalValue::ZERO) {
                q0_next = signal_not(q0);  // toggle
            } else if (j == SignalValue::ONE && k == SignalValue::ONE) {
                q0_next = SignalValue::ONE;
            } else {
                q0_next = SignalValue::X;
            }

            // Shift: Q3←Q2, Q2←Q1, Q1←Q0, Q0←JK_result
            set_param("q3", get_param("q2", SignalValue::ZERO));
            set_param("q2", get_param("q1", SignalValue::ZERO));
            set_param("q1", q0);
            set_param("q0", q0_next);
        }
    }

    for (int i = 0; i < 4; ++i)
        out["Q" + std::to_string(i)] = get_param("q" + std::to_string(i), SignalValue::ZERO);
    out["Q3N"] = signal_not(get_param("q3", SignalValue::ZERO));
    return out;
}

// ============================================================
// 74LS194 / 74194: 4-bit bidirectional shift register
// CR (async clear), CP, S0,S1 (mode), DSR (serial right), DSL (serial left),
// D0-D3 (parallel) → Q0-Q3
// S1S0=00 hold, 01 shift right, 10 shift left, 11 parallel load
// ============================================================

Chip74LS194::Chip74LS194(const std::string& id)
    : Device(id, "74LS194")
{
    add_pin("CR", "CR", Pin::INPUT, Pin::ACTIVE_LOW, "clear");
    add_pin("CP", "CP", Pin::INPUT, Pin::ACTIVE_HIGH, "clock");
    add_pin("S0", "S0", Pin::INPUT, Pin::ACTIVE_HIGH, "mode");
    add_pin("S1", "S1", Pin::INPUT, Pin::ACTIVE_HIGH, "mode");
    add_pin("DSR", "DSR", Pin::INPUT, Pin::ACTIVE_HIGH, "serial_right");
    add_pin("DSL", "DSL", Pin::INPUT, Pin::ACTIVE_HIGH, "serial_left");
    for (int i = 0; i < 4; ++i)
        add_pin("D" + std::to_string(i), "D" + std::to_string(i),
                Pin::INPUT, Pin::ACTIVE_HIGH, "data");
    for (int i = 0; i < 4; ++i)
        add_pin("Q" + std::to_string(i), "Q" + std::to_string(i),
                Pin::OUTPUT, Pin::ACTIVE_HIGH, "data");
    set_param_int("initial", 0);
}

void Chip74LS194::reset() {
    int init = get_param_int("initial", 0);
    for (int i = 0; i < 4; ++i)
        set_param("q" + std::to_string(i),
                   (init & (1 << i)) ? SignalValue::ONE : SignalValue::ZERO);
}

std::map<std::string, SignalValue> Chip74LS194::eval(
    const std::vector<Node>& nodes, const std::string&) {
    SignalValue cr = input("CR", nodes);
    std::map<std::string, SignalValue> out;

    if (cr == SignalValue::ZERO) {
        for (int i = 0; i < 4; ++i) {
            set_param("q" + std::to_string(i), SignalValue::ZERO);
            out["Q" + std::to_string(i)] = SignalValue::ZERO;
        }
        return out;
    }

    if (rising_edge("CP", nodes)) {
        SignalValue s0 = input("S0", nodes);
        SignalValue s1 = input("S1", nodes);

        int mode = -1;
        if (s1 == SignalValue::ONE && s0 == SignalValue::ONE) mode = 3;     // parallel load
        else if (s1 == SignalValue::ZERO && s0 == SignalValue::ONE) mode = 1;  // shift right
        else if (s1 == SignalValue::ONE && s0 == SignalValue::ZERO) mode = 2;  // shift left
        else if (s1 == SignalValue::ZERO && s0 == SignalValue::ZERO) mode = 0; // hold

        if (mode == 3) {
            // Parallel load
            for (int i = 0; i < 4; ++i) {
                SignalValue d = input("D" + std::to_string(i), nodes);
                set_param("q" + std::to_string(i),
                           (d == SignalValue::X || d == SignalValue::Z)
                           ? SignalValue::X : d);
            }
        } else if (mode == 1) {
            // Shift right: Q3←DSR, Q2←Q3, Q1←Q2, Q0←Q1
            SignalValue dsr = input("DSR", nodes);
            SignalValue q3 = get_param("q3", SignalValue::ZERO);
            SignalValue q2 = get_param("q2", SignalValue::ZERO);
            SignalValue q1 = get_param("q1", SignalValue::ZERO);
            set_param("q3", (dsr == SignalValue::Z) ? SignalValue::X : dsr);
            set_param("q2", q3);
            set_param("q1", q2);
            set_param("q0", q1);
        } else if (mode == 2) {
            // Shift left: Q0←DSL, Q1←Q0, Q2←Q1, Q3←Q2
            SignalValue dsl = input("DSL", nodes);
            SignalValue q2 = get_param("q2", SignalValue::ZERO);
            SignalValue q1 = get_param("q1", SignalValue::ZERO);
            SignalValue q0 = get_param("q0", SignalValue::ZERO);
            set_param("q0", (dsl == SignalValue::Z) ? SignalValue::X : dsl);
            set_param("q1", q0);
            set_param("q2", q1);
            set_param("q3", q2);
        }
        // mode 0: hold
    }

    for (int i = 0; i < 4; ++i)
        out["Q" + std::to_string(i)] = get_param("q" + std::to_string(i), SignalValue::ZERO);
    return out;
}

// ============================================================
// 74161 / 74LS161: 4-bit sync binary counter, async clear
// CR (async clear), CP, LD (sync load), EP, ET, D0-D3 → Q0-Q3, CO
// ============================================================

Chip74161::Chip74161(const std::string& id)
    : Device(id, "74161")
{
    add_pin("CR", "CR", Pin::INPUT, Pin::ACTIVE_LOW, "clear");
    add_pin("CP", "CP", Pin::INPUT, Pin::ACTIVE_HIGH, "clock");
    add_pin("LD", "LD", Pin::INPUT, Pin::ACTIVE_LOW, "load");
    add_pin("EP", "EP", Pin::INPUT, Pin::ACTIVE_HIGH, "count_enable");
    add_pin("ET", "ET", Pin::INPUT, Pin::ACTIVE_HIGH, "count_enable");
    for (int i = 0; i < 4; ++i)
        add_pin("D" + std::to_string(i), "D" + std::to_string(i),
                Pin::INPUT, Pin::ACTIVE_HIGH, "data");
    for (int i = 0; i < 4; ++i)
        add_pin("Q" + std::to_string(i), "Q" + std::to_string(i),
                Pin::OUTPUT, Pin::ACTIVE_HIGH, "data");
    add_pin("CO", "CO", Pin::OUTPUT, Pin::ACTIVE_HIGH, "carry_out");
    set_param_int("initial", 0);
}

void Chip74161::reset() {
    int init = get_param_int("initial", 0);
    set_param_int("count", init);
}

std::map<std::string, SignalValue> Chip74161::eval(
    const std::vector<Node>& nodes, const std::string&) {
    SignalValue cr = input("CR", nodes);
    std::map<std::string, SignalValue> out;
    int count = get_param_int("count", 0);

    // Async clear
    if (cr == SignalValue::ZERO) {
        count = 0;
        set_param_int("count", count);
    } else if (rising_edge("CP", nodes)) {
        SignalValue ld = input("LD", nodes);
        SignalValue ep = input("EP", nodes);
        SignalValue et = input("ET", nodes);

        if (ld == SignalValue::ZERO) {
            // Sync load
            uint32_t d_val = 0;
            bool d_ok = true;
            for (int i = 0; i < 4; ++i) {
                SignalValue d = input("D" + std::to_string(i), nodes);
                if (d == SignalValue::ONE) d_val |= (1u << i);
                else if (d != SignalValue::ZERO) d_ok = false;
            }
            count = d_ok ? static_cast<int>(d_val) : -1;
        } else if (ep == SignalValue::ONE && et == SignalValue::ONE) {
            // Count up
            if (count >= 0 && count < 15) count++;
            else if (count == 15) count = 0;
        }
        // else hold
        set_param_int("count", count);
    }

    // Output Q values
    for (int i = 0; i < 4; ++i) {
        if (count < 0) {
            out["Q" + std::to_string(i)] = SignalValue::X;
        } else {
            out["Q" + std::to_string(i)] = (count & (1 << i)) ? SignalValue::ONE : SignalValue::ZERO;
        }
    }

    // CO = ET & Q3 & Q2 & Q1 & Q0
    SignalValue et = input("ET", nodes);
    bool all_ones = (count == 15);
    out["CO"] = (et == SignalValue::ONE && all_ones) ? SignalValue::ONE : SignalValue::ZERO;

    return out;
}

// ============================================================
// 74163: 4-bit sync binary counter, sync clear
// Same as 74161 but CR is synchronous
// ============================================================

Chip74163::Chip74163(const std::string& id)
    : Device(id, "74163")
{
    add_pin("CR", "CR", Pin::INPUT, Pin::ACTIVE_LOW, "clear");
    add_pin("CP", "CP", Pin::INPUT, Pin::ACTIVE_HIGH, "clock");
    add_pin("LD", "LD", Pin::INPUT, Pin::ACTIVE_LOW, "load");
    add_pin("EP", "EP", Pin::INPUT, Pin::ACTIVE_HIGH, "count_enable");
    add_pin("ET", "ET", Pin::INPUT, Pin::ACTIVE_HIGH, "count_enable");
    for (int i = 0; i < 4; ++i)
        add_pin("D" + std::to_string(i), "D" + std::to_string(i),
                Pin::INPUT, Pin::ACTIVE_HIGH, "data");
    for (int i = 0; i < 4; ++i)
        add_pin("Q" + std::to_string(i), "Q" + std::to_string(i),
                Pin::OUTPUT, Pin::ACTIVE_HIGH, "data");
    add_pin("CO", "CO", Pin::OUTPUT, Pin::ACTIVE_HIGH, "carry_out");
    set_param_int("initial", 0);
}

void Chip74163::reset() {
    int init = get_param_int("initial", 0);
    set_param_int("count", init);
}

std::map<std::string, SignalValue> Chip74163::eval(
    const std::vector<Node>& nodes, const std::string&) {
    std::map<std::string, SignalValue> out;
    int count = get_param_int("count", 0);

    if (rising_edge("CP", nodes)) {
        SignalValue cr = input("CR", nodes);
        SignalValue ld = input("LD", nodes);
        SignalValue ep = input("EP", nodes);
        SignalValue et = input("ET", nodes);

        // Sync clear (priority over load)
        if (cr == SignalValue::ZERO) {
            count = 0;
        } else if (ld == SignalValue::ZERO) {
            uint32_t d_val = 0;
            bool d_ok = true;
            for (int i = 0; i < 4; ++i) {
                SignalValue d = input("D" + std::to_string(i), nodes);
                if (d == SignalValue::ONE) d_val |= (1u << i);
                else if (d != SignalValue::ZERO) d_ok = false;
            }
            count = d_ok ? static_cast<int>(d_val) : -1;
        } else if (ep == SignalValue::ONE && et == SignalValue::ONE) {
            if (count >= 0 && count < 15) count++;
            else if (count == 15) count = 0;
        }
        set_param_int("count", count);
    }

    for (int i = 0; i < 4; ++i) {
        out["Q" + std::to_string(i)] = (count < 0) ? SignalValue::X :
            ((count & (1 << i)) ? SignalValue::ONE : SignalValue::ZERO);
    }
    SignalValue et = input("ET", nodes);
    out["CO"] = (et == SignalValue::ONE && count == 15) ? SignalValue::ONE : SignalValue::ZERO;
    return out;
}

// ============================================================
// 74191: 4-bit up/down counter with async load
// CTEN (active low count enable), D/U (0=up,1=down),
// LD (async load, active low), CP, D0-D3 → Q0-Q3, MAX_MIN, RCO
// ============================================================

Chip74191::Chip74191(const std::string& id)
    : Device(id, "74191")
{
    add_pin("CTEN", "CTEN", Pin::INPUT, Pin::ACTIVE_LOW, "count_enable");
    add_pin("DU", "DU", Pin::INPUT, Pin::ACTIVE_HIGH, "up_down");
    add_pin("LD", "LD", Pin::INPUT, Pin::ACTIVE_LOW, "load");
    add_pin("CP", "CP", Pin::INPUT, Pin::ACTIVE_HIGH, "clock");
    for (int i = 0; i < 4; ++i)
        add_pin("D" + std::to_string(i), "D" + std::to_string(i),
                Pin::INPUT, Pin::ACTIVE_HIGH, "data");
    for (int i = 0; i < 4; ++i)
        add_pin("Q" + std::to_string(i), "Q" + std::to_string(i),
                Pin::OUTPUT, Pin::ACTIVE_HIGH, "data");
    add_pin("MAXMIN", "MAXMIN", Pin::OUTPUT, Pin::ACTIVE_HIGH, "terminal_count");
    add_pin("RCO", "RCO", Pin::OUTPUT, Pin::ACTIVE_LOW, "ripple_clock");
    set_param_int("initial", 0);
}

void Chip74191::reset() {
    int init = get_param_int("initial", 0);
    set_param_int("count", init);
}

std::map<std::string, SignalValue> Chip74191::eval(
    const std::vector<Node>& nodes, const std::string&) {
    SignalValue ld   = input("LD", nodes);
    SignalValue cten = input("CTEN", nodes);
    SignalValue du   = input("DU", nodes);
    SignalValue cp   = input("CP", nodes);

    std::map<std::string, SignalValue> out;
    int count = get_param_int("count", 0);

    // Async load
    if (ld == SignalValue::ZERO) {
        uint32_t d_val = 0;
        bool d_ok = true;
        for (int i = 0; i < 4; ++i) {
            SignalValue d = input("D" + std::to_string(i), nodes);
            if (d == SignalValue::ONE) d_val |= (1u << i);
            else if (d != SignalValue::ZERO) d_ok = false;
        }
        count = d_ok ? static_cast<int>(d_val) : -1;
        set_param_int("count", count);
    } else if (rising_edge("CP", nodes) && cten == SignalValue::ZERO) {
        if (du == SignalValue::ZERO) {
            // Count up
            if (count >= 0 && count < 15) count++;
            else if (count == 15) count = 0;
        } else {
            // Count down
            if (count > 0 && count <= 15) count--;
            else if (count == 0) count = 15;
        }
        set_param_int("count", count);
    }

    for (int i = 0; i < 4; ++i) {
        out["Q" + std::to_string(i)] = (count < 0) ? SignalValue::X :
            ((count & (1 << i)) ? SignalValue::ONE : SignalValue::ZERO);
    }

    // MAX_MIN: high when terminal count (15 for up, 0 for down)
    bool terminal = (du == SignalValue::ZERO && count == 15) ||
                    (du == SignalValue::ONE && count == 0);
    out["MAXMIN"] = terminal ? SignalValue::ONE : SignalValue::ZERO;

    // RCO: low when MAXMIN=1 AND CP=0 (for cascading)
    bool rco_active = terminal && (cp == SignalValue::ZERO);
    out["RCO"] = rco_active ? SignalValue::ZERO : SignalValue::ONE;

    return out;
}

// ============================================================
// 74160: BCD decade counter
// Same interface as 74161 but counts 0→9→0
// ============================================================

Chip74160::Chip74160(const std::string& id)
    : Device(id, "74160")
{
    add_pin("CR", "CR", Pin::INPUT, Pin::ACTIVE_LOW, "clear");
    add_pin("CP", "CP", Pin::INPUT, Pin::ACTIVE_HIGH, "clock");
    add_pin("LD", "LD", Pin::INPUT, Pin::ACTIVE_LOW, "load");
    add_pin("EP", "EP", Pin::INPUT, Pin::ACTIVE_HIGH, "count_enable");
    add_pin("ET", "ET", Pin::INPUT, Pin::ACTIVE_HIGH, "count_enable");
    for (int i = 0; i < 4; ++i)
        add_pin("D" + std::to_string(i), "D" + std::to_string(i),
                Pin::INPUT, Pin::ACTIVE_HIGH, "data");
    for (int i = 0; i < 4; ++i)
        add_pin("Q" + std::to_string(i), "Q" + std::to_string(i),
                Pin::OUTPUT, Pin::ACTIVE_HIGH, "data");
    add_pin("CO", "CO", Pin::OUTPUT, Pin::ACTIVE_HIGH, "carry_out");
    set_param_int("initial", 0);
}

void Chip74160::reset() {
    int init = get_param_int("initial", 0) % 10;
    set_param_int("count", init);
}

std::map<std::string, SignalValue> Chip74160::eval(
    const std::vector<Node>& nodes, const std::string&) {
    SignalValue cr = input("CR", nodes);
    std::map<std::string, SignalValue> out;
    int count = get_param_int("count", 0);

    if (cr == SignalValue::ZERO) {
        count = 0;
        set_param_int("count", count);
    } else if (rising_edge("CP", nodes)) {
        SignalValue ld = input("LD", nodes);
        SignalValue ep = input("EP", nodes);
        SignalValue et = input("ET", nodes);

        if (ld == SignalValue::ZERO) {
            uint32_t d_val = 0;
            bool d_ok = true;
            for (int i = 0; i < 4; ++i) {
                SignalValue d = input("D" + std::to_string(i), nodes);
                if (d == SignalValue::ONE) d_val |= (1u << i);
                else if (d != SignalValue::ZERO) d_ok = false;
            }
            count = d_ok ? static_cast<int>(d_val) % 10 : -1;
        } else if (ep == SignalValue::ONE && et == SignalValue::ONE) {
            if (count >= 0 && count < 9) count++;
            else if (count == 9) count = 0;
        }
        set_param_int("count", count);
    }

    for (int i = 0; i < 4; ++i) {
        out["Q" + std::to_string(i)] = (count < 0) ? SignalValue::X :
            ((count & (1 << i)) ? SignalValue::ONE : SignalValue::ZERO);
    }

    // CO = ET & Q3 & Q0 (when count=9)
    SignalValue et = input("ET", nodes);
    out["CO"] = (et == SignalValue::ONE && count == 9) ? SignalValue::ONE : SignalValue::ZERO;

    return out;
}

// ============================================================
// Factory
// ============================================================

std::unique_ptr<Device> create_chip(const std::string& type, const std::string& id) {
    // Accept both "74LSxxx" and "74xxx" / "74138" formats
    if (type == "74LS148" || type == "74148") return std::make_unique<Chip74LS148>(id);
    if (type == "74LS147" || type == "74147") return std::make_unique<Chip74LS147>(id);
    if (type == "74139")   return std::make_unique<Chip74139>(id);
    if (type == "74LS138" || type == "74138") return std::make_unique<Chip74LS138>(id);
    if (type == "74LS42" || type == "7442")   return std::make_unique<Chip74LS42>(id);
    if (type == "7448")    return std::make_unique<Chip7448>(id);
    if (type == "74153" || type == "74HC153") return std::make_unique<Chip74153>(id);
    if (type == "74LS151" || type == "74151") return std::make_unique<Chip74LS151>(id);
    if (type == "7485")    return std::make_unique<Chip7485>(id);
    if (type == "74280")   return std::make_unique<Chip74280>(id);
    if (type == "74283")   return std::make_unique<Chip74283>(id);
    if (type == "74175")   return std::make_unique<Chip74175>(id);
    if (type == "74LS195" || type == "74195") return std::make_unique<Chip74LS195>(id);
    if (type == "74LS194" || type == "74194") return std::make_unique<Chip74LS194>(id);
    if (type == "74161" || type == "74LS161") return std::make_unique<Chip74161>(id);
    if (type == "74163")   return std::make_unique<Chip74163>(id);
    if (type == "74191")   return std::make_unique<Chip74191>(id);
    if (type == "74160")   return std::make_unique<Chip74160>(id);
    return nullptr;
}

}  // namespace logic_sim
