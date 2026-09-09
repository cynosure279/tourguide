import { vertexIds } from './graph'

/**
 * Dijkstra 求单源最短路径（O(n^2) 朴素实现）。
 * 返回 { reachable, distance, path }，path 含起终点；
 * from === to 时 distance=0、path=[from]；不可达时 reachable=false。
 */
export function dijkstra(attractions, edges, from, to) {
  const ids = vertexIds(attractions)
  if (!ids.includes(from)) throw new Error(`景点不存在: ${from}`)
  if (!ids.includes(to)) throw new Error(`景点不存在: ${to}`)

  const weight = new Map()
  for (const e of edges) {
    if (!weight.has(`${e.from}|${e.to}`)) {
      weight.set(`${e.from}|${e.to}`, e.distance)
      weight.set(`${e.to}|${e.from}`, e.distance)
    }
  }
  const neighbors = (u) => ids.filter((v) => weight.has(`${u}|${v}`))

  const dist = new Map(ids.map((id) => [id, Infinity]))
  const prev = new Map()
  const done = new Set()
  dist.set(from, 0)

  for (;;) {
    let u = null
    let best = Infinity
    for (const id of ids) {
      if (!done.has(id) && dist.get(id) < best) {
        best = dist.get(id)
        u = id
      }
    }
    if (u === null) break
    done.add(u)
    for (const v of neighbors(u)) {
      if (done.has(v)) continue
      const nd = dist.get(u) + weight.get(`${u}|${v}`)
      if (nd < dist.get(v)) {
        dist.set(v, nd)
        prev.set(v, u)
      }
    }
  }

  if (!Number.isFinite(dist.get(to))) {
    return { reachable: false, distance: null, path: [] }
  }
  const path = [to]
  let cur = to
  while (cur !== from) {
    cur = prev.get(cur)
    path.push(cur)
  }
  path.reverse()
  return { reachable: true, distance: round2(dist.get(to)), path }
}

/**
 * Floyd 求全源最短路径。
 * 返回 { ids, matrix, next }：matrix[i][j] 为最短距离（null=不可达，对角线0）；
 * next 为路径还原矩阵，queryPath(next, ids, i, j) 还原 i→j 途经顶点。
 */
export function floydAll(attractions, edges) {
  const ids = vertexIds(attractions)
  const n = ids.length
  const INF = Infinity
  const dist = Array.from({ length: n }, (_, i) =>
    Array.from({ length: n }, (_, j) => (i === j ? 0 : INF))
  )
  const next = Array.from({ length: n }, () => Array(n).fill(-1))
  const idx = new Map(ids.map((id, i) => [id, i]))

  for (const e of edges) {
    if (!idx.has(e.from) || !idx.has(e.to)) continue
    const i = idx.get(e.from)
    const j = idx.get(e.to)
    if (e.distance < dist[i][j]) {
      dist[i][j] = e.distance
      dist[j][i] = e.distance
      next[i][j] = j
      next[j][i] = i
    }
  }

  for (let k = 0; k < n; k++) {
    for (let i = 0; i < n; i++) {
      if (dist[i][k] === INF) continue
      for (let j = 0; j < n; j++) {
        if (dist[k][j] === INF) continue
        if (dist[i][k] + dist[k][j] < dist[i][j]) {
          dist[i][j] = dist[i][k] + dist[k][j]
          next[i][j] = next[i][k]
        }
      }
    }
  }

  const matrix = dist.map((row) => row.map((d) => (d === INF ? null : round2(d))))
  return { ids, matrix, next }
}

export function queryPath(floyd, from, to) {
  const idx = floyd.idx ?? new Map(floyd.ids.map((id, i) => [id, i]))
  const i = idx.get(from)
  const j = idx.get(to)
  if (i === undefined || j === undefined) throw new Error('景点不存在')
  if (floyd.matrix[i][j] === null) return { reachable: false, distance: null, path: [] }
  const path = [i]
  let cur = i
  while (cur !== j) {
    cur = floyd.next[cur][j]
    path.push(cur)
  }
  return {
    reachable: true,
    distance: floyd.matrix[i][j],
    path: path.map((k) => floyd.ids[k]),
  }
}

function round2(x) {
  return Math.round(x * 100) / 100
}
