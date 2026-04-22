#!/bin/bash
# Build script - Cyberpunk 2077 RT Init Fix
# Nécessite: x86_64-w64-mingw32-g++ (apt install mingw-w64)

set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
OUT_DIR="$SCRIPT_DIR/../files"
BUILD_DIR="/tmp/dxgi_proxy"

echo "=== Build Cyberpunk 2077 RT Init Fix ==="
echo "Source: $SCRIPT_DIR"
echo "Output: $OUT_DIR"
echo ""

mkdir -p "$OUT_DIR"

# Compiler sl.interposer.dll (proxy principal)
echo "[1/3] Compilation sl.interposer.dll..."
cd "$BUILD_DIR"
x86_64-w64-mingw32-g++ \
    -shared -o "$OUT_DIR/sl.interposer.dll" \
    sl_interposer_proxy4.cpp sl_all.def \
    -static -std=c++17 -DUNICODE -D_UNICODE \
    -ldxgi -lole32 -luuid \
    -Wl,--kill-at 2>&1
echo "  -> OK"

# nvapi64.dll stub
echo "[2/3] Compilation nvapi64.dll..."
x86_64-w64-mingw32-g++ \
    -shared -o "$OUT_DIR/nvapi64.dll" \
    nvapi64_stub.cpp \
    -static -std=c++17 \
    -Wl,--kill-at 2>&1
echo "  -> OK"

# GFSDK_Aftermath stub (si présent)
if [ -f "$BUILD_DIR/gfsdk_stub.cpp" ]; then
    echo "[3/3] Compilation GFSDK_Aftermath_Lib.x64.dll..."
    x86_64-w64-mingw32-g++ \
        -shared -o "$OUT_DIR/GFSDK_Aftermath_Lib.x64.dll" \
        gfsdk_stub.cpp \
        -static -std=c++17 \
        -Wl,--kill-at 2>&1
    echo "  -> OK"
else
    echo "[3/3] GFSDK stub source absent — copie depuis le jeu ou build manuel"
fi

echo ""
echo "=== Build terminé. Fichiers dans $OUT_DIR ==="
ls -lh "$OUT_DIR"/*.dll
