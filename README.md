# 数字逻辑电路仿真系统 Digital Logic Circuit Simulator

一个面向数字逻辑课程教学的电路仿真系统，支持拖拽搭建电路、事件驱动仿真、数字波形显示。

## 项目结构

```
digital-logic-sim/
├── sim-core/                 # C++ 仿真核心
│   ├── include/logic_sim/    # 头文件 (8 个)
│   │   └── devices/          # 器件头文件 (gates, flip_flops, chips)
│   ├── src/                  # 仿真核心实现
│   │   └── devices/          # 器件实现 (7 门, 14 触发器, 18 芯片)
│   ├── tests/                # GoogleTest 单元测试 (9 套件)
│   └── CMakeLists.txt        # CMake 构建配置
│
├── bindings/python/          # Python pybind11 绑定
│   ├── logic_sim_pybind.cpp  # C++ → Python 桥接
│   └── tests/                # Python smoke test
│
├── device-library/           # 器件元数据 JSON
│   ├── devices.json          # 索引 (39 器件)
│   ├── gates.json            # 7 基本门电路
│   ├── flip_flops.json       # 14 触发器/锁存器
│   └── chips.json            # 18 PDF 74 系列芯片
│
├── backend/                  # Python FastAPI 后端
│   ├── app/
│   │   ├── main.py           # FastAPI 入口
│   │   ├── api/              # API 路由 (devices, projects, simulations, circuits)
│   │   ├── models/schemas.py # Pydantic 数据模型
│   │   ├── services/         # 器件库、仿真、校验服务
│   │   └── storage/          # SQLite 工程存储
│   └── tests/                # 后端测试 (57 tests)
│
├── frontend/                 # React + TypeScript 前端
│   ├── src/
│   │   ├── components/       # UI 组件
│   │   │   ├── DevicePanel   # 器件面板 (拖拽器件到画布)
│   │   │   ├── CircuitCanvas # SVG 电路画布 (放置/连线/仿真信号着色)
│   │   │   ├── Toolbar       # 工具栏 (运行/保存/加载)
│   │   │   ├── PropertyPanel # 属性面板 (器件引脚/信号值)
│   │   │   └── Oscilloscope  # 数字示波器 (多通道波形/缩放)
│   │   ├── store/            # Zustand 状态管理
│   │   ├── api/              # 后端 API 客户端
│   │   └── types/            # TypeScript 类型定义
│   └── tests/                # Vitest 测试 (17 tests)
│
├── docs/                     # 设计文档
├── scripts/test-all.sh       # 统一测试脚本
└── README.md
```

## 已实现器件 (39 个)

### 基本门电路 (7)
AND, OR, NOT, NAND, NOR, XOR, XNOR

### 触发器/锁存器 (14)
RS_LATCH_NAND, RS_LATCH_NOR, GATED_RS_LATCH, D_LATCH,
SYNC_RS_FF, SYNC_D_FF, SYNC_JK_FF, SYNC_T_FF,
MASTER_SLAVE_RS_FF, MASTER_SLAVE_D_FF, MASTER_SLAVE_JK_FF,
EDGE_D_FF, EDGE_JK_FF, BLOCKING_JK_FF

### PDF 74 系列芯片 (18)
74LS148, 74LS147, 74139, 74LS138, 74LS42, 7448,
74153, 74LS151, 7485, 74280, 74283,
74175, 74LS195, 74LS194, 74161, 74163, 74191, 74160

## API 端点

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/health` | 健康检查 |
| GET | `/api/devices` | 器件列表 (?category=gate) |
| GET | `/api/devices/{type}` | 单个器件详情 |
| GET | `/api/devices/index` | 器件库索引 |
| POST | `/api/projects` | 创建工程 |
| GET | `/api/projects/{id}` | 获取工程 |
| PUT | `/api/projects/{id}` | 更新工程 |
| DELETE | `/api/projects/{id}` | 删除工程 |
| POST | `/api/circuits/validate` | 校验电路 |
| POST | `/api/simulations/run` | 运行仿真 |
| POST | `/api/simulations/validate` | 校验电路 (同 circuits) |
| POST | `/api/simulations/step` | 单步仿真 |
| POST | `/api/simulations/reset` | 复位仿真 |

## 快速开始

### 环境要求

- Linux (Ubuntu 22.04+)
- GCC 11+ / CMake 3.16+
- Python 3.10+
- Node.js 18+

### 1. 编译 C++ 核心和 Python 绑定

```bash
# 安装依赖
pip install pybind11

# 配置和编译
cmake -S sim-core -B build/sim-core \
    -DCMAKE_CXX_COMPILER=g++ \
    -Dpybind11_DIR="$(python3 -c 'import pybind11; print(pybind11.get_cmake_dir())')"
cmake --build build/sim-core -j$(nproc)

# 运行 C++ 测试
ctest --test-dir build/sim-core --output-on-failure
```

### 2. 启动后端

```bash
pip install fastapi uvicorn httpx
cd backend
PYTHONPATH=../build/sim-core:. uvicorn app.main:app --reload --port 8000
```

### 3. 启动前端

```bash
cd frontend
npm install
npm run dev
```

访问 `http://localhost:5173` 即可使用。

## 运行测试

### 全部测试

```bash
# C++ 单元测试 (9 套件)
ctest --test-dir build/sim-core --output-on-failure

# Python 后端测试 (57 tests)
cd backend
PYTHONPATH=../build/sim-core:. pytest tests/ -v

# 前端测试 (17 tests)
cd frontend
npx vitest run

# 总计: 83 tests
```

### 或使用统一脚本

```bash
./scripts/test-all.sh
```

## 技术栈

| 层 | 技术 |
|----|------|
| 仿真核心 | C++17, 事件驱动, 4 值逻辑 (0/1/X/Z) |
| 构建 | CMake + Ninja/Make |
| C++ 测试 | GoogleTest |
| Python 绑定 | pybind11 |
| 后端 | FastAPI + Pydantic + SQLite |
| 后端测试 | pytest + pytest-asyncio + httpx |
| 前端 | React 18 + TypeScript + Vite |
| 状态管理 | Zustand |
| 画布 | SVG (拖拽/连线/信号着色) |
| 前端测试 | Vitest + Testing Library |

## 设计文档

- [完整设计文档](docs/数字逻辑电路仿真系统完整设计文档.md)
- [门电路与芯片实现说明](docs/门电路与芯片实现说明.md)
- [后端架构与技术说明](docs/后端架构与技术说明.md)
