# 原生 Windows 使用指南

项目支持 Windows 10/11 x64 原生运行。React 前端与 FastAPI 后端直接使用 Windows 的 Node.js/Python，C++ 仿真核心由 MSVC 编译为 `logic_sim*.pyd`。Linux 与 WSL 保留原有构建和运行方式。

## 环境准备

| 工具 | 要求 |
| --- | --- |
| Python | 官方 64 位 CPython 3.10+，推荐 3.12；需要正常安装，WindowsApps 中的商店执行别名不能代替解释器 |
| Node.js | 推荐 24 LTS；支持 20.19+、22.13+ 或 24+。当前 ESLint 等依赖要求比 Vite 的最低要求更严格 |
| CMake | 3.21+，添加到 PATH；脚本也可从 Visual Studio 安装目录发现内置 CMake |
| Visual Studio | Visual Studio 2022 或 Build Tools 2022，安装“使用 C++ 的桌面开发”，包含 MSVC v143、Windows SDK；内置 CMake 工具可选 |
| Git | 测试构建需要 Git for Windows 获取 GoogleTest；仅启动无需 Git |
| PowerShell | 系统自带 Windows PowerShell 5.1，或 PowerShell 7 |

本项目的 Vite 版本要求 Node.js 20.19+ 或 22.12+，实际启动脚本按完整依赖集合检查版本，推荐使用 24 LTS。[Vite 8 官方环境说明](https://v8.vite.dev/guide/)

把项目克隆或复制到本地 Windows 磁盘，例如 `C:\work\数字逻辑电路`。脚本支持目录中的空格和中文。不要直接在 `\\wsl$\...` 或网络 UNC 目录运行原生构建，以免 CMD/npm 的工作目录退回 Windows 系统目录，也避免覆盖 WSL 的依赖。

从 WSL 复制时排除 `build/`、`frontend/node_modules/`、`frontend/dist/`、虚拟环境、`.pids/`、`.logs/` 和 Python 缓存。`data/projects.db` 是已有工程库，需要保留已有工程时一起复制。Windows 原生与 WSL 应各用一份工作区；两者的二进制扩展和 npm 原生依赖不能混用。

## 一键启动

在项目目录运行：

```powershell
.\start.cmd
```

首次运行会创建 `.venv-windows`，安装后端与 pybind11 依赖，使用 CMake/MSVC 编译核心，安装 Windows 的 npm 依赖，再启动后端和前端。后续运行复用依赖和增量构建。无需激活虚拟环境。

| 操作 | 命令 |
| --- | --- |
| 仅准备依赖与编译 | `.\start.cmd -Setup` |
| 查看状态 | `.\start.cmd -Status` |
| 停止脚本管理的服务 | `.\start.cmd -Stop` |
| 服务停止后重新编译并启动 | `.\start.cmd -Rebuild` |
| 指定 Python | `.\start.cmd -Python "C:\Python312\python.exe"` |
| 自定义端口 | `.\start.cmd -BackendPort 8001 -FrontendPort 5174` |
| 查看帮助 | `.\start.cmd -Help` |

默认前端为 `http://localhost:5173`，后端 API 文档为 `http://localhost:8000/docs`。默认只监听本机。自定义后端端口由 `LOGIC_LAB_API_URL` 传给 Vite 代理，浏览器仍使用相对 `/api` 地址。

CMD 入口只为本次 PowerShell 进程设置执行策略，不更改系统配置。也可以直接使用 `.\start.ps1`；系统阻止脚本时使用 `start.cmd`。

两个受管理服务都在运行时，再次启动只检查健康状态；存在部分启动、需要重编译或修改端口时先执行 `-Stop`。脚本不会按端口终止无关程序，也不会停止原有 WSL 服务。Windows 开发后端不启用 Uvicorn 自动重载；修改 Python 后使用 `-Stop` 再启动。Vite 保留热更新。

## 目录与编译约定

| 路径 | 用途 |
| --- | --- |
| `.venv-windows/` | Windows 专用 Python 依赖 |
| `build/windows/` | CMake 工程、MSVC Release 构建与测试 |
| `build/windows/python/` | 所有 CMake 配置的 Python 扩展输出；运行脚本使用 Release |
| `.logs/windows/*.log` | 前后端标准输出 |
| `.logs/windows/*.error.log` | 前后端错误输出 |
| `.pids/windows/*.json` | PID、启动时间、可执行文件和端口记录 |
| `data/projects.db` | SQLite 工程库，与 Linux 使用相同数据格式 |

CMake 为 MSVC 使用 `/W4`、`/utf-8`，为 GCC/Clang 保留 `-Wall -Wextra`，通过 `POSITION_INDEPENDENT_CODE` 配置静态库；GoogleTest 与核心统一使用 MSVC 动态运行库。`BUILD_TESTING=OFF` 可仅构建运行所需的核心与绑定，避免启动时拉取测试依赖。

绑定使用明确的虚拟环境解释器与现代 FindPython。脚本要求扩展能够实际导入，健康检查还必须返回 `sim_core_available=true`，避免出现界面可打开却无法仿真的假启动。[pybind11 CMake 官方说明](https://pybind11.readthedocs.io/en/stable/cmake/)

切换 Python 安装位置、主/次版本或 CMake 生成器时，请在服务停止后重新建立 `.venv-windows/` 与 `build/windows/`，不要复用旧 ABI 或 CMake 缓存。上述两个目录都是生成物，不包含工程数据。

## 验证与持续集成

停止开发服务后执行完整测试：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\test-all.ps1
```

该脚本准备环境并构建测试，依次运行 C++ CTest、Python 绑定 smoke、后端 pytest、前端构建/Lint/Vitest 和 Windows 启动脚本回归。Python 绑定在 smoke 前强制检查导入，缺失模块不会被当成成功。

仅验证启动脚本（需要 Node.js，不需要 Python/MSVC）：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\test-windows.ps1
```

Windows CI 使用 `.github/workflows/windows.yml`，运行上述完整流程，并通过 Vite 代理进行真实 C++ 仿真、检查重复启动与停止。CI 失败时保留进程日志。

本次本机验证：Windows PowerShell 5.1 启动脚本检查通过，涵盖带空格和中文路径/参数、引号/尾部反斜线/空参数、命令失败退出码、绕过代理的本机健康检查、有效进程停止、失效 PID 拒绝停止和端口冲突；Windows Node.js 下前端构建、Lint 和 60 项测试通过。Windows Vite 已实际启动，并通过自定义 API 代理调用现有 WSL 后端获取器件与运行仿真，随后由进程管理脚本停止。Linux 重新构建后 9 个 C++ 套件、Python 绑定 smoke 和 62 项后端测试通过。本机没有 MSVC C++ 组件，原生 `.pyd` 编译及 Windows 全栈启动尚未在本机执行，需由配置好编译工具的机器或新增 Windows CI 验证。

## 常见问题

- **提示本地磁盘要求**：把当前 `\\wsl$\Ubuntu\...` 项目复制到 `C:\work\...` 后执行 `start.cmd`；继续使用 WSL 则在 Ubuntu 运行 `./start.sh`。
- **CMake 找不到编译器/生成器**：安装 Visual Studio 2022 的 C++ 工作负载和 Windows SDK，再重新打开终端。仅安装 C#/.NET 工作负载不包含 MSVC。
- **Python 不存在**：使用官方 64 位 Python，或通过 `-Python` 指定真实 `python.exe`，不要使用 WindowsApps 的执行别名。
- **端口被占用**：`-Status` 会标明外部占用；停止原来的服务，或同时指定不同的后端/前端端口。
- **扩展导入或后端启动失败**：查看 `.logs/windows/backend.error.log`；确认 Python x64 与 MSVC x64 一致，虚拟环境与扩展由同一 Python 版本构建。
- **前端依赖来自其他平台**：在 Windows 工作区执行 `npm ci`；不要复用 WSL 的 `node_modules`。
