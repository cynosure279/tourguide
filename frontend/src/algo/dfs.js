import { buildAdjacency, vertexIds } from './graph'

/**
 * 深度优先策略生成导游线路图。
 * 语义（与 docs/API.md 3.1 一致）：
 *  - 邻点按景点 id 升序访问
 *  - walk 为含回溯的完整行走序列
 *  - edges 为 walk 相邻点构成的有向边（含回溯反向边），去重后输出，即导游线路图
 * 返回 { startId, walk, edges, unreachable }
 */
export function buildTour(attractions, edges, startId) {
  const ids = vertexIds(attractions)
  if (!ids.includes(startId)) {
    throw new Error(`景点不存在: ${startId}`)
  }
  const adj = buildAdjacency(attractions, edges)
  const visited = new Set()
  const walk = []

  const dfs = (u) => {
    visited.add(u)
    walk.push(u)
    for (const { to: v } of adj.get(u)) {
      if (visited.has(v)) continue
      dfs(v)
      walk.push(u)
    }
  }
  dfs(startId)

  const seenEdge = new Set()
  const tourEdges = []
  for (let i = 0; i + 1 < walk.length; i++) {
    const key = `${walk[i]}->${walk[i + 1]}`
    if (!seenEdge.has(key)) {
      seenEdge.add(key)
      tourEdges.push({ from: walk[i], to: walk[i + 1] })
    }
  }

  return {
    startId,
    walk,
    edges: tourEdges,
    unreachable: ids.filter((id) => !visited.has(id)),
  }
}
