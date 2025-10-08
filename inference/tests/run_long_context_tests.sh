#!/bin/bash

# Script to build and run long context input tests
# Usage: ./run_long_context_tests.sh [model_path]

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
INFERENCE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$INFERENCE_DIR/build"
TEST_EXECUTABLE="$BUILD_DIR/bin/test_long_context"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "========================================"
echo "Long Context Input Test Runner"
echo "========================================"
echo ""

# Check if model path provided
if [ $# -lt 1 ]; then
    echo -e "${RED}Error: Model path required${NC}"
    echo "Usage: $0 <model.gguf>"
    echo ""
    echo "Example:"
    echo "  $0 /path/to/models/Qwen2.5-1.5B-Instruct-Q4_K_M.gguf"
    exit 1
fi

MODEL_PATH="$1"

# Verify model exists
if [ ! -f "$MODEL_PATH" ]; then
    echo -e "${RED}Error: Model file not found: $MODEL_PATH${NC}"
    exit 1
fi

echo -e "${GREEN}Model:${NC} $MODEL_PATH"
echo ""

# Build test if needed
if [ ! -f "$TEST_EXECUTABLE" ]; then
    echo -e "${YELLOW}Test executable not found. Building...${NC}"
    
    # Create build directory if needed
    mkdir -p "$BUILD_DIR"
    
    # Configure CMake
    echo "Configuring CMake..."
    cd "$BUILD_DIR"
    cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
    
    # Build the test
    echo "Building test_long_context..."
    make test_long_context -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
    
    echo -e "${GREEN}Build completed${NC}"
    echo ""
fi

# Verify test executable exists
if [ ! -f "$TEST_EXECUTABLE" ]; then
    echo -e "${RED}Error: Failed to build test executable${NC}"
    exit 1
fi

# Set library path for macOS
if [[ "$OSTYPE" == "darwin"* ]]; then
    LIB_DIR="$BUILD_DIR/Release"
    if [ -d "$LIB_DIR" ]; then
        export DYLD_LIBRARY_PATH="$LIB_DIR:${DYLD_LIBRARY_PATH:-}"
        echo -e "${YELLOW}Set DYLD_LIBRARY_PATH=$LIB_DIR${NC}"
        echo ""
    fi
fi

# Run the test
echo "Running long context input tests..."
echo "========================================"
echo ""

"$TEST_EXECUTABLE" "$MODEL_PATH"
TEST_EXIT_CODE=$?

echo ""
echo "========================================"

# Report results
if [ $TEST_EXIT_CODE -eq 0 ]; then
    echo -e "${GREEN}✅ ALL TESTS PASSED${NC}"
    exit 0
else
    echo -e "${RED}❌ SOME TESTS FAILED (exit code: $TEST_EXIT_CODE)${NC}"
    exit $TEST_EXIT_CODE
fi
