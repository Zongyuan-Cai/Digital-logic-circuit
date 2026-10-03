"""Locate read-only bundle resources and writable user data."""

from __future__ import annotations

import os
import sys
from pathlib import Path

VERSION = "1.0.0"


def resource_root() -> Path:
    if getattr(sys, "frozen", False):
        return Path(sys._MEIPASS)
    return Path(__file__).resolve().parents[2]


def data_root() -> Path:
    override = os.environ.get("LOGIC_LAB_DATA_DIR")
    if override:
        return Path(override).expanduser().resolve()
    if getattr(sys, "frozen", False):
        return Path(os.environ.get("LOCALAPPDATA", str(Path.home()))) / "LogicLab"
    return resource_root() / "data"
