# 景区旅游信息管理系统 — 前后端接口契约 v1.0

本文档是前后端开发的**唯一约定**。后端（C++）按此实现，前端已按此实现。

## 1. 基础约定

- 后端监听 `http://localhost:8080`，所有接口以 `/api` 为前缀。
- 请求/响应均为 JSON（`Content-Type: application/json`）。
- **统一响应格式**：

```json
{ "code": 0, "msg": "ok", "data": { } }
```

| 字段 | 说明 |
|---|---|
| code | `0` 成功；非 0 业务失败（如参数非法、景点不存在） |
| msg | 成功为 `"ok"`，失败为中文错误描述 |
| data | 业务数据，无数据时为 `null` |

- 业务错误建议返回 HTTP 200 + `code≠0`；HTTP 4xx/5xx 前端也能处理（读取 body 中 msg）。
- 开发阶段前端经 Vite 代理访问 `/api`（无跨域问题）；若前端单独部署请开启 CORS：
  `Access-Control-Allow-Origin: *` 并响应 OPTIONS 预检。

## 2. 通用数据结构

**景点 Attraction**：

```json
{ "id": 0, "code": "A01", "name": "包公园", "intro": "为纪念北宋清官包拯而建……", "posx": 55, "posy": 42 }
```

| 字段 | 类型 | 说明 |
|---|---|---|
| id | int | 景点编号（顶点号），全局唯一，新增时由后端分配（建议 max(id)+1） |
| code | string | 代号，如 "A13" |
| name | string | 名称 |
| intro | string | 简介 |
| posx, posy | number | 平面坐标 0~100，仅供前端画图，不参与算法 |

**道路 Edge**（无向带权边）：

```json
{ "from": 0, "to": 1, "distance": 2.8 }
```

`from`/`to` 为景点 id，`distance` 为里程（km，>0）。后端需保证：无自环、无重边（重复添加返回错误）。

## 3. 算法语义定义（重要，保证结果与前端一致）

### 3.1 导游线路图（DFS）

- 从入口景点做深度优先遍历，**邻点按景点 id 升序**访问（保证结果确定）。
- `walk` 为**含回溯的完整行走序列**，如 `[0,1,3,1,0,2,...,0]`（进入下一景点或原路返回都记一步）。
- `edges` 为行走序列相邻两点构成的有向边（`walk[i] → walk[i+1]`），**去重**后输出——这就是导游线路图（有向图）。
- 图不连通时，未被访问的景点放入 `unreachable`。

### 3.2 回路检测（拓扑排序）

- 对 3.1 得到的**有向导游线路图**运行 Kahn 拓扑排序。
- 入拓扑序列的顶点数 < 顶点总数 ⇒ 有回路。
- `topoOrder`：成功入序的顶点；`cyclicVertices`：未入序（在环上或指向环）的顶点；`cycles`：提取出的环，每个环是顶点序列 `[u, v, ...]`（首尾相接，如 `[0,1]` 表示 0→1→0）。
- 提示：DFS 回溯必然产生 u→v 与 v→u 的双向边，因此正常情况下导游线路图**一定有回路**，环内景点即"供人工优化"的对象。

### 3.3 最短路径（Dijkstra，from==to 时距离 0、路径为 `[from]`）

- 不可达时 `reachable=false, distance=null, path=[]`。
- `path` 含起点与终点，按途经顺序排列。
- 全源接口用 Floyd 实现，矩阵对角线为 0，不可达为 `null`。

### 3.4 最小生成树（Prim / Kruskal，二选一实现亦可，接口需支持 algo 参数）

- 返回 `{spanning, totalCost, edges}`；图连通时 `spanning=true`。
- 图不连通时返回**最小生成森林**（能连的部分 + `spanning=false`），`totalCost` 为森林总里程。
- `totalCost` 保留两位小数。

## 4. 接口明细

### 4.1 GET /api/health — 健康检查

```json
{ "code": 0, "msg": "ok", "data": { "status": "ok" } }
```

### 4.2 GET /api/attractions — 景点列表

`data` 为 Attraction 数组（建议按 id 升序；邻接矩阵的行列顺序与此数组一致）。

### 4.3 POST /api/attractions — 新增景点

请求体：`{ "code": "A13", "name": "xxx", "intro": "xxx", "posx": 50, "posy": 50 }`

响应 data 为新建的完整 Attraction（含 id）。校验：code/name 非空、code 不重复。

### 4.4 DELETE /api/attractions/{id} — 删除景点

同时删除所有与该景点相连的道路。成功 data 为 null。不存在时 code=1。

### 4.5 GET /api/edges — 道路列表

`data` 为 Edge 数组。

