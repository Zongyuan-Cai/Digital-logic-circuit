"""FastAPI application entry point."""

from __future__ import annotations

import os
import sys
from pathlib import Path

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from fastapi.staticfiles import StaticFiles
from .runtime import VERSION, resource_root

# Support both single-configuration Linux builds and native Windows .pyd builds.
_PROJECT_DIR = Path(__file__).resolve().parents[2]
_BUILD_DIRS = [
    _PROJECT_DIR / "build" / "windows" / "python" if sys.platform == "win32"
    else _PROJECT_DIR / "build" / "sim-core",
    _PROJECT_DIR / "build",
]
if os.environ.get("LOGIC_SIM_BUILD_DIR"):
    _BUILD_DIRS.insert(0, Path(os.environ["LOGIC_SIM_BUILD_DIR"]))
for _build_dir in reversed(_BUILD_DIRS):
    if _build_dir.is_dir():
        sys.path.insert(0, str(_build_dir))


app = FastAPI(
    title="Digital Logic Circuit Simulator API",
    version=VERSION,
    description="Backend API for digital logic circuit simulation teaching system",
)

# CORS — allow frontend dev server
app.add_middleware(
    CORSMiddleware,
    allow_origins=["http://localhost:3000", "http://localhost:5173", "http://127.0.0.1:3000", "http://127.0.0.1:5173"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

from .api import circuits, devices, projects, simulations  # noqa: E402

app.include_router(devices.router)
app.include_router(projects.router)
app.include_router(simulations.router)
app.include_router(circuits.router)


@app.get("/api/health")
async def health():
    from .services.simulator_service import SimulatorService
    return {
        "status": "ok",
        "version": VERSION,
        "sim_core_available": SimulatorService.is_available(),
    }


@app.on_event("startup")
async def startup():
    print("[startup] Server started")
    from .services.simulator_service import SimulatorService
    print(f"[startup] Sim core: {'available' if SimulatorService.is_available() else 'NOT available'}")


# Frozen distribution serves the built frontend from the same local origin.
if getattr(sys, "frozen", False):
    app.mount("/", StaticFiles(directory=resource_root() / "frontend" / "dist", html=True), name="frontend")
