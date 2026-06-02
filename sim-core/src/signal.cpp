#include "logic_sim/signal.hpp"

namespace logic_sim {

// ---- Conversion ----

SignalValue signal_from_char(char c) {
    switch (c) {
        case '0': return SignalValue::ZERO;
        case '1': return SignalValue::ONE;
        case 'X':
        case 'x': return SignalValue::X;
        case 'Z':
        case 'z': return SignalValue::Z;
        default:  return SignalValue::X;
    }
}

SignalValue signal_from_string(const std::string& s) {
    if (s == "0") return SignalValue::ZERO;
    if (s == "1") return SignalValue::ONE;
    if (s == "X" || s == "x") return SignalValue::X;
    if (s == "Z" || s == "z") return SignalValue::Z;
    return SignalValue::X;
}

char signal_to_char(SignalValue v) {
    switch (v) {
        case SignalValue::ZERO: return '0';
        case SignalValue::ONE:  return '1';
        case SignalValue::X:    return 'X';
        case SignalValue::Z:    return 'Z';
    }
    return 'X';
}

std::string signal_to_string(SignalValue v) {
    switch (v) {
        case SignalValue::ZERO: return "0";
        case SignalValue::ONE:  return "1";
        case SignalValue::X:    return "X";
        case SignalValue::Z:    return "Z";
    }
    return "X";
}

bool is_strong(SignalValue v) {
    return v == SignalValue::ZERO || v == SignalValue::ONE;
}

// ---- Single-input operations ----

SignalValue signal_not(SignalValue a) {
    switch (a) {
        case SignalValue::ZERO: return SignalValue::ONE;
        case SignalValue::ONE:  return SignalValue::ZERO;
        case SignalValue::X:    return SignalValue::X;
        case SignalValue::Z:    return SignalValue::X;  // Z treated as X
    }
    return SignalValue::X;
}

// ---- Two-input operations ----

SignalValue signal_and(SignalValue a, SignalValue b) {
    // Any 0 → 0
    if (a == SignalValue::ZERO || b == SignalValue::ZERO)
        return SignalValue::ZERO;
    // No 0, but has X/Z → X
    if (a == SignalValue::X || a == SignalValue::Z ||
        b == SignalValue::X || b == SignalValue::Z)
        return SignalValue::X;
    // Both 1 → 1
    return SignalValue::ONE;
}

SignalValue signal_or(SignalValue a, SignalValue b) {
    // Any 1 → 1
    if (a == SignalValue::ONE || b == SignalValue::ONE)
        return SignalValue::ONE;
    // No 1, but has X/Z → X
    if (a == SignalValue::X || a == SignalValue::Z ||
        b == SignalValue::X || b == SignalValue::Z)
        return SignalValue::X;
    // Both 0 → 0
    return SignalValue::ZERO;
}

SignalValue signal_xor(SignalValue a, SignalValue b) {
    // Any X/Z → X
    if (a == SignalValue::X || a == SignalValue::Z ||
        b == SignalValue::X || b == SignalValue::Z)
        return SignalValue::X;
    // Both known: parity
    return (a != b) ? SignalValue::ONE : SignalValue::ZERO;
}

SignalValue signal_nand(SignalValue a, SignalValue b) {
    return signal_not(signal_and(a, b));
}

SignalValue signal_nor(SignalValue a, SignalValue b) {
    return signal_not(signal_or(a, b));
}

SignalValue signal_xnor(SignalValue a, SignalValue b) {
    return signal_not(signal_xor(a, b));
}

// ---- Multi-input operations ----

SignalValue signal_and(const std::vector<SignalValue>& inputs) {
    if (inputs.empty()) return SignalValue::X;
    SignalValue result = SignalValue::ONE;
    for (auto v : inputs) {
        result = signal_and(result, v);
        if (result == SignalValue::ZERO) break;  // early exit
    }
    return result;
}

SignalValue signal_or(const std::vector<SignalValue>& inputs) {
    if (inputs.empty()) return SignalValue::X;
    SignalValue result = SignalValue::ZERO;
    for (auto v : inputs) {
        result = signal_or(result, v);
        if (result == SignalValue::ONE) break;  // early exit
    }
    return result;
}

SignalValue signal_xor(const std::vector<SignalValue>& inputs) {
    if (inputs.empty()) return SignalValue::X;
    SignalValue result = SignalValue::ZERO;
    for (auto v : inputs) {
        result = signal_xor(result, v);
    }
    return result;
}

SignalValue signal_nand(const std::vector<SignalValue>& inputs) {
    return signal_not(signal_and(inputs));
}

SignalValue signal_nor(const std::vector<SignalValue>& inputs) {
    return signal_not(signal_or(inputs));
}

SignalValue signal_xnor(const std::vector<SignalValue>& inputs) {
    return signal_not(signal_xor(inputs));
}

// ---- Bus / vector helpers ----

std::string signal_vector_to_bits(const std::vector<SignalValue>& vec) {
    std::string result;
    for (auto v : vec) {
        switch (v) {
            case SignalValue::ZERO: result += '0'; break;
            case SignalValue::ONE:  result += '1'; break;
            default:                result += 'X'; break;
        }
    }
    return result;
}

std::vector<SignalValue> bits_to_signal_vector(const std::string& bits) {
    std::vector<SignalValue> result;
    result.reserve(bits.size());
    for (char c : bits) {
        result.push_back(signal_from_char(c));
    }
    return result;
}

bool signal_bus_to_uint(const std::vector<SignalValue>& bus, uint32_t& out) {
    out = 0;
    for (size_t i = 0; i < bus.size(); ++i) {
        auto v = bus[i];
        if (v == SignalValue::ONE) {
            out |= (1u << i);
        } else if (v != SignalValue::ZERO) {
            return false;  // X or Z in bus
        }
    }
    return true;
}

std::vector<SignalValue> uint_to_signal_bus(uint32_t val, size_t width) {
    std::vector<SignalValue> result;
    result.reserve(width);
    for (size_t i = 0; i < width; ++i) {
        result.push_back((val & (1u << i)) ? SignalValue::ONE : SignalValue::ZERO);
    }
    return result;
}

}  // namespace logic_sim
