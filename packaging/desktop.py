"""LogicLab desktop launcher: one local server, a small control window."""

from __future__ import annotations

import argparse
import json
import logging
import os
import secrets
import socket
import sys
import threading
import time
import urllib.request
import webbrowser
from pathlib import Path

from backend.app.runtime import VERSION, data_root, resource_root


def request(url: str, token: str = "") -> dict:
    opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))
    req = urllib.request.Request(url, headers={"X-LogicLab-Token": token}, method="POST" if token else "GET")
    with opener.open(req, timeout=2) as response:
        return json.load(response)


def read_running(state_path: Path) -> dict | None:
    try:
        state = json.loads(state_path.read_text(encoding="utf-8"))
        # Never contact arbitrary addresses from a stale state file.
        if state["url"] != f"http://127.0.0.1:{int(state['port'])}":
            return None
        status = request(state["url"] + "/_desktop/status")
        return state if status.get("instance") == state["instance"] else None
    except (OSError, ValueError, KeyError):
        return None


def acquire_lock(path: Path):
    handle = path.open("a+b")
    if handle.tell() == 0:
        handle.write(b"0")
        handle.flush()
    handle.seek(0)
    try:
        if sys.platform == "win32":
            import msvcrt
            msvcrt.locking(handle.fileno(), msvcrt.LK_NBLCK, 1)
        else:
            import fcntl
            fcntl.flock(handle, fcntl.LOCK_EX | fcntl.LOCK_NB)
        return handle
    except OSError:
        handle.close()
        return None


