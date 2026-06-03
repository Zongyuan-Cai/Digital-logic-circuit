#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_DIR/build/sim-core"

echo "============================================"
echo "Digital Logic Circuit Simulation — Full Test"
echo "============================================"

# Step 1: Configure and build C++ core + tests
echo ""
echo "[1/4] Configuring CMake..."
cmake -S "$PROJECT_DIR/sim-core" -B "$BUILD_DIR" \
    -DCMAKE_CXX_COMPILER=g++ \
    -Dpybind11_DIR="$(python -c 'import pybind11; print(pybind11.get_cmake_dir())' 2>/dev/null || echo '')" \
    2>&1 | tail -5

echo ""
echo "[2/4] Building C++ core and tests..."
cmake --build "$BUILD_DIR" 2>&1 | tail -5

# Step 2: Run C++ tests
echo ""
echo "[3/4] Running C++ unit tests..."
cd "$BUILD_DIR"
FAILED=0
TOTAL=0
for exe in test_gates test_flip_flops \
            test_chips_encoders test_chips_decoders \
            test_chips_mux test_chips_arithmetic \
            test_chips_registers test_chips_counters \
            test_simulator; do
    if [ -f "$exe" ]; then
        echo -n "  $exe: "
        OUTPUT=$(./$exe 2>&1)
        if echo "$OUTPUT" | grep -q "PASSED"; then
            PASSED=$(echo "$OUTPUT" | grep "PASSED" | grep -o '[0-9]\+')
            echo "PASSED ($PASSED tests)"
        elif echo "$OUTPUT" | grep -q "FAILED"; then
            FAILED_TESTS=$(echo "$OUTPUT" | grep "FAILED" | head -1)
            echo "$FAILED_TESTS"
            FAILED=1
        else
            echo "UNKNOWN RESULT"
            echo "$OUTPUT"
            FAILED=1
        fi
    else
        echo "  $exe: MISSING"
        FAILED=1
    fi
done

if [ $FAILED -ne 0 ]; then
    echo ""
    echo "C++ unit tests failed."
    exit 1
fi

# Step 3: Run Python binding smoke test
echo ""
echo "[4/4] Running Python binding smoke tests..."
cd "$PROJECT_DIR"
PYTHONPATH="$BUILD_DIR" python bindings/python/tests/test_python_binding.py 2>&1

echo ""
echo "============================================"
echo "All tests completed!"
echo "============================================"
