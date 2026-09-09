#include "algo.hpp"

#include <algorithm>
#include <functional>
#include <queue>

namespace tour::algo {

namespace {

// 邻接表（下标域）：无向边两个方向都挂，邻点按下标升序保证确定性
std::vector<std::vector<std::pair<int, double>>> buildAdj(const Graph& g) {
    int n = g.vertexCount();
    std::vector<std::vector<std::pair<int, double>>> adj(n);
    for (const Edge& e : g.edges()) {
        int a = g.indexOf(e.from);
        int b = g.indexOf(e.to);
        adj[a].push_back({b, e.distance});
        adj[b].push_back({a, e.distance});
    }
    for (auto& list : adj) {
        std::sort(list.begin(), list.end(),
                  [](const auto& x, const auto& y) { return x.first < y.first; });
    }
    return adj;
}

// 沿 prev 数组从终点走回起点，再倒过来
std::vector<int> rebuild(const Graph& g, const std::vector<int>& prev, int s, int t) {
    std::vector<int> ids = g.vertexIds();
    std::vector<int> path;
    int cur = t;
    while (true) {
        path.push_back(ids[cur]);
        if (cur == s) break;
        cur = prev[cur];
    }
    std::reverse(path.begin(), path.end());
    return path;
}

// ① Dijkstra：O(n^2) 朴素写法，每轮挑未确定里 dist 最小的
ShortestResult dijkstraNaive(const Graph& g, int s, int t) {
    int n = g.vertexCount();
    const auto w = buildWeightMatrix(g);  // 邻接矩阵，没有边是 INF

    std::vector<double> dist(n, INF);
    std::vector<int> prev(n, -1);
    std::vector<bool> done(n, false);
    dist[s] = 0.0;

    while (true) {
        // 在还没确定的顶点里挑 dist 最小的，并列取下标小（id 小）的
        int u = -1;
        double best = INF;
        for (int i = 0; i < n; i++) {
            if (!done[i] && dist[i] < best) {
                best = dist[i];
                u = i;
            }
        }
        if (u == -1) break;  // 剩下的都不可达
        done[u] = true;

        // 松弛，严格小于才更新
        for (int v = 0; v < n; v++) {
            if (done[v] || w[u][v] == INF) continue;
            if (dist[u] + w[u][v] < dist[v]) {
                dist[v] = dist[u] + w[u][v];
                prev[v] = u;
            }
        }
    }

    ShortestResult result;
    if (dist[t] == INF) return result;  // 不可达，reachable=false

    result.reachable = true;
    result.distance = dist[t];
    result.path = rebuild(g, prev, s, t);
    return result;
}

// ② Dijkstra 堆优化：优先队列代替“每轮扫一遍找最小”，O((V+E)logV)。
// 队列里可能有过期记录，弹出时用 done 数组跳过（懒惰删除）
ShortestResult dijkstraHeap(const Graph& g, int s, int t) {
    int n = g.vertexCount();
    const auto adj = buildAdj(g);

    std::vector<double> dist(n, INF);
    std::vector<int> prev(n, -1);
    std::vector<bool> done(n, false);
    using Item = std::pair<double, int>;  // (dist, 顶点下标)
    std::priority_queue<Item, std::vector<Item>, std::greater<Item>> pq;

    dist[s] = 0.0;
    pq.push({0.0, s});
    while (!pq.empty()) {
        auto [d, u] = pq.top();
        pq.pop();
        if (done[u]) continue;
        done[u] = true;
        for (const auto& [v, w] : adj[u]) {
            if (done[v]) continue;
            if (d + w < dist[v]) {
                dist[v] = d + w;
                prev[v] = u;
                pq.push({dist[v], v});
            }
        }
    }

    ShortestResult result;
    if (dist[t] == INF) return result;
    result.reachable = true;
    result.distance = dist[t];
    result.path = rebuild(g, prev, s, t);
    return result;
}

// ③ Bellman-Ford：把所有边松弛 n-1 轮，O(VE)，支持负权边（本图没有）
ShortestResult bellmanFord(const Graph& g, int s, int t) {
    int n = g.vertexCount();
    const auto adj = buildAdj(g);

    std::vector<double> dist(n, INF);
    std::vector<int> prev(n, -1);
    dist[s] = 0.0;

    for (int round = 0; round < n - 1; round++) {
        bool changed = false;
        for (int u = 0; u < n; u++) {
            if (dist[u] == INF) continue;  // 还没到过 u，它的边先不管
            for (const auto& [v, w] : adj[u]) {
                if (dist[u] + w < dist[v]) {
                    dist[v] = dist[u] + w;
                    prev[v] = u;
                    changed = true;
                }
            }
        }
        if (!changed) break;  // 这一轮没有任何松弛，提前结束
    }

    ShortestResult result;
    if (dist[t] == INF) return result;
    result.reachable = true;
    result.distance = dist[t];
    result.path = rebuild(g, prev, s, t);
    return result;
}

// ④ SPFA：队列优化的 Bellman-Ford，只有刚被更新的点才需要再松弛别人
ShortestResult spfa(const Graph& g, int s, int t) {
    int n = g.vertexCount();
    const auto adj = buildAdj(g);

    std::vector<double> dist(n, INF);
    std::vector<int> prev(n, -1);
    std::vector<bool> inQueue(n, false);
    std::deque<int> queue;

    dist[s] = 0.0;
    queue.push_back(s);
    inQueue[s] = true;
    while (!queue.empty()) {
        int u = queue.front();
        queue.pop_front();
        inQueue[u] = false;
        for (const auto& [v, w] : adj[u]) {
            if (dist[u] + w < dist[v]) {
                dist[v] = dist[u] + w;
                prev[v] = u;
                if (!inQueue[v]) {
                    queue.push_back(v);
                    inQueue[v] = true;
                }
            }
        }
    }

    ShortestResult result;
    if (dist[t] == INF) return result;
    result.reachable = true;
    result.distance = dist[t];
    result.path = rebuild(g, prev, s, t);
    return result;
}

// ⑤ Floyd：先算全源最短距离，再按 next 矩阵还原 i->j 的路径
ShortestResult floydPair(const Graph& g, int s, int t) {
    int n = g.vertexCount();
    auto dist = buildWeightMatrix(g);

    // next[i][j]：从 i 到 j 的路径上 i 的下一跳
    std::vector<std::vector<int>> next(n, std::vector<int>(n, -1));
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            if (i != j && dist[i][j] != INF) next[i][j] = j;

    // k 放最外层：中转点只允许用前 k 个
    for (int k = 0; k < n; k++) {
        for (int i = 0; i < n; i++) {
            if (dist[i][k] == INF) continue;
            for (int j = 0; j < n; j++) {
                if (dist[k][j] == INF) continue;
                if (dist[i][k] + dist[k][j] < dist[i][j]) {
                    dist[i][j] = dist[i][k] + dist[k][j];
                    next[i][j] = next[i][k];
                }
            }
        }
    }

    ShortestResult result;
    if (dist[s][t] == INF) return result;
    result.reachable = true;
    result.distance = dist[s][t];

    std::vector<int> ids = g.vertexIds();
    int cur = s;
    result.path.push_back(ids[cur]);
    while (cur != t) {
        cur = next[cur][t];
        if (cur < 0) return ShortestResult{};  // 理论到不了这里，防御一下
        result.path.push_back(ids[cur]);
    }
    return result;
}

}  // namespace

// 按算法名分发；同一张图上各算法的最短距离一致，可互验
ShortestResult shortestPath(const Graph& g, int from, int to, const std::string& algo) {
    const int s = g.indexOf(from);
    const int t = g.indexOf(to);
    if (s < 0 || t < 0) throw std::invalid_argument("shortestPath: 顶点不存在");

    if (algo == "dijkstra") return dijkstraNaive(g, s, t);
    if (algo == "dijkstra-heap") return dijkstraHeap(g, s, t);
    if (algo == "bellman-ford") return bellmanFord(g, s, t);
    if (algo == "spfa") return spfa(g, s, t);
    if (algo == "floyd") return floydPair(g, s, t);

    // 路由层已校验 algo 参数，正常到不了这里
    throw AlgoNotImplemented("未知算法: " + algo);
}

// Floyd 全源最短路径（API.md 3.3）
AllShortestResult allShortest(const Graph& g) {
    AllShortestResult result;
    result.ids = g.vertexIds();
    result.dist = buildWeightMatrix(g);  // 初始就是直接里程
    const int n = g.vertexCount();

    for (int k = 0; k < n; k++) {
        for (int i = 0; i < n; i++) {
            if (result.dist[i][k] == INF) continue;  // i 到 k 不通就跳过
            for (int j = 0; j < n; j++) {
                if (result.dist[k][j] == INF) continue;  // 省得无穷大相加
                double nd = result.dist[i][k] + result.dist[k][j];
                if (nd < result.dist[i][j]) result.dist[i][j] = nd;
            }
        }
    }
    return result;
}

}  // namespace tour::algo