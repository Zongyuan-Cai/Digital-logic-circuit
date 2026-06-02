"""Tests for /api/devices endpoints."""

import pytest


class TestDeviceLibraryService:
    """Tests for the DeviceLibraryService directly."""

    def test_loads_all_devices(self, device_library):
        """Device library should load all 39 devices from JSON."""
        idx = device_library.index
        assert idx is not None
        assert idx.total_devices == 39

    def test_list_all_returns_devices(self, device_library):
        """list_all should return all device definitions."""
        all_devs = device_library.list_all()
        assert len(all_devs) >= 39

    def test_filter_by_category(self, device_library):
        """list_all should filter by category."""
        gates = device_library.list_all(category="gate")
        assert len(gates) == 7
        gate_types = {g.type for g in gates}
        assert gate_types == {"AND", "OR", "NOT", "NAND", "NOR", "XOR", "XNOR"}

    def test_filter_by_flip_flop(self, device_library):
        """list_all should filter flip_flops."""
        ffs = device_library.list_all(category="flip_flop")
        assert len(ffs) == 14

    def test_filter_by_chip(self, device_library):
        """list_all should filter chips."""
        chips = device_library.list_all(category="chip")
        assert len(chips) >= 18  # Some may have aliases

    def test_get_device_by_type(self, device_library):
        """get() should return device by type string."""
        dev = device_library.get("AND")
        assert dev is not None
        assert dev.type == "AND"
        assert dev.category == "gate"

    def test_get_nonexistent_device(self, device_library):
        """get() should return None for unknown type."""
        assert device_library.get("NONEXISTENT") is None

    def test_every_pdf_chip_exists(self, device_library):
        """All 18 PDF chips must exist in the library."""
        pdf_chips = [
            "74LS148", "74LS147", "74139", "74LS138", "74138",
            "74LS42", "7448", "74153", "74HC153", "74LS151", "74151",
            "7485", "74280", "74283", "74175", "74LS195", "74195",
            "74LS194", "74194", "74161", "74LS161", "74163", "74191", "74160",
        ]
        for chip_type in pdf_chips:
            dev = device_library.get(chip_type)
            assert dev is not None, f"PDF chip '{chip_type}' not found in library"

    def test_every_gate_has_pins(self, device_library):
        """Every gate must have at least one input and one output."""
        gates = device_library.list_all(category="gate")
        for gate in gates:
            assert len(gate.pins) >= 2, f"Gate {gate.type} has too few pins"

    def test_every_device_has_unique_pin_ids(self, device_library):
        """Pin IDs must be unique within each device."""
        for dev in device_library.list_all():
            pin_ids = [p.id for p in dev.pins]
            assert len(pin_ids) == len(set(pin_ids)), \
                f"Device {dev.type} has duplicate pin IDs: {pin_ids}"

    def test_pin_directions_valid(self, device_library):
        """Pin directions must be 'input', 'output', or 'bidirectional'."""
        valid = {"input", "output", "bidirectional"}
        for dev in device_library.list_all():
            for pin in dev.pins:
                assert pin.direction in valid, \
                    f"Device {dev.type} pin {pin.id} has invalid direction '{pin.direction}'"

    def test_pin_active_valid(self, device_library):
        """Pin active levels must be 'high' or 'low'."""
        valid = {"high", "low"}
        for dev in device_library.list_all():
            for pin in dev.pins:
                assert pin.active in valid, \
                    f"Device {dev.type} pin {pin.id} has invalid active '{pin.active}'"


class TestDevicesAPI:
    """Tests for the /api/devices HTTP endpoints."""

    @pytest.mark.anyio
    async def test_get_devices(self, async_client):
        """GET /api/devices should return device list."""
        response = await async_client.get("/api/devices")
        assert response.status_code == 200
        data = response.json()
        assert len(data) >= 39

    @pytest.mark.anyio
    async def test_get_devices_filtered(self, async_client):
        """GET /api/devices?category=gate should return only gates."""
        response = await async_client.get("/api/devices?category=gate")
        assert response.status_code == 200
        data = response.json()
        assert len(data) == 7

    @pytest.mark.anyio
    async def test_get_single_device(self, async_client):
        """GET /api/devices/{type} should return one device."""
        response = await async_client.get("/api/devices/74LS138")
        assert response.status_code == 200
        data = response.json()
        assert data["type"] == "74LS138"
        assert len(data["pins"]) >= 3  # at least address + output pins

    @pytest.mark.anyio
    async def test_get_nonexistent_device(self, async_client):
        """GET /api/devices/UNKNOWN should return 404."""
        response = await async_client.get("/api/devices/NONEXISTENT")
        assert response.status_code == 404

    @pytest.mark.anyio
    async def test_get_device_index(self, async_client):
        """GET /api/devices/index should return library index."""
        response = await async_client.get("/api/devices/index")
        assert response.status_code == 200
        data = response.json()
        assert data["total_devices"] == 39

    @pytest.mark.anyio
    async def test_get_device_types(self, async_client):
        """GET /api/devices/types should return all type strings."""
        response = await async_client.get("/api/devices/types")
        assert response.status_code == 200
        data = response.json()
        assert "AND" in data
        assert "74LS138" in data or "74138" in data

    @pytest.mark.anyio
    async def test_health_check(self, async_client):
        """GET /api/health should return status."""
        response = await async_client.get("/api/health")
        assert response.status_code == 200
        data = response.json()
        assert data["status"] == "ok"
        assert "version" in data
