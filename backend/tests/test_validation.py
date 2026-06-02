"""Tests for circuit validation."""

import pytest


class TestValidationService:
    """Unit tests for ValidationService."""

    def test_valid_circuit_passes(self, validation_service, basic_circuit):
        """A valid circuit should pass validation."""
        result = validation_service.validate(basic_circuit)
        assert result.valid is True
        assert len(result.errors) == 0

    def test_duplicate_device_id(self, validation_service):
        """Duplicate device IDs should produce an error."""
        from backend.app.models.schemas import CircuitDef, DeviceInstance
        circuit = CircuitDef(devices=[
            DeviceInstance(id="u1", type="AND"),
            DeviceInstance(id="u1", type="OR"),
        ])
        result = validation_service.validate(circuit)
        assert result.valid is False
        assert any("Duplicate" in e.message for e in result.errors)

    def test_unknown_device_type(self, validation_service):
        """Unknown device types should produce an error."""
        from backend.app.models.schemas import CircuitDef, DeviceInstance
        circuit = CircuitDef(devices=[
            DeviceInstance(id="u1", type="MADE_UP_CHIP_999"),
        ])
        result = validation_service.validate(circuit)
        assert result.valid is False
        assert any("Unknown" in e.message for e in result.errors)

    def test_floating_input_warning(self, validation_service, not_gate_circuit):
        """Unconnected input pins should produce a warning."""
        result = validation_service.validate(not_gate_circuit)
        # The NOT gate's input "I" is unconnected → warning
        assert len(result.warnings) >= 1
        assert any("Floating" in w.message for w in result.warnings)

    def test_wire_references_unknown_device(self, validation_service, basic_circuit):
        """Wires to unknown devices should produce errors."""
        from backend.app.models.schemas import CircuitDef, DeviceInstance, WireDef, WireEndpoint
        circuit = CircuitDef(
            devices=[
                DeviceInstance(id="u1", type="AND"),
                DeviceInstance(id="u2", type="OR"),
            ],
            wires=[
                WireDef(
                    id="w1",
                    from_=WireEndpoint(device="u1", pin="Y"),
                    to=WireEndpoint(device="u99", pin="I0"),
                ),
            ],
        )
        result = validation_service.validate(circuit)
        assert result.valid is False
        assert any("unknown device" in e.message.lower() for e in result.errors)

    def test_wire_references_nonexistent_pin(self, validation_service, basic_circuit):
        """Wires to non-existent pins should produce errors."""
        from backend.app.models.schemas import CircuitDef, DeviceInstance, WireDef, WireEndpoint
        circuit = CircuitDef(
            devices=[
                DeviceInstance(id="u1", type="AND"),
                DeviceInstance(id="u2", type="NOT"),
            ],
            wires=[
                WireDef(
                    id="w1",
                    from_=WireEndpoint(device="u1", pin="NONEXISTENT"),
                    to=WireEndpoint(device="u2", pin="I"),
                ),
            ],
        )
        result = validation_service.validate(circuit)
        assert result.valid is False

    def test_empty_circuit_passes(self, validation_service):
        """An empty circuit should pass validation (no errors)."""
        from backend.app.models.schemas import CircuitDef
        circuit = CircuitDef()
        result = validation_service.validate(circuit)
        assert result.valid is True
        assert len(result.errors) == 0

    def test_connected_inputs_no_warning(self, validation_service):
        """Connected input pins should not produce floating warnings."""
        from backend.app.models.schemas import CircuitDef, DeviceInstance, WireDef, WireEndpoint
        circuit = CircuitDef(
            devices=[
                DeviceInstance(id="u1", type="AND"),
                DeviceInstance(id="u2", type="NOT"),
            ],
            wires=[
                WireDef(
                    id="w1",
                    from_=WireEndpoint(device="u1", pin="Y"),
                    to=WireEndpoint(device="u2", pin="I"),
                ),
            ],
        )
        result = validation_service.validate(circuit)
        # AND has I0, I1 still floating → warnings for those
        # But the NOT's input "I" is connected, so only AND's unconnected pins warn
        floating_inputs = sum(
            1 for w in result.warnings
            if w.message == "Floating input" and "u2" in w.detail
        )
        assert floating_inputs == 0, "u2's I pin should not be floating"

    def test_output_conflict_direct_connection(self, validation_service):
        """Two output pins connected together should produce an error."""
        from backend.app.models.schemas import CircuitDef, DeviceInstance, WireDef, WireEndpoint
        circuit = CircuitDef(
            devices=[
                DeviceInstance(id="u1", type="AND"),
                DeviceInstance(id="u2", type="OR"),
            ],
            wires=[
                WireDef(
                    id="w1",
                    from_=WireEndpoint(device="u1", pin="Y"),
                    to=WireEndpoint(device="u2", pin="Y"),
                ),
            ],
        )
        result = validation_service.validate(circuit)
        assert result.valid is False
        assert any(e.message == "Output conflict" for e in result.errors)

    def test_output_conflict_on_shared_net(self, validation_service):
        """Output conflicts should be detected through an intermediate input pin."""
        from backend.app.models.schemas import CircuitDef, DeviceInstance, WireDef, WireEndpoint
        circuit = CircuitDef(
            devices=[
                DeviceInstance(id="u1", type="AND"),
                DeviceInstance(id="u2", type="OR"),
                DeviceInstance(id="u3", type="NOT"),
            ],
            wires=[
                WireDef(
                    id="w1",
                    from_=WireEndpoint(device="u1", pin="Y"),
                    to=WireEndpoint(device="u3", pin="I"),
                ),
                WireDef(
                    id="w2",
                    from_=WireEndpoint(device="u2", pin="Y"),
                    to=WireEndpoint(device="u3", pin="I"),
                ),
            ],
        )
        result = validation_service.validate(circuit)
        assert result.valid is False
        assert any(e.message == "Output conflict" for e in result.errors)


