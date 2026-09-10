#!/usr/bin/env python3
"""把前端构建产物与默认数据内嵌为 C++ 头文件。
EMBED_FRONTEND=ON 时由 CMake 调用：tour-backend 自解压后即可独立运行，
无需随包分发 frontend/dist 与数据文件。

注意：二进制数据以 unsigned char 数组 + 显式长度内嵌，
不能用 std::string 字面量——string 遇到 \\000 会截断（PNG 等二进制必踩）。"""
import argparse
import os


def cstr(data: bytes) -> str:
    # 每字节转 3 位八进制转义（定长无歧义），按 8KB 分段成相邻字符串字面量
    parts = []
    for i in range(0, len(data), 8192):
        chunk = data[i:i + 8192]
        parts.append('"' + "".join(f"\\{b:03o}" for b in chunk) + '"')
    return "\n    ".join(parts) if parts else '""'


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dist", required=True, help="前端构建产物目录")
    ap.add_argument("--data", required=True, help="默认数据 JSON 文件")
    ap.add_argument("--out", required=True, help="生成的头文件路径")
    a = ap.parse_args()

    entries = []
    for root, _, files in os.walk(a.dist):
        for f in sorted(files):
            full = os.path.join(root, f)
            rel = "/" + os.path.relpath(full, a.dist).replace(os.sep, "/")
            with open(full, "rb") as fh:
                entries.append((rel, fh.read()))
    with open(a.data, "rb") as fh:
        default_data = fh.read()

    lines = [
        "#pragma once",
        "// 由 tools/embed.py 自动生成，请勿手改。",
        "// 二进制用 unsigned char 数组 + 显式长度内嵌（std::string 遇 \\000 截断）",
        "#include <cstddef>",
        "#include <string>",
        "#include <utility>",
        "#include <vector>",
        "",
        "namespace tour::embed {",
        "",
        "struct EmbeddedFile {",
        "    const char* path;",
        "    const unsigned char* data;",
        "    std::size_t size;",
        "};",
        "",
    ]
    for i, (rel, data) in enumerate(entries):
        lines.append(f"inline const unsigned char DATA{i}[] = " + cstr(data) + ";")
    lines.append("")
    lines.append("inline const std::vector<EmbeddedFile>& files() {")
    lines.append("  static const std::vector<EmbeddedFile> files = {")
    for i, (rel, data) in enumerate(entries):
        lines.append(f'    {{ "{rel}", DATA{i}, sizeof(DATA{i}) - 1 }},')
    lines.append("  };")
    lines.append("  return files;")
    lines.append("}")
    lines.append("inline std::string defaultData() {")
    lines.append("  static const unsigned char D[] = " + cstr(default_data) + ";")
    lines.append("  return std::string(reinterpret_cast<const char*>(D), sizeof(D) - 1);")
    lines.append("}")
    lines.append("}  // namespace tour::embed")

    with open(a.out, "w", encoding="utf-8") as fh:
        fh.write("\n".join(lines) + "\n")
    size = sum(len(d) for _, d in entries) + len(default_data)
    print(f"generated {a.out}: {len(entries)} files, {size} bytes")


if __name__ == "__main__":
    main()
