#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace logic_sim {

/// Unified signal value enumeration for the simulation.
/// Matches the 4-value logic in the design document.
enum class SignalValue : uint8_t {
    ZERO = 0,  ///< Logic 0
    ONE  = 1,  ///< Logic 1
    X    = 2,  ///< Unknown / conflict / indeterminate
    Z    = 3   ///< High impedance
};

// ---- Conversion functions ----

/// Parse a single character: '0', '1', 'X', 'Z'
SignalValue signal_from_char(char c);

/// Parse a string: "0", "1", "X", "Z"
SignalValue signal_from_string(const std::string& s);

/// Convert to character representation
char signal_to_char(SignalValue v);

/// Convert to string representation
std::string signal_to_string(SignalValue v);

/// Check if a value is a strong driver (0 or 1)
bool is_strong(SignalValue v);

// ---- Single-input operations ----

/// Logical NOT, following the doc rules:
///   0->1, 1->0, X->X, Z->X
SignalValue signal_not(SignalValue a);

// ---- Two-input operations ----

/// Logical AND:
///   Any 0 -> 0; no 0 but has X/Z -> X; all 1 -> 1
SignalValue signal_and(SignalValue a, SignalValue b);

/// Logical OR:
///   Any 1 -> 1; no 1 but has X/Z -> X; all 0 -> 0
SignalValue signal_or(SignalValue a, SignalValue b);

/// Logical XOR:
///   Any X/Z -> X; otherwise parity
SignalValue signal_xor(SignalValue a, SignalValue b);

/// Derived: NAND = NOT(AND)
SignalValue signal_nand(SignalValue a, SignalValue b);

/// Derived: NOR = NOT(OR)
SignalValue signal_nor(SignalValue a, SignalValue b);

/// Derived: XNOR = NOT(XOR)
SignalValue signal_xnor(SignalValue a, SignalValue b);

// ---- Multi-input operations ----

SignalValue signal_and(const std::vector<SignalValue>& inputs);
SignalValue signal_or(const std::vector<SignalValue>& inputs);
SignalValue signal_xor(const std::vector<SignalValue>& inputs);
SignalValue signal_nand(const std::vector<SignalValue>& inputs);
SignalValue signal_nor(const std::vector<SignalValue>& inputs);
SignalValue signal_xnor(const std::vector<SignalValue>& inputs);

// ---- Bus / vector helpers ----

/// Convert a vector of SignalValue to a binary string, returning X if any bit is X/Z
std::string signal_vector_to_bits(const std::vector<SignalValue>& vec);

/// Parse a bit string like "0101" into a vector of SignalValue
std::vector<SignalValue> bits_to_signal_vector(const std::string& bits);

/// Convert a bus of signals to an unsigned integer.
/// Returns true on success; if any bit is X/Z, the conversion is indeterminate.
bool signal_bus_to_uint(const std::vector<SignalValue>& bus, uint32_t& out);

/// Convert an unsigned integer to a bus of signals (width bits, LSB first)
std::vector<SignalValue> uint_to_signal_bus(uint32_t val, size_t width);

}  // namespace logic_sim
