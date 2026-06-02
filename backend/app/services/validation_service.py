"""Circuit validation service."""

from __future__ import annotations

from ..models.schemas import (
    CircuitDef,
    ValidationMessage,
    ValidationResult,
    PinDirection,
    PinDef,
)
from .device_library import DeviceLibraryService


class ValidationService:
    """Validates circuit JSON before simulation."""

    def __init__(self, device_library: DeviceLibraryService):
        self._lib = device_library

    def validate(self, circuit: CircuitDef) -> ValidationResult:
        errors: list[ValidationMessage] = []
        warnings: list[ValidationMessage] = []

        # 1. device.id must be unique
        seen_ids: set[str] = set()
        device_types: dict[str, str] = {}
        for dev in circuit.devices:
            if dev.id in seen_ids:
                errors.append(ValidationMessage(
                    severity="error",
                    message="Duplicate device ID",
                    detail=f"Device id '{dev.id}' appears more than once.",
                ))
            seen_ids.add(dev.id)
            device_types.setdefault(dev.id, dev.type)

            # 2. device.type must exist in library
            if not self._lib.exists(dev.type):
                errors.append(ValidationMessage(
                    severity="error",
                    message="Unknown device type",
                    detail=f"Device '{dev.id}' has unknown type '{dev.type}'.",
                ))

        pin_defs: dict[tuple[str, str], PinDef] = {}
        for dev in circuit.devices:
            dev_def = self._lib.get(dev.type)
            if dev_def is None:
                continue
            for pin_def in dev_def.pins:
                pin_defs[(dev.id, pin_def.id)] = pin_def

        # 3. Wires: check referenced devices and pins exist
        for wire in circuit.wires:
            for endpoint, role in [(wire.from_, "from"), (wire.to, "to")]:
                dev = endpoint.device
                pin = endpoint.pin
                # Check device exists in circuit
                if dev not in seen_ids:
                    errors.append(ValidationMessage(
                        severity="error",
                        message="Wire references unknown device",
                        detail=f"Wire '{wire.id}' {role} references unknown device '{dev}'.",
                    ))
                    continue
                # Check pin exists on device type
                dev_def = self._lib.get(device_types.get(dev, ""))
                if dev_def and (dev, pin) not in pin_defs:
                    errors.append(ValidationMessage(
                        severity="error",
                        message="Wire references unknown pin",
                        detail=f"Device '{dev}' has no pin '{pin}'.",
                    ))

        # 4. Output conflicts: any connected net with multiple output pins is invalid.
        valid_wires = [
            wire for wire in circuit.wires
            if (wire.from_.device, wire.from_.pin) in pin_defs
            and (wire.to.device, wire.to.pin) in pin_defs
        ]
        for output_group in self._connected_output_groups(valid_wires, pin_defs):
            if len(output_group) > 1:
                endpoints = ", ".join(
                    f"{device}.{pin}" for device, pin in sorted(output_group)
                )
                errors.append(ValidationMessage(
                    severity="error",
                    message="Output conflict",
                    detail=f"Multiple output pins drive the same net: {endpoints}.",
                ))

        # 5. Floating inputs — warning
        connected_pins: set[tuple[str, str]] = set()
        for wire in circuit.wires:
            connected_pins.add((wire.from_.device, wire.from_.pin))
            connected_pins.add((wire.to.device, wire.to.pin))

        for dev in circuit.devices:
            dev_def = self._lib.get(dev.type)
            if not dev_def:
                continue
            for pin_def in dev_def.pins:
                if pin_def.direction == PinDirection.INPUT:
                    if (dev.id, pin_def.id) not in connected_pins:
                        warnings.append(ValidationMessage(
                            severity="warning",
                            message="Floating input",
                            detail=f"Device '{dev.id}' pin '{pin_def.id}' is unconnected.",
                        ))

        valid = len(errors) == 0
        return ValidationResult(valid=valid, errors=errors, warnings=warnings)

    @staticmethod
    def _connected_output_groups(
        wires,
        pin_defs: dict[tuple[str, str], PinDef],
    ) -> list[set[tuple[str, str]]]:
        parent: dict[tuple[str, str], tuple[str, str]] = {}

        def find(endpoint: tuple[str, str]) -> tuple[str, str]:
            parent.setdefault(endpoint, endpoint)
            if parent[endpoint] != endpoint:
                parent[endpoint] = find(parent[endpoint])
            return parent[endpoint]

        def union(a: tuple[str, str], b: tuple[str, str]) -> None:
            root_a = find(a)
            root_b = find(b)
            if root_a != root_b:
                parent[root_b] = root_a

        for wire in wires:
            union(
                (wire.from_.device, wire.from_.pin),
                (wire.to.device, wire.to.pin),
            )

        outputs_by_net: dict[tuple[str, str], set[tuple[str, str]]] = {}
        for endpoint, pin_def in pin_defs.items():
            if endpoint not in parent:
                continue
            if pin_def.direction == PinDirection.OUTPUT:
                outputs_by_net.setdefault(find(endpoint), set()).add(endpoint)

        return list(outputs_by_net.values())
