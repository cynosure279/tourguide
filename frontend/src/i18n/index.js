import { reactive, computed } from 'vue'

import zhCnEl from 'element-plus/es/locale/lang/zh-cn'
import enEl from 'element-plus/es/locale/lang/en'
import jaEl from 'element-plus/es/locale/lang/ja'

import zhCN from './zh-CN'
import enUS from './en-US'
import jaJP from './ja-JP'

const messages = { 'zh-CN': zhCN, 'en-US': enUS, 'ja-JP': jaJP }
const elLocales = { 'zh-CN': zhCnEl, 'en-US': enEl, 'ja-JP': jaEl }

const state = reactive({ locale: 'zh-CN' })

function detect() {
  const saved = localStorage.getItem('tour-lang')
  if (saved && messages[saved]) return saved
  const nav = (navigator.language || 'zh-CN').toLowerCase()
  if (nav.startsWith('zh')) return 'zh-CN'
  if (nav.startsWith('ja')) return 'ja-JP'
  return 'en-US'
}

// 点号路径取值 + {x} 插值，如 t('path.dist', { v: 3.5 })
export function t(key, params) {
  const dict = messages[state.locale] || messages['zh-CN']
  const v = key.split('.').reduce((o, k) => (o == null ? o : o[k]), dict)
  if (typeof v !== 'string') return key
  if (!params) return v
  return v.replace(/\{(\w+)\}/g, (m, k) => (k in params ? params[k] : m))
}

export function setLocale(l) {
  if (!messages[l]) return
  state.locale = l
  localStorage.setItem('tour-lang', l)
  document.documentElement.lang = l === 'zh-CN' ? 'zh-CN' : l === 'ja-JP' ? 'ja' : 'en'
  document.title = t('app.title')
}

// 初始化语言（本地缓存 > 浏览器语言），返回当前语言
export function initI18n() {
  setLocale(detect())
  return state.locale
}

export const locale = computed(() => state.locale)
export const elLocale = computed(() => elLocales[state.locale])
