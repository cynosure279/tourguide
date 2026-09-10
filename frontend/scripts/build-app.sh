#!/usr/bin/env bash
# 桌面版构建脚本：前端 → 后端内嵌版 → 侧车放到 Tauri 的 binaries/ 目录
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FRONTEND_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
ROOT="$(cd "$FRONTEND_DIR/.." && pwd)"

echo "== 1/3 构建前端 =="
cd "$FRONTEND_DIR"
npm run build

echo "== 2/3 构建后端内嵌版（EMBED_FRONTEND）=="
cd "$ROOT"
cmake -S . -B build-app -DCMAKE_BUILD_TYPE=Release -DEMBED_FRONTEND=ON
cmake --build build-app

echo "== 3/3 复制侧车 =="
TRIPLE="$(rustc -vV 2>/dev/null | sed -n 's/^host: //p')"
if [ -z "$TRIPLE" ]; then echo "错误：找不到 rustc（请先安装 Rust）"; exit 1; fi
BIN=$(find "$ROOT/build-app" -name "tour-backend*" -type f ! -name "*.pdb" | head -1)
if [ -z "$BIN" ]; then
  echo "错误：未找到后端可执行文件（build-app 构建可能失败）"
  exit 1
fi
mkdir -p "$FRONTEND_DIR/src-tauri/binaries"
cp "$BIN" "$FRONTEND_DIR/src-tauri/binaries/tour-backend-$TRIPLE"
echo "侧车就绪：tour-backend-$TRIPLE（$BIN）"
