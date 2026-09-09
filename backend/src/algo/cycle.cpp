#include "algo.hpp"

#include <deque>
#include <functional>
#include <vector>

namespace tour::algo {

// 对导游线路图做 Kahn 拓扑排序判断有没有回路（API.md 3.2）。
// 先拓扑排序，入序顶点数 < 总数说明有环；再在剩下的子图里 DFS 把环挖出来
CycleResult detectCycle(const Graph& g, int startId) {
    const TourResult tour = buildTour(g, startId);
    const std::vector<int> ids = g.vertexIds();
    const int n = g.vertexCount();

    // 顶点编号换成 0..n-1 的下标，数组可以直接拿来用
    std::vector<std::vector<int>> adj(n);
    std::vector<int> inDeg(n, 0);
    for (const auto& e : tour.edges) {
        int u = g.indexOf(e.from);
        int v = g.indexOf(e.to);
        adj[u].push_back(v);
        inDeg[v]++;
    }

    // Kahn：入度为 0 的顶点按下标（也就是 id）升序入队，FIFO 出队
    std::deque<int> queue;
    for (int i = 0; i < n; i++) {
        if (inDeg[i] == 0) queue.push_back(i);
    }
    std::vector<int> topoIdx;
    while (!queue.empty()) {
        int u = queue.front();
        queue.pop_front();
        topoIdx.push_back(u);
        for (int v : adj[u]) {
            if (--inDeg[v] == 0) queue.push_back(v);
        }
    }

    CycleResult result;
    result.hasCycle = (int)topoIdx.size() < n;
    for (int i : topoIdx) result.topoOrder.push_back(ids[i]);

    // 没入序的顶点就是环上（或指向环）的顶点
    std::vector<bool> inTopo(n, false);
    for (int i : topoIdx) inTopo[i] = true;
    for (int i = 0; i < n; i++) {
        if (!inTopo[i]) result.cyclicVertices.push_back(ids[i]);
    }

    // 在残留子图里 DFS 找环。显式栈记当前路径，posOf 记每个点在栈里的位置，
    // 走到路径上已有的顶点 v 时，从 posOf[v] 截到栈顶就是一条环
    std::vector<bool> visited(n, false);
    std::vector<bool> onStack(n, false);
    std::vector<int> stack;
    std::vector<int> posOf(n, -1);

    std::function<void(int)> dfs = [&](int u) {
        visited[u] = true;
        onStack[u] = true;
        posOf[u] = (int)stack.size();
        stack.push_back(u);
        for (int v : adj[u]) {
            if (inTopo[v]) continue;  // 只走残留子图
            if (onStack[v]) {
                std::vector<int> cycle;
                for (int k = posOf[v]; k < (int)stack.size(); k++) {
                    cycle.push_back(ids[stack[k]]);
                }
                result.cycles.push_back(cycle);
            } else if (!visited[v]) {
                dfs(v);
            }
        }
        // 往回退的时候要把状态清掉，不然会把交叉边误判成环
        stack.pop_back();
        posOf[u] = -1;
        onStack[u] = false;
    };
    for (int s : result.cyclicVertices) {
        if (!visited[g.indexOf(s)]) dfs(g.indexOf(s));
    }
    return result;
}

}  // namespace tour::algo