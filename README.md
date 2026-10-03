# 数字逻辑电路仿真系统 · Digital Logic Circuit Simulator

一个面向数字逻辑课程教学的交互式电路仿真平台。支持拖拽搭建电路、事件驱动仿真、数字波形分析，覆盖从基本门电路到 74 系列中规模芯片的全部教学内容。

An interactive circuit simulation platform for digital logic education. Build circuits by drag-and-drop, simulate with an event-driven engine, and analyze digital waveforms — covering everything from basic gates to 74-series MSI chips.

---

## 功能特性 · Features

- **拖拽式电路编辑** — 从器件面板拖拽门电路、触发器、芯片到 SVG 画布，点击引脚连线，所见即所得
- **事件驱动仿真引擎** — C++ 实现，4 值逻辑 (0/1/X/Z)，支持门延迟、多驱动冲突检测、反馈环路
- **数字示波器** — 多通道波形显示，支持缩放、滚动、回放游标，直观观察时序行为
- **实时信号着色** — 仿真结果直接渲染在画布上：绿色=1，灰色=0，红色=X，蓝色=Z
- **45 种器件** — 基本门电路、触发器/锁存器、74 系列芯片、I/O 交互器件
- **电路校验** — 自动检测重复 ID、未知器件、输出冲突、浮空输入
- **工程管理** — 保存/加载电路到 SQLite 数据库
- **编辑工作台** — 缩放与平移、网格吸附、撤销/重做、快捷键与窄屏侧栏
- **PNG 导出** — 一键导出电路图为图片

---

## 项目结构 · Project Structure

```
digital-logic-sim/
├── sim-core/                     # C++ 仿真核心 (事件驱动, 4值逻辑)
│   ├── include/logic_sim/        # 头文件: signal, pin, node, device, circuit, simulator, wave_recorder
│   │   └── devices/              # 器件头文件: gates, flip_flops, chips, io_devices
│   ├── src/                      # 核心实现 + 45 种器件 eval 函数
│   ├── tests/                    # GoogleTest 单元测试 (9 套件)
│   └── CMakeLists.txt
│
├── bindings/python/              # pybind11 C++ → Python 桥接
│   ├── logic_sim_pybind.cpp
│   └── tests/                    # Python smoke test
│
├── backend/                      # FastAPI 后端 (Python)
│   ├── app/
│   │   ├── main.py               # 应用入口 + CORS
│   │   ├── api/                  # REST 路由: devices, projects, simulations, circuits
│   │   ├── models/schemas.py     # Pydantic 数据模型 (18 类)
│   │   ├── services/             # 器件库加载, C++ 仿真服务, 电路校验
│   │   └── storage/              # SQLite 工程持久化
│   ├── tests/                    # pytest 测试 (62 tests)
│   └── requirements.txt
│
├── frontend/                     # React + TypeScript 前端
│   ├── src/
│   │   ├── App.tsx               # 根布局 (工具栏 + 双面板 + 画布 + 示波器)
│   │   ├── components/
│   │   │   ├── DevicePanel.tsx   # 可拖拽器件面板 (搜索/分类过滤/尺寸调节)
│   │   │   ├── CircuitCanvas.tsx # SVG 电路画布 (网格/拖放/连线/信号着色)
│   │   │   ├── Toolbar.tsx       # 工程/撤销/运行/复位/PNG/帮助
│   │   │   ├── PropertyPanel.tsx # 属性面板 (引脚表/信号值/参数编辑)
│   │   │   ├── Oscilloscope.tsx  # 数字示波器 (多通道/缩放/回放游标/时间轴)
│   │   │   └── ProjectDialog.tsx # 工程保存/打开/未保存提示
│   │   ├── lib/                 # 器件/几何/信号/示例/导出领域逻辑
│   │   ├── store/useStore.ts     # Zustand 状态 (电路/历史/工程/仿真/回放)
│   │   ├── api/client.ts         # Fetch API 客户端
│   │   ├── types/circuit.ts      # TypeScript 类型定义
│   │   └── test/                # Vitest 测试 (60 tests)
│   └── package.json
│
├── device-library/               # 器件元数据 (JSON)
│   ├── devices.json              # 索引 (45 器件)
│   ├── gates.json                # 7 基本门电路
│   ├── flip_flops.json           # 14 触发器/锁存器
│   ├── chips.json                # 18 74 系列芯片
│   └── io_devices.json           # 6 I/O 器件
│
├── scripts/test-all.sh           # 统一测试脚本 (C++ + Python 绑定)
├── scripts/test-all.ps1          # Windows 全栈构建与测试
├── scripts/windows-common.ps1    # Windows 依赖与进程管理
├── start.sh                      # 一键启动脚本 (后端 + 前端)
├── start.cmd / start.ps1          # 原生 Windows 启动/设置/停止/状态
├── docs/                         # 设计文档
└── data/projects.db              # SQLite 运行时数据库
```

