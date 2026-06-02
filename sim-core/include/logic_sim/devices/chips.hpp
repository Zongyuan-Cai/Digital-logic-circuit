#pragma once

#include <memory>
#include "../device.hpp"

namespace logic_sim {

// ============================================================
// Encoder chips
// ============================================================

/// 74LS148: 8-to-3 priority encoder.
/// Inputs:  EI (active low enable), I0-I7 (active low)
/// Outputs: A2,A1,A0 (active low encoded), GS (active low valid), EO (active low cascade)
class Chip74LS148 : public Device {
public:
    explicit Chip74LS148(const std::string& id);
    std::map<std::string, SignalValue> eval(
        const std::vector<Node>& nodes, const std::string& changed) override;
    void reset() override {}
};

/// 74LS147: BCD (decimal to 4-bit) priority encoder.
/// Inputs:  I1-I9 (active low, digit 1-9; I0 is implied — no input → 0)
/// Outputs: D,C,B,A (active low BCD)
class Chip74LS147 : public Device {
public:
    explicit Chip74LS147(const std::string& id);
    std::map<std::string, SignalValue> eval(
        const std::vector<Node>& nodes, const std::string& changed) override;
    void reset() override {}
};

// ============================================================
// Decoder chips
// ============================================================

/// 74139: Dual 2-to-4 decoder.
/// Two independent decoders sharing the same chip.
/// Each half: A0,A1 (address), E (active low enable), Y0-Y3 (active low outputs)
class Chip74139 : public Device {
public:
    explicit Chip74139(const std::string& id);
    std::map<std::string, SignalValue> eval(
        const std::vector<Node>& nodes, const std::string& changed) override;
    void reset() override {}
};

/// 74LS138 / 74138: 3-to-8 decoder.
/// Inputs:  A,B,C (address), G1 (active high enable), G2A,G2B (active low enable)
/// Outputs: Y0-Y7 (active low)
class Chip74LS138 : public Device {
public:
    explicit Chip74LS138(const std::string& id);
    std::map<std::string, SignalValue> eval(
        const std::vector<Node>& nodes, const std::string& changed) override;
    void reset() override {}
};

/// 74LS42: BCD to decimal decoder.
/// Inputs:  A,B,C,D (BCD, active high)
/// Outputs: Y0-Y9 (active low)
/// Invalid inputs 10-15 → all outputs inactive (high).
class Chip74LS42 : public Device {
public:
    explicit Chip74LS42(const std::string& id);
    std::map<std::string, SignalValue> eval(
        const std::vector<Node>& nodes, const std::string& changed) override;
    void reset() override {}
};

/// 7448: BCD to 7-segment decoder/driver.
/// Inputs:  A,B,C,D (BCD), LT (lamp test), RBI (ripple blanking input),
///          BI/RBO (blanking input / ripple blanking output, bidirectional)
/// Outputs: a,b,c,d,e,f,g (active high segments)
class Chip7448 : public Device {
public:
    explicit Chip7448(const std::string& id);
    std::map<std::string, SignalValue> eval(
        const std::vector<Node>& nodes, const std::string& changed) override;
    void reset() override {}
};

// ============================================================
// Multiplexer chips
// ============================================================

/// 74153 / 74HC153: Dual 4-to-1 data selector.
/// Two 4-to-1 muxes sharing select lines A,B.
/// Each half: C0-C3 (data), G (active low enable), Y (output)
class Chip74153 : public Device {
public:
    explicit Chip74153(const std::string& id);
    std::map<std::string, SignalValue> eval(
        const std::vector<Node>& nodes, const std::string& changed) override;
    void reset() override {}
};

/// 74LS151 / 74151: 8-to-1 data selector.
/// Inputs:  D0-D7 (data), A,B,C (select), G (active low strobe/enable)
/// Outputs: Y (active high), W (active low, complement of Y)
class Chip74LS151 : public Device {
public:
    explicit Chip74LS151(const std::string& id);
    std::map<std::string, SignalValue> eval(
        const std::vector<Node>& nodes, const std::string& changed) override;
    void reset() override {}
};

// ============================================================
// Arithmetic / comparison chips
// ============================================================

/// 7485: 4-bit magnitude comparator.
/// Inputs:  A0-A3, B0-B3, AGTB_in, AEQB_in, ALTB_in (cascade)
/// Outputs: AGTB_out, AEQB_out, ALTB_out
class Chip7485 : public Device {
public:
    explicit Chip7485(const std::string& id);
    std::map<std::string, SignalValue> eval(
        const std::vector<Node>& nodes, const std::string& changed) override;
    void reset() override {}
};

/// 74280: 9-bit parity generator/checker.
/// Inputs:  A-I (9 data bits)
/// Outputs: EVEN, ODD
class Chip74280 : public Device {
public:
    explicit Chip74280(const std::string& id);
    std::map<std::string, SignalValue> eval(
        const std::vector<Node>& nodes, const std::string& changed) override;
    void reset() override {}
};

/// 74283: 4-bit binary full adder.
/// Inputs:  A0-A3, B0-B3, C0 (carry in)
/// Outputs: S0-S3 (sum), C4 (carry out)
class Chip74283 : public Device {
public:
    explicit Chip74283(const std::string& id);
    std::map<std::string, SignalValue> eval(
        const std::vector<Node>& nodes, const std::string& changed) override;
    void reset() override {}
};

// ============================================================
// Register / shift register chips
// ============================================================

/// 74175: 4-bit register with async clear.
/// Inputs:  CLR (active low async clear), CP, D0-D3
/// Outputs: Q0-Q3, Q0N-Q3N (complementary)
class Chip74175 : public Device {
public:
    explicit Chip74175(const std::string& id);
    std::map<std::string, SignalValue> eval(
        const std::vector<Node>& nodes, const std::string& changed) override;
    void reset() override;
};

/// 74LS195 / 74195: 4-bit right-shift register.
/// Inputs:  CLR (async clear), CP, J, K, D0-D3, SH/LD (shift/load)
/// Outputs: Q0-Q3, Q3N
class Chip74LS195 : public Device {
public:
    explicit Chip74LS195(const std::string& id);
    std::map<std::string, SignalValue> eval(
        const std::vector<Node>& nodes, const std::string& changed) override;
    void reset() override;
};

/// 74LS194 / 74194: 4-bit bidirectional shift register.
/// Inputs:  CR (async clear), CP, S0,S1 (mode), DSR (serial right), DSL (serial left),
///          D0-D3 (parallel data)
/// Outputs: Q0-Q3
class Chip74LS194 : public Device {
public:
    explicit Chip74LS194(const std::string& id);
    std::map<std::string, SignalValue> eval(
        const std::vector<Node>& nodes, const std::string& changed) override;
    void reset() override;
};

// ============================================================
// Counter chips
// ============================================================

/// 74161 / 74LS161: 4-bit synchronous binary counter, async clear.
/// Inputs:  CR (async clear), CP, LD (sync load), EP, ET, D0-D3
/// Outputs: Q0-Q3, CO (carry out)
class Chip74161 : public Device {
public:
    explicit Chip74161(const std::string& id);
    std::map<std::string, SignalValue> eval(
        const std::vector<Node>& nodes, const std::string& changed) override;
    void reset() override;
};

/// 74163: 4-bit synchronous binary counter, sync clear.
/// Same as 74161 but CR is synchronous (sampled at clock edge).
class Chip74163 : public Device {
public:
    explicit Chip74163(const std::string& id);
    std::map<std::string, SignalValue> eval(
        const std::vector<Node>& nodes, const std::string& changed) override;
    void reset() override;
};

/// 74191: 4-bit up/down counter with async load.
/// Inputs:  CTEN (active low count enable), D/U (0=up,1=down),
///          LD (async load, active low), CP, D0-D3
/// Outputs: Q0-Q3, MAX_MIN, RCO (ripple clock out)
class Chip74191 : public Device {
public:
    explicit Chip74191(const std::string& id);
    std::map<std::string, SignalValue> eval(
        const std::vector<Node>& nodes, const std::string& changed) override;
    void reset() override;
};

/// 74160: BCD decade counter (0-9).
/// Same interface as 74161 but counts 0→9→0.
class Chip74160 : public Device {
public:
    explicit Chip74160(const std::string& id);
    std::map<std::string, SignalValue> eval(
        const std::vector<Node>& nodes, const std::string& changed) override;
    void reset() override;
};

/// Factory: create a chip by type string.  Supports alias resolution
/// (e.g. "74138" → Chip74LS138, "74LS161" → Chip74161).
std::unique_ptr<Device> create_chip(const std::string& type, const std::string& id);

}  // namespace logic_sim
