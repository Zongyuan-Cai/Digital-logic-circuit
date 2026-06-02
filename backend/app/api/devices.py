"""Device library API routes."""

from __future__ import annotations

from typing import Optional

from fastapi import APIRouter, HTTPException, Query

from ..models.schemas import DeviceDef, DeviceLibraryIndex
from ..services.device_library import DeviceLibraryService

router = APIRouter(prefix="/api/devices", tags=["devices"])

# Lazy-init singleton
_library: Optional[DeviceLibraryService] = None


def get_device_library() -> DeviceLibraryService:
    global _library
    if _library is None:
        _library = DeviceLibraryService()
    return _library


@router.get("", response_model=list[DeviceDef])
async def list_devices(category: Optional[str] = Query(None)):
    """Return all device metadata, optionally filtered by category."""
    return get_device_library().list_all(category=category)


@router.get("/index", response_model=DeviceLibraryIndex)
async def device_index():
    """Return device library index."""
    lib = get_device_library()
    idx = lib.index
    if idx is None:
        raise HTTPException(status_code=404, detail="Index not found")
    return idx


@router.get("/types", response_model=list[str])
async def device_types():
    """Return all known device type strings."""
    return get_device_library().list_types()


@router.get("/{device_type}", response_model=DeviceDef)
async def get_device(device_type: str):
    """Return metadata for a single device type."""
    lib = get_device_library()
    dev = lib.get(device_type)
    if dev is None:
        raise HTTPException(status_code=404, detail=f"Device '{device_type}' not found")
    return dev