### 4.6 POST /api/edges — 新增道路

请求体：`{ "from": 0, "to": 5, "distance": 7.6 }`

校验：两端点存在、from≠to、distance>0、不与已有边重复（无向，正反都算重复）。成功 data 为新建 Edge。

### 4.7 DELETE /api/edges/{u}/{v} — 删除道路

无向边，`u v` 顺序不限。成功 data 为 null。

### 4.8 GET /api/graph/matrix — 邻接矩阵

`data`：

```json
{
  "ids":   [0, 1, 2],
  "names": ["包公园", "逍遥津", "明教寺"],
  "matrix": [
    [0,    2.8,  1.9],
    [2.8,  0,    null],
    [1.9,  null, 0  ]
  ]
}
```

`matrix[i][j]`：i→j 直接距离；无边为 `null`；对角线为 0。行列顺序 = `/api/attractions` 返回顺序。

### 4.9 GET /api/route/tour?start={id} — 导游线路图（DFS）

`start` 可省略，默认 id 最小的景点。`data`：

```json
{
  "startId": 0,
  "walk": [0, 1, 2, 1, 0, 3, 0],
  "edges": [ { "from": 0, "to": 1 }, { "from": 1, "to": 2 }, { "from": 1, "to": 0 },
             { "from": 2, "to": 1 }, { "from": 0, "to": 3 }, { "from": 3, "to": 0 } ],
  "unreachable": []
}
```

### 4.10 GET /api/route/cycle?start={id} — 回路检测（对导游线路图）

`start` 可省略，默认同上。`data`：

```json
{
  "hasCycle": true,
  "topoOrder": [],
  "cyclicVertices": [0, 1, 2, 3],
  "cycles": [ [0, 1], [1, 2], [0, 3] ]
}
```

### 4.11 GET /api/path/shortest?from={id}&to={id}&algo={algo} — 两点最短路径

`from`/`to` 必填。`algo` 可选，默认 `dijkstra`，可选值：

| algo | 算法 | 复杂度 |
|---|---|---|
| `dijkstra` | Dijkstra（朴素版） | O(n²) |
| `dijkstra-heap` | Dijkstra（优先队列堆优化） | O((V+E)logV) |
| `bellman-ford` | Bellman-Ford | O(VE) |
| `spfa` | SPFA（队列优化 Bellman-Ford） | 均 O(kE) |
| `floyd` | Floyd 全源后取单对 | O(n³) |

同一张图上各算法的最短距离一致（可互验）；并列路径时不同算法可能选出不同的等长路径。
非法 `algo` 返回 code=1。`data`：

```json
{ "reachable": true, "distance": 4.7, "path": [0, 5, 8] }
```

### 4.12 GET /api/path/all — 全源最短距离矩阵（Floyd）

结构与 4.8 相同（ids/names/matrix），但 `matrix[i][j]` 为 i→j **最短**距离，不可达为 `null`，对角线 0。

### 4.13 GET /api/roads/mst?algo=prim|kruskal — 道路修建规划（最小生成树）

`algo` 可省略，默认 `prim`。`data`：

```json
{
  "spanning": true,
  "totalCost": 63.9,
  "edges": [ { "from": 2, "to": 3, "distance": 0.9 }, { "from": 0, "to": 3, "distance": 1.9 } ]
}
```

### 4.14 POST /api/graph/reset — 恢复默认数据

后端从初始数据文件（`data/hefei.json`，随本项目提供）重新加载。data 为 null。

### 4.15 GET /api/graph/export — 导出预设

把当前景点/道路导出成一份预设，可直接保存成 .json 文件再导入。`data`：

```json
{
  "name": "合肥",
  "attractions": [ { "id": 0, "code": "A01", "name": "包公园", "intro": "…", "posx": 55, "posy": 42 } ],
  "edges": [ { "from": 0, "to": 1, "distance": 2.8 } ]
}
```

`name` 为当前预设名（默认数据叫"合肥"，导入的预设沿用其 `name` 或 `city` 字段）。

### 4.16 POST /api/graph/import — 导入预设

请求体为 4.15 导出的同格式对象（`name` 可选）。校验规则与加载数据文件一致：
attractions/edges 必须为数组、景点 id 不重复、坐标在 0~100、道路端点存在、
无自环、distance>0、无重边。成功后整体替换当前数据，data 为 null；校验失败
时原数据不受影响。

## 5. 错误响应示例

```json
{ "code": 1, "msg": "景点不存在: 99", "data": null }
```

## 6. 默认数据

见项目根目录 `data/hefei.json`（合肥 24 景点 / 48 条道路，连通且含多个回路，满足课设全部演示需求）。启动时加载，`/api/graph/reset` 恢复。

