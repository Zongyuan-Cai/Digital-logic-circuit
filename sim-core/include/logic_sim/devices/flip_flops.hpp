#pragma once

#include <memory>
#include "../device.hpp"

namespace logic_sim {

// ============================================================
// Basic latches
// ============================================================

/// NAND-type basic RS latch.
/// Inputs: S_N (active low set), R_N (active low reset)
/// Outputs: Q, QN
/// S_N=0,R_N=0 → Q=X,QN=X (illegal)
class RSLatchNand : public Device {
public:
    explicit RSLatchNand(const std::string& id);
    std::map<std::string, SignalValue> eval(
        const std::vector<Node>& nodes, const std::string& changed) override;
    void reset() override;
};

/// NOR-type basic RS latch.
/// Inputs: S (active high set), R (active high reset)
/// Outputs: Q, QN
/// S=1,R=1 → Q=X,QN=X (illegal)
class RSLatchNor : public Device {
public:
    explicit RSLatchNor(const std::string& id);
    std::map<std::string, SignalValue> eval(
        const std::vector<Node>& nodes, const std::string& changed) override;
    void reset() override;
};

/// Gated RS latch (clocked RS).
/// Inputs: S, R, EN
/// Outputs: Q, QN
/// EN=0: hold; EN=1: behaves like RS latch
class GatedRSLatch : public Device {
public:
    explicit GatedRSLatch(const std::string& id);
    std::map<std::string, SignalValue> eval(
        const std::vector<Node>& nodes, const std::string& changed) override;
    void reset() override;
};

/// Gated D latch (transparent latch).
/// Inputs: D, EN
/// Outputs: Q, QN
/// EN=0: hold; EN=1: Q follows D (transparent)
class DLatch : public Device {
public:
    explicit DLatch(const std::string& id);
    std::map<std::string, SignalValue> eval(
        const std::vector<Node>& nodes, const std::string& changed) override;
    void reset() override;
};

// ============================================================
// Synchronous (clocked) flip-flops
// ============================================================

/// Synchronous RS flip-flop (level-sensitive).
/// Inputs: S, R, CP
/// Outputs: Q, QN
/// Updates on active CP edge (configurable rising/falling).
class SyncRSFF : public Device {
public:
    explicit SyncRSFF(const std::string& id);
    std::map<std::string, SignalValue> eval(
        const std::vector<Node>& nodes, const std::string& changed) override;
    void reset() override;
};

/// Synchronous D flip-flop.
/// Inputs: D, CP
/// Outputs: Q, QN
/// Q ← D on active CP edge.
class SyncDFF : public Device {
public:
    explicit SyncDFF(const std::string& id);
    std::map<std::string, SignalValue> eval(
        const std::vector<Node>& nodes, const std::string& changed) override;
    void reset() override;
};

/// Synchronous JK flip-flop.
/// Inputs: J, K, CP
/// Outputs: Q, QN
/// J=0,K=0: hold; J=0,K=1: reset; J=1,K=0: set; J=1,K=1: toggle.
class SyncJKFF : public Device {
public:
    explicit SyncJKFF(const std::string& id);
    std::map<std::string, SignalValue> eval(
        const std::vector<Node>& nodes, const std::string& changed) override;
    void reset() override;
};

/// Synchronous T flip-flop.
/// Inputs: T, CP
/// Outputs: Q, QN
/// T=0: hold; T=1: toggle.
class SyncTFF : public Device {
public:
    explicit SyncTFF(const std::string& id);
    std::map<std::string, SignalValue> eval(
        const std::vector<Node>& nodes, const std::string& changed) override;
    void reset() override;
};

// ============================================================
// Master-slave flip-flops
// ============================================================

/// Master-slave RS flip-flop.
/// Inputs: S, R, CP
/// Outputs: Q, QN
/// Master latches on CP=1, slave transfers on CP→0.
class MasterSlaveRSFF : public Device {
public:
    explicit MasterSlaveRSFF(const std::string& id);
    std::map<std::string, SignalValue> eval(
        const std::vector<Node>& nodes, const std::string& changed) override;
    void reset() override;
};

/// Master-slave D flip-flop.
class MasterSlaveDFF : public Device {
public:
    explicit MasterSlaveDFF(const std::string& id);
    std::map<std::string, SignalValue> eval(
        const std::vector<Node>& nodes, const std::string& changed) override;
    void reset() override;
};

/// Master-slave JK flip-flop.
class MasterSlaveJKFF : public Device {
public:
    explicit MasterSlaveJKFF(const std::string& id);
    std::map<std::string, SignalValue> eval(
        const std::vector<Node>& nodes, const std::string& changed) override;
    void reset() override;
};

// ============================================================
// Edge-triggered flip-flops
// ============================================================

/// Edge-triggered D flip-flop.
/// Inputs: D, CP; optional PRE (preset), CLR (clear) — async, active low.
/// Outputs: Q, QN
class EdgeDFF : public Device {
public:
    explicit EdgeDFF(const std::string& id);
    std::map<std::string, SignalValue> eval(
        const std::vector<Node>& nodes, const std::string& changed) override;
    void reset() override;
};

/// Edge-triggered JK flip-flop.
/// Inputs: J, K, CP; optional PRE, CLR.
/// Outputs: Q, QN
class EdgeJKFF : public Device {
public:
    explicit EdgeJKFF(const std::string& id);
    std::map<std::string, SignalValue> eval(
        const std::vector<Node>& nodes, const std::string& changed) override;
    void reset() override;
};

/// Maintenance-blocking (维持-阻塞) JK flip-flop.
/// Behaviorally equivalent to positive-edge-triggered JK.
class BlockingJKFF : public Device {
public:
    explicit BlockingJKFF(const std::string& id);
    std::map<std::string, SignalValue> eval(
        const std::vector<Node>& nodes, const std::string& changed) override;
    void reset() override;
};

/// Factory: create a flip-flop by type string.
std::unique_ptr<Device> create_flip_flop(const std::string& type,
                                          const std::string& id);

}  // namespace logic_sim
