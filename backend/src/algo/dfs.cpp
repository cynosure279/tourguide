#include "algo.hpp"

#include <functional>
#include <set>
#include <utility>

namespace tour::algo {

// DFS 生成导游线路图，语义见 API.md 3.1：
// 邻点按 id 升序访问；walk 记录含回溯的完整行走序列；
// edges 是 walk 相邻两点组成的有向边（去重）；没走到的景点放 unreachable
TourResult buildTour(const Graph& g, int startId) {
    if (!g.hasVertex(startId)) throw std::invalid_argument("buildTour: 起点不存在");

    TourResult result;
    result.startId = startId;

    std::set<int> visited;
    std::vector<int>& walk = result.walk;

    // 用 std::function 是因为递归 lambda 要能调用到它自己
    std::function<void(int)> dfs = [&](int u) {
        visited.insert(u);
        walk.push_back(u);
        for (const auto& nb : g.neighbors(u)) {  // neighbors 已按邻点 id 升序排好
            int v = nb.first;
            if (visited.count(v)) continue;
            dfs(v);
            walk.push_back(u);  // 从 v 退回到 u，也算一步
        }
    };
    dfs(startId);

    // 相邻两点组成有向边，按第一次出现的顺序去重
    std::set<std::pair<int, int>> seen;
    for (size_t i = 0; i + 1 < walk.size(); i++) {
        if (seen.insert({walk[i], walk[i + 1]}).second) {
            result.edges.push_back({walk[i], walk[i + 1]});
        }
    }

    // 没被访问到的就是 unreachable（vertexIds 本身升序）
    for (int id : g.vertexIds()) {
        if (!visited.count(id)) result.unreachable.push_back(id);
    }
    return result;
}

}  // namespace tour::algo