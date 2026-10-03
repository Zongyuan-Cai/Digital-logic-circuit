# Run with the same native CPython used to build logic_sim.pyd.
import os
from pathlib import Path

root = Path(SPECPATH).parent
module_dir = Path(os.environ.get("LOGIC_SIM_BUILD_DIR", root / "build" / "windows" / "python"))
a = Analysis(
    [str(root / "packaging" / "desktop.py")],
    pathex=[str(root), str(module_dir)],
    binaries=[],
    datas=[(str(root / "device-library"), "device-library"),
           (str(root / "frontend" / "dist"), "frontend/dist"),
           (os.environ["LOGIC_LAB_LICENSE_DIR"], "third-party-licenses")],
    hiddenimports=["logic_sim", "uvicorn.logging", "uvicorn.loops.asyncio", "uvicorn.protocols.http.h11_impl", "uvicorn.lifespan.on"],
    hookspath=[], runtime_hooks=[], excludes=["pytest", "httpx"], noarchive=False,
)
pyz = PYZ(a.pure)
exe = EXE(pyz, a.scripts, [], exclude_binaries=True, name="LogicLab", debug=False,
          bootloader_ignore_signals=False, strip=False, upx=False, console=False)
coll = COLLECT(exe, a.binaries, a.datas, strip=False, upx=False, name="LogicLab")
