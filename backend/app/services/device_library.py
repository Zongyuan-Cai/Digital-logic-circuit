"""Device library service — loads and queries device metadata JSON."""

from __future__ import annotations

import json
import os
from pathlib import Path
from typing import Optional

from ..models.schemas import DeviceDef, PinDef, DeviceLibraryIndex


class DeviceLibraryService:
    """Loads device metadata from device-library/*.json and serves queries."""

    def __init__(self, library_dir: Optional[str] = None):
        if library_dir is None:
            # Default: project_root/device-library
            library_dir = os.path.join(
                os.path.dirname(__file__), "..", "..", "..", "device-library"
            )
        self._dir = Path(library_dir).resolve()
        self._devices: dict[str, DeviceDef] = {}
        self._index: Optional[DeviceLibraryIndex] = None
        self._load()

    def _load(self) -> None:
        # Load index
        index_path = self._dir / "devices.json"
        if index_path.exists():
            with open(index_path, "r", encoding="utf-8") as f:
                raw = json.load(f)
            self._index = DeviceLibraryIndex(**raw)

        # Load each category file
        for category_file in ["gates.json", "flip_flops.json", "chips.json", "io_devices.json"]:
            path = self._dir / category_file
            if not path.exists():
                continue
            with open(path, "r", encoding="utf-8") as f:
                entries = json.load(f)
            for entry in entries:
                # Build unified device definition
                pins: list[PinDef] = []
                for p in entry.get("pins", []):
                    pins.append(PinDef(
                        id=p["id"], name=p["name"],
                        direction=p.get("direction", "input"),
                        role=p.get("role", "data"),
                        active=p.get("active", "high"),
                    ))
                for p in entry.get("output_pins", []):
                    pins.append(PinDef(
                        id=p["id"], name=p["name"],
                        direction=p.get("direction", "output"),
                        role=p.get("role", "data"),
                        active=p.get("active", "high"),
                    ))

                dev = DeviceDef(
                    type=entry["type"],
                    name=entry["name"],
                    category=entry.get("category", "chip"),
                    family=entry.get("family", ""),
                    description=entry.get("description", ""),
                    aliases=entry.get("aliases", []),
                    pins=pins,
                    params=entry.get("params", {}),
                )
                self._devices[dev.type] = dev

                # Register aliases
                for alias in entry.get("aliases", []):
                    if alias not in self._devices:
                        alias_dev = dev.model_copy()
                        alias_dev.type = alias
                        self._devices[alias] = alias_dev

    @property
    def index(self) -> Optional[DeviceLibraryIndex]:
        return self._index

    def list_all(self, category: Optional[str] = None) -> list[DeviceDef]:
        result = list(self._devices.values())
        if category:
            result = [d for d in result if d.category == category]
        # Deduplicate by type, preferring the canonical (non-alias) entry
        seen: set[str] = set()
        deduped: list[DeviceDef] = []
        for d in result:
            if d.type not in seen:
                seen.add(d.type)
                deduped.append(d)
        return deduped

    def get(self, device_type: str) -> Optional[DeviceDef]:
        """Get a device by type name (supports alias lookup)."""
        return self._devices.get(device_type)

    def exists(self, device_type: str) -> bool:
        return device_type in self._devices

    def list_types(self) -> list[str]:
        return list(self._devices.keys())
