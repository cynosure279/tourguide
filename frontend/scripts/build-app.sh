#!/usr/bin/env bash
# 桌面版构建脚本：前端 → 后端内嵌版 → 侧车放到 Tauri 的 binaries/ 目录
set -eo pipefail
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
# 目标三元组探测：rustc > Tauri 环境变量 > uname 推断
TRIPLE="$(rustc -vV 2>/dev/null | sed -n 's/^host: //p')"
if [ -z "$TRIPLE" ]; then TRIPLE="${TAURI_ENV_TARGET_TRIPLE:-}"; fi
if [ -z "$TRIPLE" ]; then
  ARCH="$(uname -m)"
  case "$(uname -s)-$ARCH" in
    Darwin-arm64) TRIPLE="aarch64-apple-darwin" ;;
    Darwin-x86_64) TRIPLE="x86_64-apple-darwin" ;;
    Linux-x86_64) TRIPLE="x86_64-unknown-linux-gnu" ;;
    MINGW*|MSYS*|CYGWIN*)
      case "$ARCH" in
        x86_64) TRIPLE="x86_64-pc-windows-msvc" ;;
        aarch64) TRIPLE="aarch64-pc-windows-msvc" ;;
      esac ;;
  esac
fi
BIN=$(find "$ROOT/build-app" -name "tour-backend*" -type f ! -name "*.pdb" | head -1)
if [ -z "$BIN" ]; then
  echo "错误：未找到后端可执行文件（build-app 构建可能失败）"
  exit 1
fi
if [ -z "$TRIPLE" ]; then
  echo "错误：无法确定目标三元组（需 Rust 或 uname）"
  exit 1
fi
mkdir -p "$FRONTEND_DIR/src-tauri/binaries"
cp "$BIN" "$FRONTEND_DIR/src-tauri/binaries/tour-backend-$TRIPLE"
echo "侧车就绪：tour-backend-$TRIPLE（$BIN）"
