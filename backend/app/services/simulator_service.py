"""Simulator service — wraps the C++ logic_sim pybind11 module."""

from __future__ import annotations

from typing import Optional

from ..models.schemas import CircuitDef, SimOptions, SimResult, SimStepResult


class SimulatorService:
    """Manages a simulation session, delegating to the C++ logic_sim module."""

    def __init__(self):
        self._circuit: Optional[CircuitDef] = None
        self._sim = None  # C++ Simulator instance if step-by-step mode

    def run(self, circuit: CircuitDef, options: SimOptions) -> SimResult:
        """Run a complete simulation and return the result."""
        import logic_sim  # pybind11 module

        # Convert CircuitDef to dict for pybind11
        circuit_dict = self._circuit_to_dict(circuit)
        options_dict = {
            "max_ticks": options.max_ticks,
            "max_events": options.max_events,
            "record_all": options.record_all,
            "record": options.record,
            "default_delay": options.default_delay,
        }

        raw: dict = logic_sim.run(circuit_dict, options_dict)

        # Convert waveform
        from ..models.schemas import WaveformData, WaveSignal, WavePoint
        waveform = WaveformData(
            time_unit=raw.get("waveform", {}).get("time_unit", "tick"),
            signals=[
                WaveSignal(
                    id=sig["id"],
                    name=sig["name"],
                    values=[WavePoint(t=vp["t"], v=vp["v"]) for vp in sig.get("values", [])],
                )
                for sig in raw.get("waveform", {}).get("signals", [])
            ],
        )

        return SimResult(
            status=raw.get("status", "error"),
            errors=raw.get("errors", []),
            warnings=raw.get("warnings", []),
            final_nodes=raw.get("final_nodes", {}),
            waveform=waveform,
            ticks_elapsed=raw.get("ticks_elapsed", 0),
            events_processed=raw.get("events_processed", 0),
        )

    def step(self) -> SimStepResult:
        """Run a single simulation step. Requires a prior load_circuit + reset."""
        if self._sim is None:
            return SimStepResult(event_processed=False, events_remaining=0)
        had_event = self._sim.step()
        # We don't have direct access to current_time from the C++ side via step()
        # but we can approximate
        return SimStepResult(
            event_processed=had_event,
            events_remaining=0,  # not directly exposed
        )

    def load_circuit(self, circuit: CircuitDef) -> None:
        """Load a circuit for step-by-step simulation."""
        import logic_sim
        self._circuit = circuit
        circuit_dict = self._circuit_to_dict(circuit)
        # For step mode we'd need the C++ Simulator class exposed via pybind11
        # Currently only logic_sim.run() is exposed, so we create and discard.
        # This is a placeholder for when Simulator class is exposed.
        self._sim = None  # Will be set when pybind exposes Simulator class

    def reset(self) -> None:
        if self._sim is not None:
            self._sim.reset()

    @staticmethod
    def _circuit_to_dict(circuit: CircuitDef) -> dict:
        """Convert CircuitDef Pydantic model to dict for pybind11."""
        devices = []
        for dev in circuit.devices:
            d = {"id": dev.id, "type": dev.type}
            if dev.params:
                d["params"] = dev.params
            devices.append(d)

        wires = []
        for w in circuit.wires:
            wires.append({
                "from": {"device": w.from_.device, "pin": w.from_.pin},
                "to": {"device": w.to.device, "pin": w.to.pin},
            })

        return {"devices": devices, "wires": wires}

    @staticmethod
    def is_available() -> bool:
        """Check if the C++ logic_sim module is importable."""
        try:
            import logic_sim  # noqa: F401
            return True
        except ImportError:
            return False
