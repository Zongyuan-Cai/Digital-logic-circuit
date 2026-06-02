"""FastAPI application entry point."""

from __future__ import annotations

import os
import sys

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware

# Ensure the C++ build directory is on path for logic_sim imports
_BUILD_DIR = os.path.join(
    os.path.dirname(__file__), "..", "..", "build", "sim-core"
)
if os.path.isdir(_BUILD_DIR):
    sys.path.insert(0, _BUILD_DIR)


app = FastAPI(
    title="Digital Logic Circuit Simulator API",
    version="1.0.0",
    description="Backend API for digital logic circuit simulation teaching system",
)

# CORS — allow frontend dev server
app.add_middleware(
    CORSMiddleware,
    allow_origins=["http://localhost:3000", "http://localhost:5173", "http://127.0.0.1:3000"],
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
        "version": "1.0.0",
        "sim_core_available": SimulatorService.is_available(),
    }


@app.on_event("startup")
async def startup():
    print("[startup] Server started")
    from .services.simulator_service import SimulatorService
    print(f"[startup] Sim core: {'available' if SimulatorService.is_available() else 'NOT available'}")
