"""Circuit validation API routes."""

from __future__ import annotations

from fastapi import APIRouter

from ..models.schemas import CircuitDef, ValidationResult
from .simulations import get_validation_service

router = APIRouter(prefix="/api/circuits", tags=["circuits"])


@router.post("/validate", response_model=ValidationResult)
async def validate_circuit(circuit: CircuitDef):
    """Validate a circuit JSON without running simulation."""
    return get_validation_service().validate(circuit)
