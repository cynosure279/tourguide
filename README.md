# 景区旅游信息管理系统（数据结构课程设计）

以合肥景区为例的无向带权图应用系统：导游线路（DFS）、回路检测（拓扑排序）、最短路径（Dijkstra / Floyd）、道路修建规划（最小生成树）。

## 项目结构

```
alg/
├── docs/API.md            前后端接口约定
├── docs/BACKEND_DESIGN.md 后端设计说明
├── docs/CODE_EXPLAINED.md 后端代码讲解
├── data/hefei.json        默认景区数据（合肥 24 景点 / 48 道路）
├── frontend/              前端（Vue3 + Vite + Element Plus + ECharts）
└── backend/               C++ 后端
    ├── include/ src/      源码（src/algo/ 下是四个算法）
    ├── third_party/       httplib 和 nlohmann/json 两个头文件库
    └── scripts/smoke.sh   接口冒烟测试
```

## 前端运行（环境已配好：Node 22 + npm）

```bash
cd frontend
npm install
npm run dev        # http://localhost:5173 ，/api 自动代理到 localhost:8080
```

## 前端特性

- **深色模式**：右上角外观设置里可切换 自动/浅色/深色（默认自动，跟随系统）。
- **主题色**：外观设置里的连续色相渐变条，点击或按住拖动即可取色（每次 ±5° 一档），也支持键盘方向键微调；默认莫兰迪紫，即时生效并记忆。
- **毛玻璃质感**：卡片、顶栏、侧栏和浮层统一玻璃拟态风格（半透明 + 背景模糊），背景为柔和渐变与彩色光斑，亮/暗模式各有配色。
- **景点悬停简介**：图形页面鼠标悬停景点时，右上角弹出玻璃信息卡，显示名称、代号和简介，不遮挡图形。
- **多语言**：支持 简体中文 / English / 日本語，页面标题、菜单、提示语全部跟随。
- **无障碍**：键盘可完整操作（tab 焦点可见）、图表带 aria 说明、表格有语义标题、支持系统"减弱动态效果"偏好。

## Mock 模式（重要）

前端内置了一套**模拟后端**（`frontend/src/api/mock.js`）：启动时探测不到 8080 端口就自动切到模拟模式，右上角可手动切换。

### JS 参考算法（写 C++ 时的对照实现）

| 文件 | 内容 |
|---|---|
| `frontend/src/algo/dfs.js` | DFS 导游线路（含回溯序列语义） |
| `frontend/src/algo/cycle.js` | Kahn 拓扑排序判回路 + 环提取 |
| `frontend/src/algo/shortest.js` | Dijkstra（O(n²)）+ Floyd（含路径还原） |
| `frontend/src/algo/mst.js` | Prim + Kruskal（并查集） |

每个文件的函数头注释说明了与 `docs/API.md` 对应的输出语义。你的 C++ 实现结果若与模拟模式显示一致，即联调通过。

## 后端构建与运行

依赖只有两个头文件库（httplib、nlohmann/json），放在 backend/third_party/ 里，不用额外安装。

```bash
cmake --preset default                  # 配置（也可以 --preset release 出优化版）
cmake --build --preset default          # 编译
./out/build/default/backend/tour-backend    # 监听 http://localhost:8080
```

- 后端监听 8080 端口，启动时读 `data/hefei.json`。
- 如果有 `frontend/dist`（`npm run build` 的产物），后端会直接托管它，浏览器打开 http://localhost:8080 就是完整系统。
- 想换数据文件：`./tour-backend 路径/到/data.json`。
- 接口冒烟测试：`backend/scripts/smoke.sh`。

## 后端说明

- HTTP 服务、数据管理这些都在 `server.cpp` 和 `graph.cpp` 里；四个算法在 `backend/src/algo/` 下，每个文件对应一个功能：`dfs`（导游线路）、`cycle`（回路检测）、`shortest`（最短路径）、`mst`（修路规划）。
- 算法的具体语义（比如 DFS 邻点按 id 升序、walk 含回溯）按 `docs/API.md` 第 3 节来，和前端模拟模式的结果是一致的。
- 设计和代码的详细说明见 `docs/BACKEND_DESIGN.md` 和 `docs/CODE_EXPLAINED.md`。

## 常见问题

- **前端报"无法连接后端"**：后端未启动或端口不是 8080；改端口在 `frontend/vite.config.js` 的 proxy.target。
- **改了默认数据**：`data/hefei.json` 与 `frontend/src/api/data.json` 需同步修改（后者是 Mock 的副本）。
- **构建部署**：`npm run build` 产出 `frontend/dist/`，可由后端直接托管静态文件（此时接口同源，无跨域问题）。
