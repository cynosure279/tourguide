import initialData from './data.json'
import { buildTour } from '../algo/dfs'
import { detectCycle } from '../algo/cycle'
import { dijkstra, floydAll } from '../algo/shortest'
import { prim, kruskal } from '../algo/mst'

let db = loadInitial()

function loadInitial() {
  return {
    name: initialData.city || '合肥',
    attractions: initialData.attractions.map((a) => ({ ...a })),
    edges: initialData.edges.map((e) => ({ ...e })),
    nextId: Math.max(...initialData.attractions.map((a) => a.id)) + 1,
  }
}

function delay(ms = 120) {
  return new Promise((r) => setTimeout(r, ms))
}

function ok(data = null) {
  return Promise.resolve(data)
}

function fail(msg) {
  return Promise.reject(new Error(msg))
}

function defaultStart() {
  const ids = db.attractions.map((a) => a.id)
  if (ids.length === 0) throw new Error('当前没有景点')
  return Math.min(...ids)
}

function buildMatrix() {
  const ids = db.attractions.map((a) => a.id)
  const w = new Map()
  for (const e of db.edges) {
    w.set(`${e.from}-${e.to}`, e.distance)
    w.set(`${e.to}-${e.from}`, e.distance)
  }
  const matrix = ids.map((i) => ids.map((j) => (i === j ? 0 : w.get(`${i}-${j}`) ?? null)))
  return { ids, names: db.attractions.map((a) => a.name), matrix }
}

export async function mockRequest(method, url, body) {
  await delay()
  const [path, qs] = url.split('?')
  const q = Object.fromEntries(new URLSearchParams(qs || ''))
  const seg = path.split('/').filter(Boolean)
  try {
    switch (seg[1]) {
      case 'health':
        return ok({ status: 'mock' })

      case 'attractions': {
        if (method === 'GET') return ok(db.attractions.map((a) => ({ ...a })))
        if (method === 'POST') {
          const { code, name, intro = '', posx = 50, posy = 50 } = body || {}
          if (!code || !name) throw new Error('代号和名称不能为空')
          if (db.attractions.some((a) => a.code === code)) throw new Error(`代号已存在: ${code}`)
          const item = { id: db.nextId++, code, name, intro, posx, posy }
          db.attractions.push(item)
          return ok({ ...item })
        }
        if (method === 'DELETE') {
          const id = Number(seg[2])
          const idx = db.attractions.findIndex((a) => a.id === id)
          if (idx === -1) throw new Error(`景点不存在: ${id}`)
          db.attractions.splice(idx, 1)
          db.edges = db.edges.filter((e) => e.from !== id && e.to !== id)
          return ok()
        }
        break
      }

      case 'edges': {
        if (method === 'GET') return ok(db.edges.map((e) => ({ ...e })))
        if (method === 'POST') {
          const { from, to, distance } = body || {}
          if (!db.attractions.some((a) => a.id === from)) throw new Error(`景点不存在: ${from}`)
          if (!db.attractions.some((a) => a.id === to)) throw new Error(`景点不存在: ${to}`)
          if (from === to) throw new Error('道路两端不能是同一景点')
          if (!(distance > 0)) throw new Error('里程必须大于 0')
          if (db.edges.some((e) => (e.from === from && e.to === to) || (e.from === to && e.to === from)))
            throw new Error('该道路已存在')
          const item = { from, to, distance }
          db.edges.push(item)
          return ok({ ...item })
        }
        if (method === 'DELETE') {
          const u = Number(seg[2])
          const v = Number(seg[3])
          const idx = db.edges.findIndex(
            (e) => (e.from === u && e.to === v) || (e.from === v && e.to === u)
          )
          if (idx === -1) throw new Error(`道路不存在: ${u}-${v}`)
          db.edges.splice(idx, 1)
          return ok()
        }
        break
      }

      case 'graph': {
        if (seg[2] === 'matrix' && method === 'GET') return ok(buildMatrix())
        if (seg[2] === 'reset' && method === 'POST') {
          db = loadInitial()
          return ok()
        }
        // 导出预设：把当前景点/道路原样给出去
        if (seg[2] === 'export' && method === 'GET') {
          return ok({
            name: db.name,
            attractions: db.attractions.map((a) => ({ ...a })),
            edges: db.edges.map((e) => ({ ...e })),
          })
        }
        // 导入预设：用一个预设整体替换当前数据（和真实后端行为一致）
        if (seg[2] === 'import' && method === 'POST') {
          const { attractions = [], edges = [], name } = body || {}
          if (!Array.isArray(attractions) || !Array.isArray(edges))
            throw new Error('导入数据里 attractions/edges 应该是数组')
          const ids = attractions.map((a) => a.id)
          if (new Set(ids).size !== ids.length) throw new Error('景点 id 重复')
          db = {
            name: name || '未命名',
            attractions: attractions.map((a) => ({ ...a })),
            edges: edges.map((e) => ({ ...e })),
            nextId: ids.length ? Math.max(...ids) + 1 : 0,
          }
          return ok()
        }
        break
      }

      case 'route': {
        const start = q.start === undefined ? defaultStart() : Number(q.start)
        if (seg[2] === 'tour' && method === 'GET') {
          return ok(buildTour(db.attractions, db.edges, start))
        }
        if (seg[2] === 'cycle' && method === 'GET') {
          return ok(detectCycle(db.attractions, db.edges, start))
        }
        break
      }

      case 'path': {
        if (seg[2] === 'shortest' && method === 'GET') {
          // 五种算法在同一张图上结果一致，mock 里统一用内置 Dijkstra 计算
          const algos = ['dijkstra', 'dijkstra-heap', 'bellman-ford', 'spfa', 'floyd']
          if (q.algo && !algos.includes(q.algo)) throw new Error(`参数非法: algo=${q.algo}`)
          return ok(dijkstra(db.attractions, db.edges, Number(q.from), Number(q.to)))
        }
        if (seg[2] === 'all' && method === 'GET') {
          const f = floydAll(db.attractions, db.edges)
          return ok({ ids: f.ids, names: db.attractions.map((a) => a.name), matrix: f.matrix })
        }
        break
      }

      case 'roads': {
        if (seg[2] === 'mst' && method === 'GET') {
          return ok((q.algo || 'prim') === 'kruskal' ? kruskal(db.attractions, db.edges) : prim(db.attractions, db.edges))
        }
        break
      }
    }
    return fail(`未知接口: ${method} ${path}`)
  } catch (err) {
    return fail(err.message)
  }
}
