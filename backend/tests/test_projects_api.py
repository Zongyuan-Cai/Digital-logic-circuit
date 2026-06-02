"""Tests for /api/projects CRUD endpoints."""

import pytest


class TestProjectsAPI:
    """HTTP endpoint tests for project CRUD."""

    @pytest.mark.anyio
    async def test_create_project(self, async_client):
        """POST /api/projects should create a new project."""
        response = await async_client.post("/api/projects", json={
            "name": "Test Project",
            "description": "A test circuit project",
        })
        assert response.status_code == 201
        data = response.json()
        assert data["name"] == "Test Project"
        assert data["description"] == "A test circuit project"
        assert "id" in data
        assert "created_at" in data

    @pytest.mark.anyio
    async def test_list_projects(self, async_client):
        """GET /api/projects should list all projects."""
        # Create a project first
        await async_client.post("/api/projects", json={
            "name": "List Test", "description": ""
        })
        response = await async_client.get("/api/projects")
        assert response.status_code == 200
        data = response.json()
        assert isinstance(data, list)
        assert len(data) >= 1
        assert all("id" in p for p in data)

    @pytest.mark.anyio
    async def test_get_project(self, async_client):
        """GET /api/projects/{id} should return a single project."""
        create_resp = await async_client.post("/api/projects", json={
            "name": "Get Test", "description": "Desc"
        })
        pid = create_resp.json()["id"]

        response = await async_client.get(f"/api/projects/{pid}")
        assert response.status_code == 200
        data = response.json()
        assert data["name"] == "Get Test"
        assert "circuit" in data

    @pytest.mark.anyio
    async def test_get_nonexistent_project(self, async_client):
        """GET /api/projects/{id} with invalid id should return 404."""
        response = await async_client.get("/api/projects/nonexistent-id")
        assert response.status_code == 404

    @pytest.mark.anyio
    async def test_update_project(self, async_client):
        """PUT /api/projects/{id} should update project metadata."""
        create_resp = await async_client.post("/api/projects", json={
            "name": "Old Name", "description": "Old Desc"
        })
        pid = create_resp.json()["id"]

        response = await async_client.put(f"/api/projects/{pid}", json={
            "name": "New Name",
        })
        assert response.status_code == 200
        data = response.json()
        assert data["name"] == "New Name"
        assert data["description"] == "Old Desc"  # unchanged

    @pytest.mark.anyio
    async def test_update_nonexistent_project(self, async_client):
        """PUT /api/projects/{id} with invalid id should return 404."""
        response = await async_client.put("/api/projects/nonexistent", json={
            "name": "X",
        })
        assert response.status_code == 404

    @pytest.mark.anyio
    async def test_delete_project(self, async_client):
        """DELETE /api/projects/{id} should remove the project."""
        create_resp = await async_client.post("/api/projects", json={
            "name": "Delete Me", "description": ""
        })
        pid = create_resp.json()["id"]

        response = await async_client.delete(f"/api/projects/{pid}")
        assert response.status_code == 204

        # Verify it's gone
        get_resp = await async_client.get(f"/api/projects/{pid}")
        assert get_resp.status_code == 404

    @pytest.mark.anyio
    async def test_delete_nonexistent_project(self, async_client):
        """DELETE /api/projects/{id} with invalid id should return 404."""
        response = await async_client.delete("/api/projects/nonexistent")
        assert response.status_code == 404

    @pytest.mark.anyio
    async def test_update_project_with_circuit(self, async_client):
        """PUT /api/projects/{id} should save circuit data."""
        create_resp = await async_client.post("/api/projects", json={
            "name": "CircuitTest", "description": ""
        })
        pid = create_resp.json()["id"]

        circuit = {
            "devices": [
                {"id": "u1", "type": "AND", "x": 100, "y": 100, "params": {"delay": 1}},
                {"id": "u2", "type": "NOT", "x": 200, "y": 100, "params": {"delay": 1}},
            ],
            "wires": [
                {"id": "w1", "from": {"device": "u1", "pin": "Y"}, "to": {"device": "u2", "pin": "I"}},
            ],
        }

        response = await async_client.put(f"/api/projects/{pid}", json={
            "circuit": circuit,
        })
        assert response.status_code == 200

        # Fetch back and verify
        get_resp = await async_client.get(f"/api/projects/{pid}")
        data = get_resp.json()
        assert data["circuit"]["devices"][0]["id"] == "u1"
        assert data["circuit"]["devices"][1]["type"] == "NOT"
        assert len(data["circuit"]["wires"]) == 1

    @pytest.mark.anyio
    async def test_create_returns_201(self, async_client):
        """Creating multiple projects should each return unique IDs."""
        ids: set[str] = set()
        for i in range(3):
            resp = await async_client.post("/api/projects", json={
                "name": f"Project {i}",
            })
            assert resp.status_code == 201
            ids.add(resp.json()["id"])
        assert len(ids) == 3, "All project IDs should be unique"
