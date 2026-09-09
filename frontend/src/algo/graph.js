export function buildAdjacency(attractions, edges) {
  const ids = attractions.map((a) => a.id)
  const adj = new Map(ids.map((id) => [id, []]))
  for (const e of edges) {
    if (adj.has(e.from) && adj.has(e.to)) {
      adj.get(e.from).push({ to: e.to, w: e.distance })
      adj.get(e.to).push({ to: e.from, w: e.distance })
    }
  }
  for (const [, list] of adj) list.sort((a, b) => a.to - b.to)
  return adj
}

export function vertexIds(attractions) {
  return attractions.map((a) => a.id)
}
