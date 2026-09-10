# 构建与使用指南

本项目 = C++ 后端（单二进制）+ Vue3 前端。后端可以直接托管前端页面，所以整个系统既可以开发模式跑，也可以打成一个**自包含的可执行文件**分发。

## 1. 环境要求

| 工具 | 用途 | 说明 |
|---|---|---|
| CMake ≥ 3.16 | 构建后端 | `cmake --version` 检查 |
| C++17 编译器 | 编译后端 | macOS Xcode CLT / Linux gcc / Windows MSVC |
| Node.js ≥ 18 | 构建前端 | `node --version` 检查 |
| Python 3 | 仅"单文件内嵌版"需要 | 三大平台默认都有 |

第三方库（cpp-httplib、nlohmann-json）以内嵌头文件形式放在 `backend/third_party/`，**无需额外安装**。

## 2. 三种运行方式

### 方式 A：开发模式（改代码实时看）

```bash
# 终端 1：后端（8080）
cmake --preset default && cmake --build --preset default
./out/build/default/backend/tour-backend

# 终端 2：前端开发服务器（5173，/api 自动代理到 8080）
cd frontend && npm run dev
```

浏览器打开 http://localhost:5173。改前端代码热更新；改 C++ 代码需重新编译并**重启后端**。

### 方式 B：标准本地构建（前后端分离部署）

```bash
cd frontend && npm ci && npm run build && cd ..
cmake --preset default && cmake --build --preset default
./out/build/default/backend/tour-backend
```

后端启动时自动寻找 `frontend/dist` 并托管——浏览器直接开 **http://localhost:8080** 就是完整系统（前端 + 接口同源，无跨域问题）。

### 方式 C：单文件可执行版（推荐用于演示/分发）

```bash
cd frontend && npm ci && npm run build && cd ..
cmake -S . -B build-embed -DCMAKE_BUILD_TYPE=Release -DEMBED_FRONTEND=ON
cmake --build build-embed
./build-embed/backend/tour-backend
```

`EMBED_FRONTEND=ON` 会把前端页面和默认数据**直接内嵌进二进制**：得到的 `tour-backend` 是一个自包含的可执行文件，拷到任何同系统机器上双击就能跑，不需要随包携带任何其它文件。

## 3. 配置说明

### 数据文件

启动时的数据来源优先级：**命令行参数 > 磁盘 data/hefei.json > 内嵌默认数据**（仅方式 C 有内嵌）。

```bash
./tour-backend                          # 自动定位数据文件
./tour-backend /path/to/data.json      # 指定数据文件（任意符合格式的 JSON）
```

数据文件格式见 `docs/API.md` 第 2/6 节。运行中可在「景点管理」页导入/导出预设（JSON），或点「恢复默认数据」重置。

### 端口

固定 8080（前端代理与文档均按此约定）。端口被占用时后端会启动失败并提示。

### 外观设置（右上角齿轮）

- **主题色**：连续色相条，默认莫兰迪紫，全站 UI/图表/表格跟随
- **深色模式**：自动（跟随系统）/ 浅色 / 深色
- **信息卡大小**：悬停信息卡的缩放滑杆
- **动画效果**：一键关闭全部动画（含图表动画）
- **语言**：简体中文 / English / 日本語

以上均存 localStorage，刷新后保持。

### 图表交互

地图可滚轮缩放、拖拽平移；悬停节点显示简介信息卡。最短路径页可切换 5 种算法（Dijkstra 朴素/堆优化、Bellman-Ford、SPFA、Floyd）并一键互验。

### 方式 D：桌面应用（Tauri，推荐给最终用户）

项目含一个 Tauri（Rust）桌面壳：原生窗口 + 内嵌后端侧车，打包成各平台安装包。

```bash
cd frontend
npm install          # 首次安装依赖（含 @tauri-apps/cli）
npx tauri build      # 自动：构建前端 → 构建内嵌后端 → 打包
```

产物（macOS 示例）：

```
frontend/src-tauri/target/release/bundle/macos/tour-guide.app
frontend/src-tauri/target/release/bundle/dmg/tour-guide_1.0.0_aarch64.dmg
```

- 首次打开 .app 如果被系统拦截（未签名应用）：右键 → 打开 → 确认即可
- 应用启动 = 自动拉起内嵌后端（8080）→ 打开窗口加载系统页面；退出时自动关闭后端
- 端口被占用时显示"启动失败"错误页
- Windows（.msi/.exe）、Linux（.deb/AppImage）由 CI 的 `desktop` job 打包

## 4. 冒烟测试

后端运行时执行 `backend/scripts/smoke.sh`，自动覆盖全部接口（含错误用例），结尾自动恢复默认数据。

## 5. 分支模型

| 分支 | 内容 | 用途 |
|---|---|---|
| `full` | 完整版：含全部预设数据与资源 | 私有开发分支 |
| `release` | 发布版：不含特定机构预设数据与对应载入入口 | 推送到公开仓库 / 打 tag 发布 |

同步流程：日常开发在 `full`；需要发布时：

```bash
git checkout release
git merge full          # 或 cherry-pick
# 确认预设相关文件未被带入：
ls data/hfu.json frontend/src/api/hfu.json 2>/dev/null   # 应提示不存在
git push origin release
```

若合并后发现预设文件被带入：`git rm data/hfu.json frontend/src/api/hfu.json` 后再提交。

## 6. CI/CD（GitHub Actions）

- **ci.yml**：每次 push/PR 自动在 Linux/macOS/Windows 三平台构建前端与后端，并做接口冒烟测试。
- **release.yml**：在 `release` 分支推送 `v*` 标签（如 `v1.0.0`）时触发：三平台编译**内嵌版单文件** + **Tauri 桌面应用**（macOS .dmg / Windows .msi / Linux .deb），自动创建 GitHub Release 并挂载全部产物。

```bash
git checkout release
git tag v1.0.0 && git push origin v1.0.0   # 触发发布
```

## 7. 常见问题

- **端口被占用**：后端启动失败提示"监听失败"，关闭占用 8080 的进程即可。
- **改了 C++ 代码没生效**：需要重新编译**并重启后端**（前端/静态文件改动则无需重启，刷新即可）。
- **改了前端代码没生效**：方式 A 下热更新；方式 B/C 需要 `npm run build` 后刷新（后端响应带 no-cache 头，普通刷新即可）。
- **浏览器页面是旧的**：确认访问的是 8080；后端响应已带 `Cache-Control: no-cache`，正常刷新即可拿到最新页面。
- **换了机器二进制找不到数据**：内嵌版自动使用内嵌数据；标准版用 `./tour-backend /path/to/data.json` 指定。