class TestValidationAPI:
    """HTTP endpoint tests for /api/simulations/validate."""

    @pytest.mark.anyio
    async def test_validate_valid_circuit(self, async_client):
        """POST /api/simulations/validate with a valid circuit."""
        payload = {
            "devices": [
                {"id": "u1", "type": "AND", "x": 0, "y": 0, "params": {"delay": 1}},
            ],
            "wires": [],
        }
        response = await async_client.post("/api/simulations/validate", json=payload)
        assert response.status_code == 200
        data = response.json()
        assert data["valid"] is True

    @pytest.mark.anyio
    async def test_validate_unknown_device(self, async_client):
        """POST /api/simulations/validate with unknown device type."""
        payload = {
            "devices": [
                {"id": "u1", "type": "FAKE_CHIP", "x": 0, "y": 0},
            ],
        }
        response = await async_client.post("/api/simulations/validate", json=payload)
        assert response.status_code == 200
        data = response.json()
        assert data["valid"] is False
        assert len(data["errors"]) >= 1

    @pytest.mark.anyio
    async def test_validate_duplicate_ids(self, async_client):
        """POST /api/simulations/validate with duplicate device IDs."""
        payload = {
            "devices": [
                {"id": "u1", "type": "AND", "x": 0, "y": 0},
                {"id": "u1", "type": "OR", "x": 100, "y": 0},
            ],
        }
        response = await async_client.post("/api/simulations/validate", json=payload)
        assert response.status_code == 200
        data = response.json()
        assert data["valid"] is False

    @pytest.mark.anyio
    async def test_circuits_validate_documented_route(self, async_client):
        """POST /api/circuits/validate should match the documented API."""
        payload = {
            "devices": [
                {"id": "u1", "type": "AND", "x": 0, "y": 0},
                {"id": "u2", "type": "OR", "x": 100, "y": 0},
            ],
            "wires": [
                {"id": "w1", "from": {"device": "u1", "pin": "Y"}, "to": {"device": "u2", "pin": "Y"}},
            ],
        }
        response = await async_client.post("/api/circuits/validate", json=payload)
        assert response.status_code == 200
        data = response.json()
        assert data["valid"] is False
        assert any(e["message"] == "Output conflict" for e in data["errors"])
