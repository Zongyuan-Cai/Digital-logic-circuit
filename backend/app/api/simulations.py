"""Simulation API routes."""

from __future__ import annotations

from typing import Optional

from fastapi import APIRouter, HTTPException

from ..models.schemas import (
    CircuitDef,
    SimOptions,
    SimResult,
    SimStepResult,
    ValidationResult,
)
from ..services.simulator_service import SimulatorService
from ..services.validation_service import ValidationService
from .devices import get_device_library

router = APIRouter(prefix="/api/simulations", tags=["simulations"])

_sim_service: Optional[SimulatorService] = None
_validation_service: Optional[ValidationService] = None


def get_simulator_service() -> SimulatorService:
    global _sim_service
    if _sim_service is None:
        _sim_service = SimulatorService()
    return _sim_service


def get_validation_service() -> ValidationService:
    global _validation_service
    if _validation_service is None:
        _validation_service = ValidationService(get_device_library())
    return _validation_service


@router.post("/validate", response_model=ValidationResult)
async def validate_circuit(circuit: CircuitDef):
    """Validate a circuit JSON without running simulation."""
    return get_validation_service().validate(circuit)


@router.post("/run", response_model=SimResult)
async def run_simulation(
    circuit: CircuitDef,
    options: SimOptions = SimOptions(),  # noqa: B008
):
    """Run a complete simulation on the given circuit."""
    # Validate first
    validation = get_validation_service().validate(circuit)
    if not validation.valid:
        raise HTTPException(
            status_code=422,
            detail={
                "message": "Circuit validation failed",
                "errors": [e.model_dump() for e in validation.errors],
            },
        )

    sim_svc = get_simulator_service()
    if not sim_svc.is_available():
        raise HTTPException(
            status_code=503,
            detail="C++ simulation core (logic_sim) not available. "
                   "Build with: cmake --build build/sim-core",
        )

    try:
        result = sim_svc.run(circuit, options)
    except Exception as e:
        return SimResult(status="error", errors=[str(e)])

    # Attach validation warnings
    result.warnings = [w.message + ": " + w.detail for w in validation.warnings] + result.warnings
    return result


@router.post("/step", response_model=SimStepResult)
async def step_simulation():
    """Run a single step. Circuit must be loaded first."""
    return get_simulator_service().step()


@router.post("/reset")
async def reset_simulation():
    """Reset the simulation state."""
    get_simulator_service().reset()
    return {"status": "reset"}