def control_window(url: str, server, thread: threading.Thread) -> None:
    import tkinter as tk
    from tkinter import ttk

    root = tk.Tk()
    root.title("LogicLab · 数字逻辑电路工作台")
    root.geometry("480x300")
    root.resizable(False, False)
    root.configure(background="#111827")
    style = ttk.Style(root)
    style.theme_use("clam")
    style.configure("TButton", font=("Microsoft YaHei UI", 11), padding=10)
    tk.Label(root, text="LogicLab", font=("Segoe UI", 28, "bold"), fg="#e5e7eb", bg="#111827").pack(pady=(24, 4))
    tk.Label(root, text="数字逻辑电路工作台", font=("Microsoft YaHei UI", 13), fg="#9ca3af", bg="#111827").pack()
    tk.Label(root, text=f"服务已启动 · {url}", font=("Segoe UI", 10), fg="#6ee7b7", bg="#111827").pack(pady=18)
    buttons = tk.Frame(root, bg="#111827")
    buttons.pack()
    ttk.Button(buttons, text="打开工作台", command=lambda: webbrowser.open(url)).pack(side="left", padx=6)

    def stop():
        server.should_exit = True

    ttk.Button(buttons, text="退出并停止服务", command=stop).pack(side="left", padx=6)
    tk.Label(root, text="关闭此窗口会停止服务，请先保存工程。", font=("Microsoft YaHei UI", 10), fg="#9ca3af", bg="#111827").pack(pady=20)
    root.protocol("WM_DELETE_WINDOW", stop)

    def poll():
        if thread.is_alive():
            root.after(200, poll)
        else:
            root.destroy()

    root.after(200, poll)
    root.mainloop()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--data-dir", type=Path)
    parser.add_argument("--headless", action="store_true", help="Run without a window or browser (automation).")
    parser.add_argument("--no-browser", action="store_true")
    parser.add_argument("--stop", action="store_true")
    args = parser.parse_args()
    if args.data_dir:
        os.environ["LOGIC_LAB_DATA_DIR"] = str(args.data_dir.resolve())
    folder = data_root()
    folder.mkdir(parents=True, exist_ok=True)
    state_path = folder / "runtime.json"
    if args.stop:
        running = read_running(state_path)
        if running:
            request(running["url"] + "/_desktop/stop", running["token"])
        return 0

    lock = acquire_lock(folder / "runtime.lock")
    if lock is None:
        for _ in range(100):
            running = read_running(state_path)
            if running:
                if not args.headless and not args.no_browser:
                    webbrowser.open(running["url"])
                return 0
            time.sleep(0.1)
        raise RuntimeError("Another LogicLab instance is starting. Check desktop.log.")

    log = (folder / "desktop.log").open("a", encoding="utf-8", buffering=1)
    # PyInstaller windowed applications have no stdout/stderr.
    if sys.stdout is None:
        sys.stdout = log
    if sys.stderr is None:
        sys.stderr = log
    logging.basicConfig(level=logging.INFO, stream=log, format="%(asctime)s %(levelname)s %(message)s", force=True)
    import uvicorn
    from fastapi import FastAPI, Header, HTTPException
    from fastapi.staticfiles import StaticFiles
    from starlette.middleware.trustedhost import TrustedHostMiddleware
    from backend.app.main import app as api
    from backend.app.services.simulator_service import SimulatorService

    if not SimulatorService.is_available():
        raise RuntimeError("Native simulation core could not be loaded.")
    frontend = resource_root() / "frontend" / "dist"
    if not (frontend / "index.html").is_file():
        raise RuntimeError("Built frontend is missing from the application bundle.")
    instance = secrets.token_hex(16)
    token = secrets.token_hex(32)
    desktop = FastAPI(docs_url=None, redoc_url=None)
    desktop.add_middleware(TrustedHostMiddleware, allowed_hosts=["127.0.0.1"])

    @desktop.get("/_desktop/status")
    async def status():
        return {"instance": instance, "version": VERSION}

    @desktop.post("/_desktop/stop")
    async def stop(x_logiclab_token: str = Header(default="")):
        if not secrets.compare_digest(x_logiclab_token, token):
            raise HTTPException(status_code=403)
        server.should_exit = True
        return {"status": "stopping"}

    # In source mode there is no static mount on the API app.
    if not getattr(sys, "frozen", False):
        api.mount("/", StaticFiles(directory=frontend, html=True), name="frontend")
    desktop.mount("/", api)
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.bind(("127.0.0.1", 0))
    sock.listen(128)
    port = sock.getsockname()[1]
    url = f"http://127.0.0.1:{port}"
    server = uvicorn.Server(uvicorn.Config(desktop, log_config=None, loop="asyncio", http="h11", ws="none", lifespan="on"))
    thread = threading.Thread(target=server.run, kwargs={"sockets": [sock]}, daemon=True)
    try:
        thread.start()
        deadline = time.monotonic() + 30
        while not server.started:
            if not thread.is_alive() or time.monotonic() > deadline:
                raise RuntimeError("Local service failed to start. Check desktop.log.")
            time.sleep(0.05)
        state_path.write_text(json.dumps({"pid": os.getpid(), "port": port, "url": url, "instance": instance, "token": token}), encoding="utf-8")
        if not args.headless and not args.no_browser:
            webbrowser.open(url)
        if args.headless:
            while thread.is_alive():
                thread.join(0.2)
        else:
            control_window(url, server, thread)
        return 0
    finally:
        server.should_exit = True
        thread.join(10)
        sock.close()
        if state_path.exists():
            try:
                if json.loads(state_path.read_text(encoding="utf-8")).get("instance") == instance:
                    state_path.unlink()
            except (OSError, ValueError):
                logging.exception("Could not remove runtime state")
        lock.close()


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception:
        import traceback
        error = traceback.format_exc()
        folder = data_root()
        folder.mkdir(parents=True, exist_ok=True)
        with (folder / "desktop.log").open("a", encoding="utf-8") as stream:
            stream.write(error)
        if sys.platform == "win32" and "--headless" not in sys.argv and "--stop" not in sys.argv:
            import ctypes
            ctypes.windll.user32.MessageBoxW(None, f"LogicLab 启动失败。\n日志：{folder / 'desktop.log'}", "LogicLab", 0x10)
        raise SystemExit(1)
