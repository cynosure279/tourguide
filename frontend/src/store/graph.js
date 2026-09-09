import { reactive } from 'vue'
import { ElMessage } from 'element-plus'
import * as api from '../api'
import { t } from '../i18n'

export const store = reactive({
  attractions: [],
  edges: [],
  presetName: '',
  loaded: false,
  loading: false,
  online: false,
  mock: false,
  detecting: true,
})

export function nameOf(id) {
  const a = store.attractions.find((x) => x.id === id)
  return a ? a.name : `#${id}`
}

export function codeOf(id) {
  const a = store.attractions.find((x) => x.id === id)
  return a ? a.code : `#${id}`
}

export async function refresh() {
  store.loading = true
  try {
    const [attractions, edges] = await Promise.all([api.getAttractions(), api.getEdges()])
    store.attractions = attractions
    store.edges = edges
    store.loaded = true
  } finally {
    store.loading = false
  }
}

// 探测后端：8080 可用则使用真实后端，否则自动回退到模拟模式。
// 任何异常都不会中断（finally 保证 detecting 复位）
export async function detectBackend(silent = false) {
  store.detecting = true
  store.online = false
  store.mock = false
  api.setMock(false)
  try {
    await api.getHealth()
    store.online = true
    store.mock = false
  } catch {
    store.online = false
    store.mock = true
    api.setMock(true)
    if (!silent) {
      ElMessage.warning(t('app.backendNotFound'))
    }
  } finally {
    store.detecting = false
  }
  try {
    await refresh()
  } catch (err) {
    ElMessage.error(t('app.loadFailMsg', { msg: err.message }))
  }
}

// 手动切换数据来源。切到真实模式前先探测，失败则回退模拟模式，保证界面始终可用
export async function setMockMode(v) {
  if (v) {
    store.mock = true
    store.online = false
    api.setMock(true)
    try {
      await refresh()
      ElMessage.success(t('app.mockSwitched'))
    } catch (err) {
      ElMessage.error(err.message)
    }
    return
  }
  api.setMock(false)
  try {
    await api.getHealth()
  } catch (err) {
    store.mock = true
    store.online = false
    api.setMock(true)
    ElMessage.error(t('app.connFail', { msg: err.message }))
    await refresh().catch(() => {})
    return
  }
  store.mock = false
  store.online = true
  try {
    await refresh()
    ElMessage.success(t('app.connOk'))
  } catch (err) {
    ElMessage.error(err.message)
  }
}
