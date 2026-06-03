#pragma once

#include <memory>
#include "../device.hpp"

namespace logic_sim {

/// VCC / constant-high source. Outputs 1 on its output pin.
class VCC : public Device {
public:
    explicit VCC(const std::string& id);
    std::map<std::string, SignalValue> eval(
        const std::vector<Node>& nodes, const std::string& changed) override;
    void reset() override {}
};

/// GND / constant-low source. Outputs 0 on its output pin.
class GND : public Device {
public:
    explicit GND(const std::string& id);
    std::map<std::string, SignalValue> eval(
        const std::vector<Node>& nodes, const std::string& changed) override;
    void reset() override {}
};

/// User-controllable switch. Output = param("value", 0).
class Switch : public Device {
public:
    explicit Switch(const std::string& id);
    std::map<std::string, SignalValue> eval(
        const std::vector<Node>& nodes, const std::string& changed) override;
    void reset() override {}
};

/// Clock source. Output toggles 0/1 based on period param.
/// Params: period (ticks, default 2), duty (0.0-1.0, default 0.5), initial (default "0")
class Clock : public Device {
public:
    explicit Clock(const std::string& id);
    std::map<std::string, SignalValue> eval(
        const std::vector<Node>& nodes, const std::string& changed) override;
    void reset() override;
};

/// LED output indicator. Single input, no output. Visual only.
class LED : public Device {
public:
    explicit LED(const std::string& id);
    std::map<std::string, SignalValue> eval(
        const std::vector<Node>& nodes, const std::string& changed) override;
    void reset() override {}
};

/// Probe — records input value for waveform. No output.
class Probe : public Device {
public:
    explicit Probe(const std::string& id);
    std::map<std::string, SignalValue> eval(
        const std::vector<Node>& nodes, const std::string& changed) override;
    void reset() override {}
};

/// 7-segment display. 7 inputs (a-g) or 4 BCD inputs.
/// Params: mode ("segment" or "bcd", default "segment")
class SevenSegment : public Device {
public:
    explicit SevenSegment(const std::string& id);
    std::map<std::string, SignalValue> eval(
        const std::vector<Node>& nodes, const std::string& changed) override;
    void reset() override {}
};

/// Factory
std::unique_ptr<Device> create_io_device(const std::string& type, const std::string& id);

}  // namespace logic_sim
