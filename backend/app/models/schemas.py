"""Pydantic models for the Digital Logic Circuit backend API."""

from __future__ import annotations

from datetime import datetime
from enum import Enum
from typing import Any, Optional

from pydantic import BaseModel, Field


# ---- Enums ----

class PinDirection(str, Enum):
    INPUT = "input"
    OUTPUT = "output"
    BIDIRECTIONAL = "bidirectional"


class ActiveLevel(str, Enum):
    HIGH = "high"
    LOW = "low"


class SignalValue(str, Enum):
    ZERO = "0"
    ONE = "1"
    X = "X"
    Z = "Z"


class DeviceCategory(str, Enum):
    GATE = "gate"
    FLIP_FLOP = "flip_flop"
    CHIP = "chip"
    IO = "io"


# ---- Device Library ----

class PinDef(BaseModel):
    id: str
    name: str
    direction: PinDirection
    role: str = "data"
    active: ActiveLevel = ActiveLevel.HIGH


class DeviceDef(BaseModel):
    type: str
    name: str
    category: DeviceCategory
    sub_category: str = ""
    family: str = ""
    description: str = ""
    aliases: list[str] = Field(default_factory=list)
    pins: list[PinDef] = Field(default_factory=list)
    output_pins: list[PinDef] = Field(default_factory=list)
    params: dict[str, Any] = Field(default_factory=dict)


class DeviceLibraryIndex(BaseModel):
    version: str
    description: str
    categories: dict[str, str]
    families: dict[str, str]
    device_count: dict[str, int]
    total_devices: int


# ---- Circuit JSON ----

class DeviceInstance(BaseModel):
    id: str
    type: str
    x: float = 0.0
    y: float = 0.0
    params: dict[str, Any] = Field(default_factory=dict)


class WireEndpoint(BaseModel):
    device: str
    pin: str


class WireDef(BaseModel):
    id: str = ""
    from_: WireEndpoint = Field(alias="from")
    to: WireEndpoint

    model_config = {"populate_by_name": True}


class CircuitDef(BaseModel):
    version: str = "1.0"
    devices: list[DeviceInstance] = Field(default_factory=list)
    wires: list[WireDef] = Field(default_factory=list)


# ---- Simulation ----

class SimOptions(BaseModel):
    max_ticks: int = Field(default=1000, ge=1)
    max_events: int = Field(default=100000, ge=1)
    record_all: bool = False
    record: list[str] = Field(default_factory=list)
    default_delay: int = Field(default=1, ge=1)
    floating_input: str = "X"


class WavePoint(BaseModel):
    t: int
    v: str


class WaveSignal(BaseModel):
    id: str
    name: str
    values: list[WavePoint] = Field(default_factory=list)


class WaveformData(BaseModel):
    time_unit: str = "tick"
    signals: list[WaveSignal] = Field(default_factory=list)


class SimResult(BaseModel):
    status: str  # "ok" | "error" | "timeout"
    errors: list[str] = Field(default_factory=list)
    warnings: list[str] = Field(default_factory=list)
    final_nodes: dict[str, str] = Field(default_factory=dict)
    waveform: WaveformData = Field(default_factory=WaveformData)
    ticks_elapsed: int = 0
    events_processed: int = 0


class SimStepResult(BaseModel):
    event_processed: bool
    current_time: int = 0
    events_remaining: int = 0


# ---- Validation ----

class ValidationMessage(BaseModel):
    severity: str  # "warning" | "error"
    message: str
    detail: str = ""


class ValidationResult(BaseModel):
    valid: bool
    errors: list[ValidationMessage] = Field(default_factory=list)
    warnings: list[ValidationMessage] = Field(default_factory=list)


# ---- Project ----

class ProjectMeta(BaseModel):
    id: str
    name: str
    description: str = ""
    created_at: datetime = Field(default_factory=datetime.utcnow)
    updated_at: datetime = Field(default_factory=datetime.utcnow)


class ProjectData(ProjectMeta):
    circuit: CircuitDef = Field(default_factory=CircuitDef)


class ProjectCreate(BaseModel):
    name: str
    description: str = ""


class ProjectUpdate(BaseModel):
    name: Optional[str] = None
    description: Optional[str] = None
    circuit: Optional[CircuitDef] = None
