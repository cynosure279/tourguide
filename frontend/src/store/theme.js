import { reactive } from 'vue'

const KEY_MODE = 'tour-theme-mode'
const KEY_HUE = 'tour-theme-hue'
const KEY_SCALE = 'tour-card-scale'
const KEY_ANIM = 'tour-anim'
const DEFAULT_HUE = 268  // 莫兰迪紫

const media = window.matchMedia('(prefers-color-scheme: dark)')

export const theme = reactive({
  mode: localStorage.getItem(KEY_MODE) || 'auto',
  hue: Number(localStorage.getItem(KEY_HUE) ?? DEFAULT_HUE),
  cardScale: Number(localStorage.getItem(KEY_SCALE) ?? 1.25),  // 信息卡缩放，默认 1.25x
  anim: localStorage.getItem(KEY_ANIM) !== 'false',            // 动画开关，默认开
})

export function isDark() {
  return theme.mode === 'dark' || (theme.mode === 'auto' && media.matches)
}

// 动画开关是否打开（GraphCanvas 的 canvas 动画没法用 CSS 关，需要读这个值）
export function animEnabled() {
  return theme.anim
}

// 色相按 5° 一档取整，并折回 0~355
function clampHue(h) {
  return (((Math.round(h / 5) * 5) % 360) + 360) % 360
}

// HSL 转 #rrggbb
export function hslToHex(h, s, l) {
  s /= 100
  l /= 100
  const k = (n) => (n + h / 30) % 12
  const a = s * Math.min(l, 1 - l)
  const f = (n) => l - a * Math.max(-1, Math.min(k(n) - 3, Math.min(9 - k(n), 1)))
  const to = (x) => Math.round(255 * x).toString(16).padStart(2, '0')
  return '#' + to(f(0)) + to(f(8)) + to(f(4))
}

// 读一个 CSS 变量（返回具体颜色值），图表/视图需要实色时用它
export function cssVar(name, fallback = '') {
  return getComputedStyle(document.documentElement).getPropertyValue(name).trim() || fallback
}

// 当前主题色相下的一个色调：视图里要和主色同色系但不同明暗时用它，
// 保证整页颜色都跟着主题色走。light=明度 0~100，sat=饱和度 0~100
export function tone(light, sat = 16) {
  return hslToHex(theme.hue, sat, light)
}

// 主色走莫兰迪风格：低饱和、中等明度；深色模式下提亮一档
export function baseColor() {
  return isDark() ? hslToHex(theme.hue, 17, 65) : hslToHex(theme.hue, 16, 56)
}

// 淡主色：图表里的普通道路连线用，比主色更淡、不抢眼，但和主题同色系
export function softColor() {
  return isDark() ? hslToHex(theme.hue, 14, 30) : hslToHex(theme.hue, 12, 72)
}

// 把两个 #rrggbb 按权重 w（0~1）混合
function mix(c1, c2, w) {
  const p = (s) => parseInt(s, 16)
  const h1 = c1.slice(1)
  const h2 = c2.slice(1)
  const r = Math.round(p(h1.slice(0, 2)) * (1 - w) + p(h2.slice(0, 2)) * w)
  const g = Math.round(p(h1.slice(2, 4)) * (1 - w) + p(h2.slice(2, 4)) * w)
  const b = Math.round(p(h1.slice(4, 6)) * (1 - w) + p(h2.slice(4, 6)) * w)
  return '#' + [r, g, b].map((x) => x.toString(16).padStart(2, '0')).join('')
}

// 把主题实际生效：html 上挂 dark 类 + 覆盖 Element Plus 主色变量。
// light-N 在浅色模式下往白混、深色模式下往黑混，保证两种模式下都有合适对比
export function applyTheme() {
  const dark = isDark()
  const root = document.documentElement
  root.classList.toggle('dark', dark)
  // 动画开关：关掉时给根元素挂 no-anim 类，CSS 动画/过渡全部归零
  root.classList.toggle('no-anim', !theme.anim)

  const base = baseColor()
  const toward = dark ? '#1d1e1f' : '#ffffff'
  const set = (n, v) => root.style.setProperty(n, v)
  set('--el-color-primary', base)
  set('--el-color-primary-light-3', mix(base, toward, 0.3))
  set('--el-color-primary-light-5', mix(base, toward, 0.5))
  set('--el-color-primary-light-7', mix(base, toward, 0.7))
  set('--el-color-primary-light-8', mix(base, toward, 0.8))
  set('--el-color-primary-light-9', mix(base, toward, 0.9))
  set('--el-color-primary-dark-2', mix(base, dark ? '#ffffff' : '#000000', 0.2))
  // 信息卡缩放（GraphCanvas 的卡片尺寸全部用 calc 乘这个系数）
  set('--gcard-scale', String(theme.cardScale))
}

export function setAnim(v) {
  theme.anim = !!v
  localStorage.setItem(KEY_ANIM, String(theme.anim))
  applyTheme()
}

export function setCardScale(v) {
  // 吸附到 0.05 一档，并限制在 0.75~2 之间
  theme.cardScale = Math.min(2, Math.max(0.75, Math.round(v * 20) / 20))
  localStorage.setItem(KEY_SCALE, String(theme.cardScale))
  applyTheme()
}

export function setThemeHue(h) {
  theme.hue = clampHue(h)
  localStorage.setItem(KEY_HUE, String(theme.hue))
  applyTheme()
}

export function stepHue(d) {
  setThemeHue(theme.hue + d)
}

export function setThemeMode(m) {
  theme.mode = m
  localStorage.setItem(KEY_MODE, m)
  applyTheme()
}

// 应用时调用一次，并监听系统主题变化（仅"自动"模式下生效）
export function initTheme() {
  applyTheme()
  media.addEventListener('change', () => {
    if (theme.mode === 'auto') applyTheme()
  })
}
