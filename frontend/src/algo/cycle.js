import { buildTour } from './dfs'
import { vertexIds } from './graph'

/**
 * 对有向导游线路图做 Kahn 拓扑排序判断有无回路。
 * 语义（与 docs/API.md 3.2 一致）：
 *  - topoOrder：成功入序的顶点
 *  - cyclicVertices：未入序的顶点（在环上或指向环）
 *  - cycles：从残留子图中提取的环，每个环为顶点序列 [u, v, ...]（首尾相接）
 * 返回 { hasCycle, topoOrder, cyclicVertices, cycles }
 */
export function detectCycle(attractions, edges, startId) {
  const tour = buildTour(attractions, edges, startId)
  const ids = vertexIds(attractions)

  const inDeg = new Map(ids.map((id) => [id, 0]))
  const adj = new Map(ids.map((id) => [id, []]))
  for (const e of tour.edges) {
    adj.get(e.from).push(e.to)
    inDeg.set(e.to, inDeg.get(e.to) + 1)
  }

  const queue = ids.filter((id) => inDeg.get(id) === 0)
  const topoOrder = []
  while (queue.length > 0) {
    const u = queue.shift()
    topoOrder.push(u)
    for (const v of adj.get(u)) {
      inDeg.set(v, inDeg.get(v) - 1)
      if (inDeg.get(v) === 0) queue.push(v)
    }
  }

  const inTopo = new Set(topoOrder)
  const cyclicVertices = ids.filter((id) => !inTopo.has(id)).sort((a, b) => a - b)
  const cyclicSet = new Set(cyclicVertices)

  const cycles = []
  const visited = new Set()
  for (const s of cyclicVertices) {
    if (visited.has(s)) continue
    const stack = []
    const pos = new Map()
    const dfs = (u) => {
      visited.add(u)
      stack.push(u)
      pos.set(u, stack.length - 1)
      for (const v of adj.get(u)) {
        if (!cyclicSet.has(v)) continue
        if (pos.has(v)) cycles.push(stack.slice(pos.get(v)))
        else if (!visited.has(v)) dfs(v)
      }
      stack.pop()
      pos.delete(u)
    }
    dfs(s)
  }

  return {
    hasCycle: topoOrder.length < ids.length,
    topoOrder,
    cyclicVertices,
    cycles,
  }
}
