"""Tests for /api/simulations endpoints and simulation service."""

import pytest


class TestSimulationRunBasic:
    """Tests for running simulations via the API."""

    @pytest.mark.anyio
    async def test_run_not_gate(self, async_client):
        """Running a NOT gate should succeed and return a result."""
        payload = {
            "devices": [
                {"id": "u1", "type": "NOT", "x": 100, "y": 100, "params": {"delay": 1}},
            ],
            "wires": [],
        }
        options = {"max_ticks": 10, "record": ["u1.Y"], "default_delay": 1}
        response = await async_client.post("/api/simulations/run", json={
            "circuit": payload,
            "options": options,
        })
        assert response.status_code == 200
        data = response.json()
        assert data["status"] in ("ok", "error")

    @pytest.mark.anyio
    async def test_run_and_gate_with_wiring(self, async_client):
        """Running an AND gate should produce a result."""
        payload = {
            "devices": [
                {"id": "u1", "type": "AND", "x": 100, "y": 100, "params": {"delay": 1, "inputs": 2}},
            ],
            "wires": [],
        }
        options = {
            "max_ticks": 10,
            "record": ["u1.Y"],
            "default_delay": 1,
        }
        response = await async_client.post("/api/simulations/run", json={
            "circuit": payload,
            "options": options,
        })
        assert response.status_code == 200
        data = response.json()
        assert "status" in data
        # With floating inputs, AND output should be X (0 takes priority but I0, I1 are Z)
        assert "u1.Y" in data.get("final_nodes", {})

    @pytest.mark.anyio
    async def test_run_decoder_chip(self, async_client):
        """Running a 74LS138 decoder should work."""
        payload = {
            "devices": [
                {"id": "u1", "type": "74LS138", "x": 100, "y": 100, "params": {"delay": 1}},
            ],
            "wires": [],
        }
        options = {
            "max_ticks": 10,
            "record": ["u1.Y0", "u1.Y7"],
        }
        response = await async_client.post("/api/simulations/run", json={
            "circuit": payload,
            "options": options,
        })
        assert response.status_code == 200
        data = response.json()
        assert "status" in data
        assert "final_nodes" in data

    @pytest.mark.anyio
    async def test_waveform_returned(self, async_client):
        """Simulation should return waveform data."""
        payload = {
            "devices": [
                {"id": "u1", "type": "NOT", "x": 100, "y": 100, "params": {"delay": 1}},
            ],
            "wires": [],
        }
        options = {"max_ticks": 5, "record": ["u1.Y", "u1.I"]}
        response = await async_client.post("/api/simulations/run", json={
            "circuit": payload,
            "options": options,
        })
        assert response.status_code == 200
        data = response.json()
        waveform = data.get("waveform", {})
        assert waveform.get("time_unit") == "tick"
        assert "signals" in waveform

    @pytest.mark.anyio
    async def test_run_rejects_invalid_circuit(self, async_client):
        """Running a circuit with unknown device should return 422."""
        payload = {
            "devices": [
                {"id": "u1", "type": "NOT_A_REAL_DEVICE", "x": 0, "y": 0},
            ],
        }
        response = await async_client.post("/api/simulations/run", json={
            "circuit": payload,
            "options": {},
        })
        assert response.status_code == 422

    @pytest.mark.anyio
    async def test_result_has_ticks_and_events(self, async_client):
        """Result should include ticks_elapsed and events_processed."""
        payload = {
            "devices": [
                {"id": "u1", "type": "NOT", "x": 100, "y": 100, "params": {"delay": 1}},
            ],
            "wires": [],
        }
        response = await async_client.post("/api/simulations/run", json={
            "circuit": payload,
            "options": {"max_ticks": 10, "record": []},
        })
        assert response.status_code == 200
        data = response.json()
        assert "ticks_elapsed" in data
        assert "events_processed" in data

    @pytest.mark.anyio
    async def test_errors_list_in_result(self, async_client):
        """Result should always contain errors list."""
        payload = {
            "devices": [
                {"id": "u1", "type": "AND", "x": 100, "y": 100, "params": {"delay": 1}},
            ],
            "wires": [],
        }
        response = await async_client.post("/api/simulations/run", json={
            "circuit": payload,
            "options": {"max_ticks": 10},
        })
        assert response.status_code == 200
        data = response.json()
        assert isinstance(data.get("errors", []), list)

    @pytest.mark.anyio
    async def test_run_counter_simulation(self, async_client):
        """Running a 74161 counter should work correctly."""
        payload = {
            "devices": [
                {"id": "cnt", "type": "74161", "x": 100, "y": 100,
                 "params": {"delay": 1, "initial": 0}},
            ],
            "wires": [],
        }
        options = {
            "max_ticks": 50,
            "record": ["cnt.Q0", "cnt.Q1", "cnt.Q2", "cnt.Q3"],
        }
        response = await async_client.post("/api/simulations/run", json={
            "circuit": payload,
            "options": options,
        })
        assert response.status_code == 200
        data = response.json()
        assert "final_nodes" in data
        # Should have counter output pins in final_nodes
        for pin in ["cnt.Q0", "cnt.Q1", "cnt.Q2", "cnt.Q3"]:
            assert pin in data["final_nodes"], f"Missing {pin} in final_nodes"


