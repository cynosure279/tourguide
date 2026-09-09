import { mockRequest } from './mock'

const BASE = import.meta.env.VITE_API_BASE || ''

const state = { mock: false }

export function setMock(v) {
  state.mock = v
}

export function isMock() {
  return state.mock
}

export async function request(method, url, body, timeoutMs = 8000) {
  if (state.mock) return mockRequest(method, url, body)
  const ctrl = new AbortController()
  const timer = setTimeout(() => ctrl.abort(), timeoutMs)
  try {
    let res
    try {
      res = await fetch(BASE + url, {
        method,
        headers: { 'Content-Type': 'application/json' },
        body: body === undefined ? undefined : JSON.stringify(body),
        signal: ctrl.signal,
      })
    } catch (err) {
      if (err.name === 'AbortError') {
        throw new Error('请求超时：请确认后端已启动（localhost:8080）')
      }
      throw new Error('无法连接后端：请确认后端已启动（localhost:8080）')
    }
    let json
    try {
      json = await res.json()
    } catch {
      throw new Error(
        `后端响应异常（HTTP ${res.status}，非 JSON）：请确认 localhost:8080 上运行的是本系统后端`
      )
    }
    if (json.code !== 0) throw new Error(json.msg || `请求失败（HTTP ${res.status}）`)
    return json.data
  } finally {
    clearTimeout(timer)
  }
}
