import { vertexIds } from './graph'

/**
 * Prim 算法求最小生成树（从 id 最小的顶点出发，O(n^2) 量级实现）。
 * 返回 { spanning, totalCost, edges }；图不连通时返回生成森林且 spanning=false。
 */
export function prim(attractions, edges) {
  const ids = vertexIds(attractions)
  if (ids.length === 0) return { spanning: true, totalCost: 0, edges: [] }

  const adj = new Map(ids.map((id) => [id, []]))
  for (const e of edges) {
    if (adj.has(e.from) && adj.has(e.to)) {
      adj.get(e.from).push(e)
      adj.get(e.to).push(e)
    }
  }

  const inTree = new Set()
  const mstEdges = []
  let total = 0
  const start = ids[0]
  inTree.add(start)

  while (inTree.size < ids.length) {
    let best = null
    for (const u of inTree) {
      for (const e of adj.get(u)) {
        const v = e.from === u ? e.to : e.from
        if (inTree.has(v)) continue
        if (best === null || e.distance < best.e.distance) best = { e, v }
      }
    }
    if (best === null) break
    inTree.add(best.v)
    mstEdges.push({ from: best.e.from, to: best.e.to, distance: best.e.distance })
    total += best.e.distance
  }

  return {
    spanning: inTree.size === ids.length,
    totalCost: round2(total),
    edges: mstEdges,
  }
}

/**
 * Kruskal 算法（并查集），返回结构同 prim，用于结果互验。
 */
export function kruskal(attractions, edges) {
  const ids = vertexIds(attractions)
  if (ids.length === 0) return { spanning: true, totalCost: 0, edges: [] }

  const parent = new Map(ids.map((id) => [id, id]))
  const find = (x) => {
    while (parent.get(x) !== x) {
      parent.set(x, parent.get(parent.get(x)))
      x = parent.get(x)
    }
    return x
  }
  const union = (a, b) => {
    const ra = find(a)
    const rb = find(b)
    if (ra === rb) return false
    parent.set(ra, rb)
    return true
  }

  const sorted = [...edges].sort((a, b) => a.distance - b.distance)
  const mstEdges = []
  let total = 0
  for (const e of sorted) {
    if (union(e.from, e.to)) {
      mstEdges.push({ from: e.from, to: e.to, distance: e.distance })
      total += e.distance
    }
    if (mstEdges.length === ids.length - 1) break
  }

  return {
    spanning: mstEdges.length === ids.length - 1,
    totalCost: round2(total),
    edges: mstEdges,
  }
}

function round2(x) {
  return Math.round(x * 100) / 100
}