---

## 已实现器件 · Implemented Devices (45)

### 基本门电路 · Gates (7)
`AND` `OR` `NOT` `NAND` `NOR` `XOR` `XNOR`

### 触发器/锁存器 · Flip-Flops & Latches (14)
`RS_LATCH_NAND` `RS_LATCH_NOR` `GATED_RS_LATCH` `D_LATCH`
`SYNC_RS_FF` `SYNC_D_FF` `SYNC_JK_FF` `SYNC_T_FF`
`MASTER_SLAVE_RS_FF` `MASTER_SLAVE_D_FF` `MASTER_SLAVE_JK_FF`
`EDGE_D_FF` `EDGE_JK_FF` `BLOCKING_JK_FF`

### 74 系列芯片 · 74-Series Chips (18)

| 类别 | 芯片 |
|------|------|
| 编码器 | 74LS148 (8-3 优先), 74LS147 (10-4 优先) |
| 译码器 | 74139 (2-4), 74LS138 (3-8), 74LS42 (4-10), 7448 (BCD-7段) |
| 选择器 | 74153 (双4选1), 74LS151 (8选1) |
| 运算器 | 7485 (4位比较器), 74280 (奇偶校验), 74283 (4位全加器) |
| 寄存器 | 74175 (4位D), 74LS195 (4位移位), 74LS194 (4位双向移位) |
| 计数器 | 74161 (同步4位二进制), 74163 (同步4位二进制), 74191 (同步可逆), 74160 (BCD) |

### I/O 器件 · I/O Devices (6)
`VCC` (高电平) `GND` (低电平) `SWITCH` (交互开关) `CLOCK` (可配置时钟)
`LED` (电平指示) `SEVEN_SEGMENT` (7 段数码管)

---

## 仿真架构 · Simulation Architecture

```
┌─────────────┐     HTTP/JSON      ┌─────────────┐    pybind11     ┌──────────────┐
│   React 前端  │ ◄──────────────► │  FastAPI 后端  │ ◄─────────────► │  C++ 仿真核心  │
│  SVG Canvas  │   REST API        │  校验/存储    │   logic_sim    │  事件驱动引擎  │
└─────────────┘                    └─────────────┘   module        └──────────────┘
```

**仿真引擎**采用事件驱动架构：

1. **初始化** — 多轮组合逻辑稳定 (最多 10 轮)，记录初始波形
2. **事件循环** — 从优先队列取出最早事件，传播信号变化到监听器件
3. **门延迟** — 每个器件可配置传播延迟，输出在 `当前时间 + 延迟` 排入队列
4. **冲突解决** — 多驱动同一节点时，冲突 → `X` (未知)
5. **终止条件** — 事件队列为空、达到最大 tick 数、或达到最大事件数

