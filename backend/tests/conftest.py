"""Shared fixtures for backend tests."""

import asyncio
import os
import sys
from pathlib import Path

import pytest
from httpx import AsyncClient, ASGITransport

# Ensure backend is importable
sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

# Add sim-core build dir for logic_sim pybind11 module
_build_dir = os.path.join(os.path.dirname(__file__), "..", "..", "build", "sim-core")
if os.path.isdir(_build_dir):
    sys.path.insert(0, _build_dir)

# Check if logic_sim is actually importable
try:
    import logic_sim  # noqa: F401
    SIM_CORE_AVAILABLE = True
except ImportError:
    SIM_CORE_AVAILABLE = False


# Skip marker for tests that need the C++ sim core
needs_sim_core = pytest.mark.skipif(
    not SIM_CORE_AVAILABLE,
    reason="C++ logic_sim module not built. Run: cmake --build build/sim-core",
)


@pytest.fixture
def device_library():
    """Create a fresh DeviceLibraryService instance."""
    from backend.app.services.device_library import DeviceLibraryService
    return DeviceLibraryService()


@pytest.fixture
def validation_service(device_library):
    """Create a ValidationService with the device library."""
    from backend.app.services.validation_service import ValidationService
    return ValidationService(device_library)


@pytest.fixture
def async_client(tmp_path, monkeypatch):
    """Create an async HTTP client for testing the FastAPI app."""
    from backend.app.main import app
    from backend.app.api import projects as projects_api
    from backend.app.storage.project_store import ProjectStore

    store = ProjectStore(str(tmp_path / "projects.db"))
    asyncio.run(store.init())
    monkeypatch.setattr(projects_api, "_store", store)

    client = AsyncClient(transport=ASGITransport(app=app), base_url="http://test")
    yield client
    asyncio.run(client.aclose())


@pytest.fixture
def basic_circuit():
    """A minimal valid circuit with one AND gate."""
    from backend.app.models.schemas import CircuitDef, DeviceInstance
    return CircuitDef(
        devices=[DeviceInstance(id="u1", type="AND", x=100, y=100, params={"delay": 1, "inputs": 2})],
        wires=[],
    )


@pytest.fixture
def not_gate_circuit():
    """A circuit with one NOT gate."""
    from backend.app.models.schemas import CircuitDef, DeviceInstance
    return CircuitDef(
        devices=[DeviceInstance(id="u1", type="NOT", x=100, y=100, params={"delay": 1})],
        wires=[],
    )
