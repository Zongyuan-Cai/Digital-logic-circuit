"""Project storage using SQLite."""

from __future__ import annotations

import json
import os
import sqlite3
import uuid
from datetime import datetime, timezone
from typing import Optional

from ..models.schemas import CircuitDef, ProjectData, ProjectMeta


class ProjectStore:
    """Async SQLite-based project storage."""

    def __init__(self, db_path: Optional[str] = None):
        if db_path is None:
            db_path = os.path.join(
                os.path.dirname(__file__), "..", "..", "..", "data", "projects.db"
            )
        os.makedirs(os.path.dirname(db_path), exist_ok=True)
        self._db_path = db_path

    async def init(self) -> None:
        with sqlite3.connect(self._db_path) as db:
            db.execute("""
                CREATE TABLE IF NOT EXISTS projects (
                    id TEXT PRIMARY KEY,
                    name TEXT NOT NULL,
                    description TEXT DEFAULT '',
                    circuit_json TEXT DEFAULT '{}',
                    created_at TEXT NOT NULL,
                    updated_at TEXT NOT NULL
                )
            """)
            db.commit()

    async def create(self, name: str, description: str = "") -> ProjectMeta:
        pid = uuid.uuid4().hex[:12]
        now = datetime.now(timezone.utc).isoformat()
        with sqlite3.connect(self._db_path) as db:
            db.execute(
                "INSERT INTO projects (id, name, description, circuit_json, created_at, updated_at) "
                "VALUES (?, ?, ?, '{}', ?, ?)",
                (pid, name, description, now, now),
            )
            db.commit()
        return ProjectMeta(id=pid, name=name, description=description,
                           created_at=datetime.fromisoformat(now),
                           updated_at=datetime.fromisoformat(now))

    async def list_all(self) -> list[ProjectMeta]:
        with sqlite3.connect(self._db_path) as db:
            rows = db.execute(
                "SELECT id, name, description, created_at, updated_at FROM projects ORDER BY updated_at DESC"
            ).fetchall()
        return [
            ProjectMeta(
                id=row[0], name=row[1], description=row[2],
                created_at=datetime.fromisoformat(row[3]),
                updated_at=datetime.fromisoformat(row[4]),
            )
            for row in rows
        ]

    async def get(self, project_id: str) -> Optional[ProjectData]:
        with sqlite3.connect(self._db_path) as db:
            row = db.execute(
                "SELECT id, name, description, circuit_json, created_at, updated_at "
                "FROM projects WHERE id = ?", (project_id,)
            ).fetchall()
        if not row:
            return None
        r = row[0]
        circuit_raw = json.loads(r[3]) if r[3] else {}
        circuit = CircuitDef(**circuit_raw) if circuit_raw else CircuitDef()
        return ProjectData(
            id=r[0], name=r[1], description=r[2],
            circuit=circuit,
            created_at=datetime.fromisoformat(r[4]),
            updated_at=datetime.fromisoformat(r[5]),
        )

    async def update(
        self,
        project_id: str,
        name: Optional[str] = None,
        description: Optional[str] = None,
        circuit: Optional[CircuitDef] = None,
    ) -> Optional[ProjectMeta]:
        existing = await self.get(project_id)
        if existing is None:
            return None

        new_name = name if name is not None else existing.name
        new_desc = description if description is not None else existing.description
        new_circuit = circuit if circuit is not None else existing.circuit
        now = datetime.now(timezone.utc).isoformat()

        with sqlite3.connect(self._db_path) as db:
            db.execute(
                "UPDATE projects SET name=?, description=?, circuit_json=?, updated_at=? WHERE id=?",
                (new_name, new_desc, new_circuit.model_dump_json(), now, project_id),
            )
            db.commit()
        return ProjectMeta(
            id=project_id, name=new_name, description=new_desc,
            created_at=existing.created_at,
            updated_at=datetime.fromisoformat(now),
        )

    async def delete(self, project_id: str) -> bool:
        with sqlite3.connect(self._db_path) as db:
            cursor = db.execute("DELETE FROM projects WHERE id=?", (project_id,))
            db.commit()
            return cursor.rowcount > 0