**4 值逻辑** (0, 1, X, Z) 贯穿始终：
- `0` / `1` — 正常高低电平
- `X` — 未知/冲突 (多驱动不一致、触发器非法输入)
- `Z` — 高阻态 (三态输出关闭、未连接输入)

---

## API 端点 · API Endpoints

| 方法 | 路径 | 说明 |
|------|------|------|
| `GET` | `/api/health` | 健康检查 + C++ 模块状态 |
| `GET` | `/api/devices` | 器件列表 (`?category=gate\|flip_flop\|chip\|io`) |
| `GET` | `/api/devices/index` | 器件库索引 (分类统计) |
| `GET` | `/api/devices/{type}` | 单个器件元数据 (引脚、参数) |
| `GET` | `/api/devices/types` | 所有器件类型标识 |
| `POST` | `/api/projects` | 创建工程 |
| `GET` | `/api/projects` | 工程列表 |
| `GET` | `/api/projects/{id}` | 获取工程 (含电路数据) |
| `PUT` | `/api/projects/{id}` | 更新工程 |
| `DELETE` | `/api/projects/{id}` | 删除工程 |
| `POST` | `/api/circuits/validate` | 电路校验 (返回错误/警告) |
| `POST` | `/api/simulations/run` | 运行仿真 (返回波形 + 节点值) |
| `POST` | `/api/simulations/validate` | 电路校验 (同 circuits) |
| `POST` | `/api/simulations/step` | 单步仿真 (开发中) |
| `POST` | `/api/simulations/reset` | 复位仿真状态 |

---

## 快速开始 · Quick Start

### 环境要求 · Requirements

- **OS**: Windows 10/11 x64 / Linux (Ubuntu 22.04+) / WSL2
- **C++**: Windows 使用 Visual Studio 2022 C++ Build Tools + Windows SDK；Linux 使用 GCC 11+
- **CMake**: Linux 3.18+；Windows 3.21+
- **Python**: 3.10+
- **Node.js**: 推荐 24 LTS；也支持 20.19+ 或 22.13+，与当前前端依赖匹配

### 原生 Windows 启动

安装 64 位 Python、Node.js、CMake，以及 Visual Studio 2022 的“使用 C++ 的桌面开发”工作负载（含 MSVC 与 Windows SDK）；运行 C++ 测试还需要 Git。将项目放到本地磁盘，例如 `C:\work\logic-lab`，然后在 PowerShell 或 CMD 执行：

```powershell
.\start.cmd                 # 首次自动创建虚拟环境、安装依赖、编译并启动
.\start.cmd -Setup          # 只准备环境与编译
.\start.cmd -Status         # 查看服务状态
.\start.cmd -Stop           # 停止此脚本启动的服务
.\start.cmd -Rebuild        # 服务停止后，重新编译并启动
.\start.cmd -Help
```

访问 `http://localhost:5173`。可通过 `-Python "C:\Python312\python.exe"` 指定解释器，通过 `-BackendPort 8001 -FrontendPort 5174` 更改端口，前端 API 代理自动使用对应后端端口。

Windows 使用 `.venv-windows/`、`build/windows/` 和 `.logs/windows/`；启动时会验证 C++ 模块可导入，停止操作只处理带有效进程身份记录的服务。当前 `\\wsl$\...` 工作区应复制到本地磁盘后原生运行，复制时排除 `build/`、`frontend/node_modules/`、虚拟环境和进程记录；继续使用 WSL 时仍运行 `./start.sh`。完整说明见 [Windows 使用指南](docs/Windows使用指南.md)。

### Linux / WSL 启动 · One-Click Start

```bash
./start.sh
```

脚本检查并安装后端和前端依赖，启动后端 (port 8000) 和前端 (port 5173)。C++ 核心需要先按下方步骤编译。
访问 `http://localhost:5173` 即可使用。

```bash
./start.sh --stop    # 停止所有服务
./start.sh --status  # 查看服务状态
./start.sh --help    # 查看帮助
```

