#include "algo.hpp"

#include <algorithm>
#include <functional>
#include <numeric>
#include <utility>
#include <vector>

namespace tour::algo {

namespace {

// Prim：从 id 最小的景点出发，每轮在"树里连树外"的边里挑最短的。
// 遍历顺序（树内按入树顺序、边按输入顺序）和并列时取先遍历到的，
// 都跟前端 mst.js 保持一致，这样两边画出来的树一样
MstResult prim(const Graph& g) {
    const int n = g.vertexCount();
    const std::vector<Edge>& edges = g.edges();

    MstResult result;
    if (n == 0) return result;

    // 邻接表：每个点存（边下标，对面点下标）
    std::vector<std::vector<std::pair<int, int>>> adj(n);
    for (int ei = 0; ei < (int)edges.size(); ei++) {
        int a = g.indexOf(edges[ei].from);
        int b = g.indexOf(edges[ei].to);
        adj[a].push_back({ei, b});
        adj[b].push_back({ei, a});
    }

    std::vector<bool> inTree(n, false);
    std::vector<int> treeOrder;  // 按入树顺序记下来，后面按这个顺序扫
    treeOrder.push_back(0);
    inTree[0] = true;

    double total = 0;
    while ((int)treeOrder.size() < n) {
        int bestEdge = -1, bestV = -1;
        for (int u : treeOrder) {
            for (const auto& [ei, v] : adj[u]) {
                if (inTree[v]) continue;
                // 严格小于，并列时保留先遍历到的
                if (bestEdge == -1 || edges[ei].distance < edges[bestEdge].distance) {
                    bestEdge = ei;
                    bestV = v;
                }
            }
        }
        if (bestEdge == -1) break;  // 连不进来了，说明图不连通

        inTree[bestV] = true;
        treeOrder.push_back(bestV);
        const Edge& e = edges[bestEdge];
        result.edges.push_back({e.from, e.to, e.distance});
        total += e.distance;
    }

    result.spanning = (int)treeOrder.size() == n;
    result.totalCost = total;
    return result;
}

// Kruskal：边按里程从小到大试，用并查集判断会不会成环
MstResult kruskal(const Graph& g) {
    const int n = g.vertexCount();
    const std::vector<Edge>& edges = g.edges();

    MstResult result;
    if (n == 0) return result;

    // 并查集，find 写成递归 lambda 顺手压缩路径
    std::vector<int> parent(n);
    for (int i = 0; i < n; i++) parent[i] = i;
    std::function<int(int)> find = [&](int x) {
        return parent[x] == x ? x : (parent[x] = find(parent[x]));
    };

    // 排边的下标。用 stable_sort：里程相同的边保持原顺序（和前端一致）
    std::vector<int> order(edges.size());
    std::iota(order.begin(), order.end(), 0);
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
        return edges[a].distance < edges[b].distance;
    });

    double total = 0;
    for (int ei : order) {
        const Edge& e = edges[ei];
        int ra = find(g.indexOf(e.from));
        int rb = find(g.indexOf(e.to));
        if (ra == rb) continue;  // 已经连上了，再加会成环
        parent[ra] = rb;
        result.edges.push_back({e.from, e.to, e.distance});
        total += e.distance;
        if ((int)result.edges.size() == n - 1) break;  // 够 n-1 条就结束了
    }

    result.spanning = (int)result.edges.size() == n - 1;  // 不够说明图不连通
    result.totalCost = total;
    return result;
}

}  // namespace

MstResult minimumSpanningTree(const Graph& g, const std::string& algo) {
    if (algo == "prim") return prim(g);
    if (algo == "kruskal") return kruskal(g);
    // 路由层已经校验过参数，正常到不了这里
    throw AlgoNotImplemented("未知算法: " + algo);
}

}  // namespace tour::algo