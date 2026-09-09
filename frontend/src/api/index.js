import { request } from './client'

export { setMock, isMock } from './client'

export const getHealth = () => request('GET', '/api/health', undefined, 2500)
export const getAttractions = () => request('GET', '/api/attractions')
export const addAttraction = (a) => request('POST', '/api/attractions', a)
export const deleteAttraction = (id) => request('DELETE', `/api/attractions/${id}`)
export const getEdges = () => request('GET', '/api/edges')
export const addEdge = (e) => request('POST', '/api/edges', e)
export const deleteEdge = (u, v) => request('DELETE', `/api/edges/${u}/${v}`)
export const getMatrix = () => request('GET', '/api/graph/matrix')
export const getTourRoute = (start) => request('GET', `/api/route/tour?start=${start}`)
export const getCycle = (start) => request('GET', `/api/route/cycle?start=${start}`)
export const getShortest = (from, to, algo) => {
  const extra = algo ? `&algo=${algo}` : ''
  return request('GET', `/api/path/shortest?from=${from}&to=${to}${extra}`)
}
export const getAllDistances = () => request('GET', '/api/path/all')
export const getMst = (algo) => request('GET', `/api/roads/mst?algo=${algo}`)
export const resetGraph = () => request('POST', '/api/graph/reset')
export const exportGraph = () => request('GET', '/api/graph/export')
export const importGraph = (preset) => request('POST', '/api/graph/import', preset)