### 手动启动 · Manual Start

#### 1. 编译 C++ 核心

```bash
# 安装 pybind11
pip install pybind11

# 配置编译
cmake -S sim-core -B build/sim-core \
    -DCMAKE_CXX_COMPILER=g++ \
    -Dpybind11_DIR="$(python3 -c 'import pybind11; print(pybind11.get_cmake_dir())')"
cmake --build build/sim-core -j$(nproc)
```

#### 2. 启动后端

```bash
cd backend
pip install -r requirements.txt
PYTHONPATH=../build/sim-core:. uvicorn app.main:app --reload --port 8000
```

#### 3. 启动前端

```bash
cd frontend
npm install
npm run dev
```

---

## 使用流程 · Workflow

1. **开始实验** — 新建工程，或加载首页的半加器示例
2. **放置器件** — 拖拽器件到画布，或点击器件卡片添加；窄屏通过“器件库”与“属性”打开侧栏
3. **连线与配置** — 依次点击两个引脚连线；选中器件配置参数，开关也可直接在画布切换
4. **编辑画布** — 滚轮缩放、空格加拖动平移，F 适应内容，Ctrl+Z 撤销
5. **运行仿真** — 点击“运行仿真”或 Ctrl+Enter，观察画布与属性面板电平
6. **分析波形** — 在属性面板将器件加入示波器；暂停、拖动时间滑块或点击波形查看特定时刻
7. **保存实验** — 点击“保存工程”或 Ctrl+S，后续从“打开”恢复；可导出当前画布为 PNG

---

## 运行测试 · Running Tests

### C++ 单元测试 (9 套件, ~83 tests)

```bash
ctest --test-dir build/sim-core --output-on-failure
```

### Python 后端测试 (62 tests)

```bash
cd backend
PYTHONPATH=../build/sim-core:. pytest tests/ -v
```

### Python 绑定 smoke 测试 (6 checks)

```bash
cd bindings/python
PYTHONPATH=../../build/sim-core python3 tests/test_python_binding.py
```

### 前端检查与测试 (60 tests)

```bash
cd frontend
npm run build
npm run lint
npx vitest run
```

### 统一脚本

```bash
./scripts/test-all.sh    # C++ + Python 绑定
```

Windows 下（先停止开发服务）：

```powershell
.\scripts\test-all.ps1  # C++、Python 绑定、后端、前端及启动脚本回归
```

执行策略阻止直接运行 `.ps1` 时，可使用单次进程设置：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\test-all.ps1
```

`.github/workflows/windows.yml` 在 Windows runner 上执行全栈测试，并验收启动、Vite API 代理、真实仿真与停止流程。

---

## 技术栈 · Tech Stack

| 层 · Layer | 技术 · Technology |
|------------|-------------------|
| 仿真核心 | C++17, 事件驱动, 4 值逻辑 (0/1/X/Z) |
| 构建系统 | CMake |
| C++ 测试 | GoogleTest |
| Python 绑定 | pybind11 |
| 后端框架 | FastAPI + Pydantic v2 |
| 后端测试 | pytest + pytest-asyncio + httpx |
| 数据存储 | SQLite (aiosqlite 风格, 线程池异步) |
| 前端框架 | React 19 + TypeScript |
| 构建工具 | Vite 8 |
| 状态管理 | Zustand 5 |
| 画布渲染 | SVG (原生, 拖拽/连线/信号着色) |
| 前端测试 | Vitest 4 + Testing Library + jsdom |
| 样式方案 | 统一 CSS 变量、深色实验工作台、响应式侧栏 |

---

## 设计文档 · Design Docs

- [项目梳理与前端重构](docs/项目梳理与前端重构.md) — 模块职责、数据链路、重构内容、操作方式与当前边界
- [Windows 使用指南](docs/Windows使用指南.md) — 原生 Windows 依赖、启动、测试、路径与故障排查
