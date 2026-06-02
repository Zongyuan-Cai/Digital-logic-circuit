"""Project CRUD API routes."""

from __future__ import annotations

import asyncio
from typing import Optional

from fastapi import APIRouter, HTTPException

from ..models.schemas import ProjectCreate, ProjectData, ProjectMeta, ProjectUpdate
from ..storage.project_store import ProjectStore

router = APIRouter(prefix="/api/projects", tags=["projects"])

_store: Optional[ProjectStore] = None
_init_lock = asyncio.Lock()


async def get_project_store() -> ProjectStore:
    global _store
    if _store is None:
        async with _init_lock:
            if _store is None:
                _store = ProjectStore()
                await _store.init()
    return _store


@router.post("", response_model=ProjectMeta, status_code=201)
async def create_project(body: ProjectCreate):
    return await (await get_project_store()).create(name=body.name, description=body.description)


@router.get("", response_model=list[ProjectMeta])
async def list_projects():
    return await (await get_project_store()).list_all()


@router.get("/{project_id}", response_model=ProjectData)
async def get_project(project_id: str):
    store = await get_project_store()
    proj = await store.get(project_id)
    if proj is None:
        raise HTTPException(status_code=404, detail=f"Project '{project_id}' not found")
    return proj


@router.put("/{project_id}", response_model=ProjectMeta)
async def update_project(project_id: str, body: ProjectUpdate):
    store = await get_project_store()
    result = await store.update(
        project_id,
        name=body.name,
        description=body.description,
        circuit=body.circuit,
    )
    if result is None:
        raise HTTPException(status_code=404, detail=f"Project '{project_id}' not found")
    return result


@router.delete("/{project_id}", status_code=204)
async def delete_project(project_id: str):
    store = await get_project_store()
    deleted = await store.delete(project_id)
    if not deleted:
        raise HTTPException(status_code=404, detail=f"Project '{project_id}' not found")
