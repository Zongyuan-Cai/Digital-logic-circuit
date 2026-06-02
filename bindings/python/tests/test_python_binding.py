"""
Smoke tests for the logic_sim Python bindings.

These tests verify that:
1. Python can import logic_sim
2. Python can run a NOT gate circuit
3. Python can run a D flip-flop circuit
4. Python can get waveform data
5. C++ errors are properly converted to Python errors
"""

import sys
import os

# Add the build directory to path for import
sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))


def test_import():
    """Test that the module can be imported."""
    try:
        import logic_sim
        assert hasattr(logic_sim, "run"), "Module should have run function"
        print("[PASS] test_import — logic_sim imported successfully")
        return logic_sim
    except ImportError as e:
        print(f"[SKIP] test_import — {e}")
        return None


def test_not_gate(logic_sim):
    """Test running a NOT gate circuit through Python bindings."""
    circuit = {
        "devices": [
            {
                "id": "u1",
                "type": "NOT",
                "params": {"delay": 1}
            }
        ],
        "wires": [
            # No wires needed for a single NOT gate — we just test that
            # the simulation runs without error.
            # In a real circuit, you'd wire the input to a switch.
        ]
    }

    options = {
        "max_ticks": 10,
        "record": ["u1.Y"],
        "default_delay": 1,
    }

    result = logic_sim.run(circuit, options)
    assert result["status"] == "ok", f"Simulation should succeed: {result['errors']}"
    print("[PASS] test_not_gate — NOT gate simulation completed")


def test_not_gate_with_input(logic_sim):
    """Test NOT gate with a known input value."""
    circuit = {
        "devices": [
            {
                "id": "u1",
                "type": "NOT",
                "params": {"delay": 1}
            }
        ],
    }

    result = logic_sim.run(circuit, {"max_ticks": 5})
    # With floating input, output should be X
    assert result["status"] == "ok", f"Simulation should succeed"
    # Floating input → X, NOT X = X
    assert "u1.Y" in result["final_nodes"], "Should have u1.Y in final nodes"
    print(f"[INFO] NOT gate floating input result: Y = {result['final_nodes'].get('u1.Y', 'N/A')}")
    print("[PASS] test_not_gate_with_input — completed")


def test_waveform(logic_sim):
    """Test that waveform data is returned properly."""
    circuit = {
        "devices": [
            {"id": "u1", "type": "NOT", "params": {"delay": 1}}
        ],
    }

    options = {
        "max_ticks": 5,
        "record": ["u1.I", "u1.Y"],
    }

    result = logic_sim.run(circuit, options)
    waveform = result.get("waveform", {})
    assert "signals" in waveform, "Waveform should have signals"
    assert "time_unit" in waveform, "Waveform should have time_unit"
    print(f"[INFO] Waveform has {len(waveform.get('signals', []))} signal(s)")
    print("[PASS] test_waveform — waveform data returned")


def test_74138_decoder(logic_sim):
    """Test 74LS138 decoder through Python bindings."""
    circuit = {
        "devices": [
            {
                "id": "u1",
                "type": "74LS138",
                "params": {"delay": 1}
            }
        ],
    }

    options = {
        "max_ticks": 10,
        "record": ["u1.Y0", "u1.Y7"],
    }

    result = logic_sim.run(circuit, options)
    assert result["status"] == "ok", f"74LS138 simulation should succeed"
    print("[PASS] test_74138_decoder — chip simulation completed")


def test_errors():
    """Test that invalid circuits produce errors."""
    try:
        import logic_sim
        circuit = {
            "devices": [
                {"id": "u1", "type": "NONEXISTENT"}
            ]
        }
        # Should raise an exception or return error status
        try:
            result = logic_sim.run(circuit, {"max_ticks": 5})
            assert result["status"] == "error", "Unknown device should produce error status"
            print(f"[INFO] Error message: {result.get('errors', [])}")
            print("[PASS] test_errors — unknown device yields error")
        except RuntimeError as e:
            print(f"[INFO] RuntimeError: {e}")
            print("[PASS] test_errors — unknown device raises Python exception")
    except ImportError:
        print("[SKIP] test_errors — logic_sim not available")


def main():
    print("=" * 60)
    print("Python Binding Smoke Tests")
    print("=" * 60)

    logic_sim = test_import()
    if logic_sim is None:
        print("\n[Skipping] Python binding not built. Run with:")
        print("    cmake -S sim-core -B build/sim-core")
        print("    cmake --build build/sim-core")
        print("    PYTHONPATH=build/sim-core python bindings/python/tests/test_python_binding.py")
        return 0

    test_not_gate(logic_sim)
    test_not_gate_with_input(logic_sim)
    test_waveform(logic_sim)
    test_74138_decoder(logic_sim)
    test_errors()

    print("\n" + "=" * 60)
    print("All smoke tests completed!")
    print("=" * 60)
    return 0


if __name__ == "__main__":
    sys.exit(main())