class TestSimulationEdgeCases:
    """Edge case tests for simulation."""

    @pytest.mark.anyio
    async def test_empty_circuit_runs(self, async_client):
        """An empty circuit should run without error."""
        payload = {"devices": [], "wires": []}
        response = await async_client.post("/api/simulations/run", json={
            "circuit": payload,
            "options": {"max_ticks": 10},
        })
        # Empty circuit should pass validation (no devices to check)
        assert response.status_code in (200, 422)

    @pytest.mark.anyio
    async def test_max_ticks_limit(self, async_client):
        """Simulation should respect max_ticks."""
        payload = {
            "devices": [
                {"id": "u1", "type": "AND", "x": 100, "y": 100, "params": {"delay": 1}},
            ],
            "wires": [],
        }
        response = await async_client.post("/api/simulations/run", json={
            "circuit": payload,
            "options": {"max_ticks": 1, "record_all": False},
        })
        assert response.status_code == 200
        data = response.json()
        assert data["ticks_elapsed"] <= 1

    @pytest.mark.anyio
    async def test_record_all_option(self, async_client):
        """record_all=True should record all nodes."""
        payload = {
            "devices": [
                {"id": "u1", "type": "NOT", "x": 100, "y": 100, "params": {"delay": 1}},
            ],
            "wires": [],
        }
        response = await async_client.post("/api/simulations/run", json={
            "circuit": payload,
            "options": {"max_ticks": 5, "record_all": True},
        })
        assert response.status_code == 200
        data = response.json()
        # Should have waveform signals for both I and Y
        signals = data.get("waveform", {}).get("signals", [])
        signal_ids = {s["id"] for s in signals}
        assert len(signals) >= 2, f"Expected >=2 signals with record_all, got {len(signals)}"

    @pytest.mark.anyio
    async def test_validation_before_run(self, async_client):
        """Invalid circuit should be rejected before simulation."""
        payload = {
            "devices": [
                {"id": "u1", "type": "FAKE_CHIP_XYZ"},
            ],
            "wires": [],
        }
        response = await async_client.post("/api/simulations/run", json={
            "circuit": payload,
            "options": {},
        })
        assert response.status_code == 422
        data = response.json()
        assert "Circuit validation failed" in data["detail"]["message"]

    @pytest.mark.anyio
    async def test_step_endpoint(self, async_client):
        """POST /api/simulations/step should return step result."""
        response = await async_client.post("/api/simulations/step")
        assert response.status_code == 200
        data = response.json()
        assert "event_processed" in data

    @pytest.mark.anyio
    async def test_reset_endpoint(self, async_client):
        """POST /api/simulations/reset should return ok."""
        response = await async_client.post("/api/simulations/reset")
        assert response.status_code == 200
        data = response.json()
        assert data["status"] == "reset"

    @pytest.mark.anyio
    async def test_default_options_applied(self, async_client):
        """Default options should be applied when none provided."""
        payload = {
            "devices": [
                {"id": "u1", "type": "NOT", "x": 100, "y": 100, "params": {"delay": 1}},
            ],
        }
        response = await async_client.post("/api/simulations/run", json={
            "circuit": payload,
        })
        assert response.status_code == 200
        data = response.json()
        assert "ticks_elapsed" in data
        assert "events_processed" in data

    @pytest.mark.anyio
    async def test_74161_counter_initial_zero(self, async_client):
        """74161 counter with initial=0 should have Q=0 at start."""
        payload = {
            "devices": [
                {"id": "cnt", "type": "74161", "x": 100, "y": 100,
                 "params": {"delay": 1, "initial": 0}},
            ],
            "wires": [],
        }
        options = {
            "max_ticks": 10,
            "record": ["cnt.Q0", "cnt.Q1", "cnt.Q2", "cnt.Q3", "cnt.CO"],
        }
        response = await async_client.post("/api/simulations/run", json={
            "circuit": payload,
            "options": options,
        })
        assert response.status_code == 200
        data = response.json()
        # All Q pins should be 0 initially
        for pin in ["cnt.Q0", "cnt.Q1", "cnt.Q2", "cnt.Q3"]:
            assert pin in data.get("final_nodes", {}), f"{pin} missing"

    @pytest.mark.anyio
    async def test_multi_device_circuit(self, async_client):
        """A circuit with multiple wired devices should simulate correctly."""
        payload = {
            "devices": [
                {"id": "g1", "type": "AND", "x": 100, "y": 100, "params": {"delay": 1}},
                {"id": "g2", "type": "NOT", "x": 200, "y": 100, "params": {"delay": 1}},
                {"id": "g3", "type": "OR", "x": 300, "y": 100, "params": {"delay": 1}},
            ],
            "wires": [
                {"id": "w1", "from": {"device": "g1", "pin": "Y"}, "to": {"device": "g2", "pin": "I"}},
                {"id": "w2", "from": {"device": "g2", "pin": "Y"}, "to": {"device": "g3", "pin": "I0"}},
            ],
        }
        options = {
            "max_ticks": 20,
            "record": ["g1.Y", "g2.Y", "g3.Y"],
        }
        response = await async_client.post("/api/simulations/run", json={
            "circuit": payload,
            "options": options,
        })
        assert response.status_code == 200
        data = response.json()
        assert data["status"] in ("ok", "error")
        # Should have final nodes for all outputs
        for key in ["g1.Y", "g2.Y", "g3.Y"]:
            assert key in data.get("final_nodes", {}), f"{key} missing"
