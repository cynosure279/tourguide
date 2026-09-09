#pragma once

#include "graph.hpp"

#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace tour::algo {

// 不可达标记，buildWeightMatrix 里没有边的地方也是它
inline constexpr double INF = std::numeric_limits<double>::infinity();

// 算法没实现时抛这个（现在四个都写好了，只剩 mst 里对未知算法的兜底）
struct AlgoNotImplemented : std::logic_error {
    explicit AlgoNotImplemented(const std::string& what) : std::logic_error(what) {}
};

// n×n 邻接矩阵：直接里程，对角线是 0，没有边是 INF。
// 行列顺序和 Graph::vertexIds() 一致（按 id 升序），配合 indexOf/idAt 换算
std::vector<std::vector<double>> buildWeightMatrix(const Graph& g);

// ---- DFS 导游线路（API.md 3.1）----
// walk 是含回溯的行走序列，edges 是 walk 相邻两点组成的有向边（去重），
// 没走到的景点放 unreachable。邻点按 id 升序访问，结果才是确定的
struct TourEdge { int from, to; };
struct TourResult {
    int startId = 0;
    std::vector<int> walk;
    std::vector<TourEdge> edges;
    std::vector<int> unreachable;
};
TourResult buildTour(const Graph& g, int startId);  // dfs.cpp

// ---- 回路检测（3.2）：对导游线路图做 Kahn 拓扑排序 ----
struct CycleResult {
    bool hasCycle = false;
    std::vector<int> topoOrder;      // 成功入序的顶点
    std::vector<int> cyclicVertices; // 没入序的（在环上或指向环）
    std::vector<std::vector<int>> cycles;  // 提取出来的环
};
CycleResult detectCycle(const Graph& g, int startId);  // cycle.cpp

// ---- 最短路径（3.3）----
// shortestPath：from==to 时 distance=0、path={from}；不可达时 reachable=false
// allShortest：dist[i][j] 是最短距离，不可达是 INF，取整和转 null 由路由层做
struct ShortestResult {
    bool reachable = false;
    double distance = 0.0;
    std::vector<int> path;
};
struct AllShortestResult {
    std::vector<int> ids;
    std::vector<std::vector<double>> dist;
};

// 两点最短路径支持的算法（/api/path/shortest 的 algo 参数）：
//   dijkstra        Dijkstra（O(n^2) 朴素版）
//   dijkstra-heap   Dijkstra（优先队列堆优化，O((V+E)logV)）
//   bellman-ford    Bellman-Ford（O(VE)，支持负权边）
//   spfa            SPFA（队列优化 Bellman-Ford）
//   floyd           Floyd 全源后取单对
inline const std::vector<std::string>& shortestAlgoNames() {
    static const std::vector<std::string> names = {
        "dijkstra", "dijkstra-heap", "bellman-ford", "spfa", "floyd",
    };
    return names;
}

inline bool isValidShortestAlgo(const std::string& algo) {
    for (const auto& n : shortestAlgoNames())
        if (n == algo) return true;
    return false;
}

// 同一张图上各算法的最短距离应一致（可互验）；
// 并列路径时不同算法可能选出不同的等长路径
ShortestResult shortestPath(const Graph& g, int from, int to,
                            const std::string& algo = "dijkstra");  // shortest.cpp
AllShortestResult allShortest(const Graph& g);                      // shortest.cpp

// ---- 最小生成树（3.4），algo 是 "prim" 或 "kruskal" ----
// 图不连通时返回生成森林，spanning=false，totalCost 是森林总里程
struct MstEdge { int from, to; double distance; };
struct MstResult {
    bool spanning = true;
    double totalCost = 0.0;
    std::vector<MstEdge> edges;
};
MstResult minimumSpanningTree(const Graph& g, const std::string& algo);  // mst.cpp

}  // namespace tour::algo