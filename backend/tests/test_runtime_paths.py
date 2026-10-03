"""Packaged resources are read-only; user projects use a separate data root."""

import sys
import pytest

from backend.app.runtime import data_root, resource_root
from backend.app.storage.project_store import ProjectStore


def test_source_paths(monkeypatch):
    monkeypatch.delenv("LOGIC_LAB_DATA_DIR", raising=False)
    assert (resource_root() / "device-library" / "devices.json").is_file()
    assert data_root() == resource_root() / "data"


def test_frozen_paths(monkeypatch, tmp_path):
    resources = tmp_path / "bundle"
    local = tmp_path / "user"
    monkeypatch.setattr(sys, "frozen", True, raising=False)
    monkeypatch.setattr(sys, "_MEIPASS", str(resources), raising=False)
    monkeypatch.setenv("LOCALAPPDATA", str(local))
    monkeypatch.delenv("LOGIC_LAB_DATA_DIR", raising=False)
    assert resource_root() == resources
    assert data_root() == local / "LogicLab"


@pytest.mark.asyncio
async def test_projects_use_explicit_user_data(monkeypatch, tmp_path):
    folder = tmp_path / "user data"
    monkeypatch.setenv("LOGIC_LAB_DATA_DIR", str(folder))
    store = ProjectStore()
    await store.init()
    project = await store.create("Packaged project")
    assert (folder / "projects.db").is_file()
    assert (await store.get(project.id)).name == "Packaged project"
