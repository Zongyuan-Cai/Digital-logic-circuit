#!/usr/bin/env bash
#
# 一键启动脚本 — 同时启动前后端开发服务器
# One-click startup script for Digital Logic Circuit Simulator
#
# Usage:
#   ./start.sh              # 启动前后端
#   ./start.sh --stop       # 停止所有服务
#   ./start.sh --status     # 查看服务状态
#
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BACKEND_DIR="$PROJECT_DIR/backend"
FRONTEND_DIR="$PROJECT_DIR/frontend"
PID_DIR="$PROJECT_DIR/.pids"

BACKEND_PORT=8000
FRONTEND_PORT=5173

# ── 颜色 ──────────────────────────────────────────────
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# ── 工具函数 ──────────────────────────────────────────
log_info()  { echo -e "${GREEN}[INFO]${NC}  $*"; }
log_warn()  { echo -e "${YELLOW}[WARN]${NC}  $*"; }
log_error() { echo -e "${RED}[ERROR]${NC} $*"; }
log_step()  { echo -e "${BLUE}[STEP]${NC}  $*"; }

# ── 清理旧进程 ────────────────────────────────────────
stop_services() {
    log_step "Stopping all services..."

    # 按 pid 文件杀
    if [ -d "$PID_DIR" ]; then
        for f in "$PID_DIR"/*.pid; do
            if [ -f "$f" ]; then
                local name; name="$(basename "$f" .pid)"
                local pid; pid="$(cat "$f")"
                if kill -0 "$pid" 2>/dev/null; then
                    kill "$pid" 2>/dev/null || true
                    log_info "Stopped $name (pid=$pid)"
                fi
                rm -f "$f"
            fi
        done
    fi

    # 兜底：按端口杀
    for port in $BACKEND_PORT $FRONTEND_PORT; do
        local pids; pids=$(lsof -ti ":$port" 2>/dev/null || true)
        if [ -n "$pids" ]; then
            echo "$pids" | xargs kill 2>/dev/null || true
            log_info "Killed process on port $port"
        fi
    done

    rm -rf "$PID_DIR"
    log_info "All services stopped."
}

# ── 状态检查 ──────────────────────────────────────────
check_status() {
    echo "========================================"
    echo " Service Status"
    echo "========================================"

    # 后端
    if lsof -ti ":$BACKEND_PORT" >/dev/null 2>&1; then
        echo -e "  Backend  (port $BACKEND_PORT) : ${GREEN}RUNNING${NC}"
    else
        echo -e "  Backend  (port $BACKEND_PORT) : ${RED}STOPPED${NC}"
    fi

    # 前端
    if lsof -ti ":$FRONTEND_PORT" >/dev/null 2>&1; then
        echo -e "  Frontend (port $FRONTEND_PORT): ${GREEN}RUNNING${NC}"
    else
        echo -e "  Frontend (port $FRONTEND_PORT): ${RED}STOPPED${NC}"
    fi

    # 健康检查
    if command -v curl &>/dev/null; then
        local health; health=$(curl -s --noproxy '*' http://127.0.0.1:$BACKEND_PORT/api/health 2>/dev/null || echo "")
        if [ -n "$health" ]; then
            echo -e "  API Health: $health"
        fi
    fi

    echo "========================================"
}

# ── 启动后端 ──────────────────────────────────────────
start_backend() {
    log_step "Starting backend (FastAPI)..."

    cd "$BACKEND_DIR"

    # 检查依赖
    if ! python3 -c "import fastapi, uvicorn" 2>/dev/null; then
        log_warn "Installing backend dependencies..."
        pip3 install -r requirements.txt --quiet
    fi

    nohup python3 -m uvicorn app.main:app \
        --reload \
        --host 0.0.0.0 \
        --port $BACKEND_PORT \
        > "$PROJECT_DIR/.backend.log" 2>&1 &

    local pid=$!
    mkdir -p "$PID_DIR"
    echo "$pid" > "$PID_DIR/backend.pid"

    # 等待后端就绪
    log_info "Waiting for backend to be ready..."
    local retries=0
    while [ $retries -lt 30 ]; do
        if curl -s --noproxy '*' "http://127.0.0.1:$BACKEND_PORT/api/health" >/dev/null 2>&1; then
            log_info "Backend is ready! (http://localhost:$BACKEND_PORT)"
            return 0
        fi
        sleep 1
        retries=$((retries + 1))
    done

    log_error "Backend failed to start. Check .backend.log"
    return 1
}

# ── 启动前端 ──────────────────────────────────────────
start_frontend() {
    log_step "Starting frontend (Vite + React)..."

    cd "$FRONTEND_DIR"

    # 检查 node_modules
    if [ ! -d "node_modules" ]; then
        log_warn "Installing frontend dependencies..."
        npm install --silent
    fi

    nohup npm run dev -- --host 0.0.0.0 --port $FRONTEND_PORT \
        > "$PROJECT_DIR/.frontend.log" 2>&1 &

    local pid=$!
    mkdir -p "$PID_DIR"
    echo "$pid" > "$PID_DIR/frontend.pid"

    # 等待前端就绪
    log_info "Waiting for frontend to be ready..."
    local retries=0
    while [ $retries -lt 30 ]; do
        if curl -s --noproxy '*' "http://127.0.0.1:$FRONTEND_PORT" >/dev/null 2>&1; then
            log_info "Frontend is ready! (http://localhost:$FRONTEND_PORT)"
            return 0
        fi
        sleep 1
        retries=$((retries + 1))
    done

    log_error "Frontend failed to start. Check .frontend.log"
    return 1
}

# ── 打印信息 ──────────────────────────────────────────
print_banner() {
    echo ""
    echo "========================================"
    echo -e "  ${GREEN}Digital Logic Circuit Simulator${NC}"
    echo "========================================"
    echo ""
    echo -e "  Frontend : ${BLUE}http://localhost:$FRONTEND_PORT${NC}"
    echo -e "  Backend  : ${BLUE}http://localhost:$BACKEND_PORT${NC}"
    echo -e "  API Docs : ${BLUE}http://localhost:$BACKEND_PORT/docs${NC}"
    echo ""
    echo -e "  Logs:"
    echo -e "    backend  → tail -f .backend.log"
    echo -e "    frontend → tail -f .frontend.log"
    echo ""
    echo "  Stop services: ./start.sh --stop"
    echo "========================================"
}

# ── 主入口 ────────────────────────────────────────────
main() {
    cd "$PROJECT_DIR"

    case "${1:-}" in
        --stop)
            stop_services
            ;;
        --status)
            check_status
            ;;
        --help|-h)
            echo "Usage: ./start.sh [--stop|--status|--help]"
            echo ""
            echo "  (no args)   Start backend + frontend"
            echo "  --stop      Stop all services"
            echo "  --status    Show service status"
            echo "  --help      Show this help"
            ;;
        *)
            # 先检查是否已经在运行
            if lsof -ti ":$BACKEND_PORT" >/dev/null 2>&1 || \
               lsof -ti ":$FRONTEND_PORT" >/dev/null 2>&1; then
                log_warn "Some services are already running. Use --stop first, or:"
                check_status
                echo ""
                read -rp "Restart anyway? [y/N] " ans
                if [ "${ans,,}" != "y" ]; then
                    exit 0
                fi
                stop_services
                sleep 1
            fi

            start_backend &
            local be_pid=$!

            start_frontend &
            local fe_pid=$!

            wait $be_pid $fe_pid || true

            # 检查最终状态
            sleep 1
            if lsof -ti ":$BACKEND_PORT" >/dev/null 2>&1 && \
               lsof -ti ":$FRONTEND_PORT" >/dev/null 2>&1; then
                print_banner
            else
                log_error "One or more services failed to start."
                check_status
                exit 1
            fi
            ;;
    esac
}

main "$@"
