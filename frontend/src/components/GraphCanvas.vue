<template>
  <!-- 外层 wrap 承载尺寸；内层 graph-canvas 专给 echarts 用（init 会清空其子元素），
       信息卡放在 wrap 里作为兄弟节点，避免被 echarts 清掉 -->
  <div class="graph-wrap" :style="{ height }">
    <div
      ref="el"
      class="graph-canvas"
      role="img"
      :aria-label="t('graph.aria', { nodes: attractions.length, edges: edges.length })"
    ></div>
    <transition name="node-card">
      <div v-if="hoveredAttraction" class="node-card" role="tooltip" aria-live="polite">
        <div class="node-card-title">
          {{ hoveredAttraction.name }}
          <span class="node-card-code">{{ hoveredAttraction.code }} · #{{ hoveredAttraction.id }}</span>
        </div>
        <div v-if="hoveredAttraction.intro" class="node-card-intro">{{ hoveredAttraction.intro }}</div>
      </div>
    </transition>
  </div>
</template>

<script setup>
import * as echarts from 'echarts'
import { ref, computed, onMounted, onBeforeUnmount, watch } from 'vue'
import { t } from '../i18n'
import { theme, isDark, softColor, animEnabled } from '../store/theme'
import logoUrl from '../assets/node-logo.png'

const props = defineProps({
  attractions: { type: Array, default: () => [] },
  edges: { type: Array, default: () => [] },
  highlightEdges: { type: Array, default: () => [] },
  highlightNodes: { type: Object, default: () => ({}) },
  height: { type: String, default: '540px' },
})

const el = ref(null)
let chart = null
let resizeObserver = null

// 白色 logo → 运行时染色：离屏 canvas 用 source-in 填充任意状态色，
// 每个节点按语义色（主色/入口/可达/不可达）拿到染好色的 logo 图
const logoImg = new Image()
let logoReady = false
logoImg.onload = () => {
  logoReady = true
  render()
}
logoImg.src = logoUrl

const tintCache = new Map()
function tintedLogo(color) {
  if (!logoReady) return ''
  if (tintCache.has(color)) return tintCache.get(color)
  const c = document.createElement('canvas')
  c.width = 128
  c.height = 128
  const ctx = c.getContext('2d')
  ctx.drawImage(logoImg, 0, 0, 128, 128)
  ctx.globalCompositeOperation = 'source-in'  // 只保留 logo 形状，填充目标色
  ctx.fillStyle = color
  ctx.fillRect(0, 0, 128, 128)
  const url = c.toDataURL('image/png')
  tintCache.set(color, url)
  return url
}

// 悬停的景点：用 zrender 的 mousemove（必然触发）+ e.target 反查节点，
// 信息卡固定在右上角，不遮挡图形
const hoveredId = ref(null)
const hoveredAttraction = computed(
  () => props.attractions.find((a) => a.id === hoveredId.value) || null
)

// 从 zrender 元素的内部属性里读出 ECharts 的事件数据。
// 注意：ECharts 5 把 dataType/dataIndex 存在 __ec_inner* 混淆属性里，
// 直接读 el.dataIndex 永远是 undefined（信息卡此前不出现的根因）
function ecDataOf(el) {
  for (const k of Object.keys(el)) {
    if (k.indexOf('__ec_inner') === 0) {
      const d = el[k]
      if (d && typeof d === 'object' && 'dataType' in d) return d
    }
  }
  return null
}

// 判断悬停目标是不是景点节点；边（line 类）和空白返回 null
function nodeIdFromElement(target) {
  let el = target
  let depth = 0
  while (el && depth < 5) {
    const info = ecDataOf(el)
    if (info) {
      if (info.dataType === 'node' && info.dataIndex != null) {
        const item = props.attractions[info.dataIndex]
        return item ? item.id : null
      }
      return null  // 命中的是边或其它数据元素
    }
    el = el.parent
    depth++
  }
  return null
}

// 节点的屏幕位置缓存：从 zrender 显示列表读世界矩阵平移分量。
// 门框镂空处命中测试会落空，悬停判断要靠邻近检测兜底
let nodeScreen = []  // [{id, x, y}]
function syncNodePositions() {
  if (!chart) return
  nodeScreen = []
  const dl = chart.getZr().storage.getDisplayList()
  for (const el of dl) {
    if (!el.transform) continue
    let info = null
    for (const k of Object.keys(el)) {
      if (k.indexOf('__ec_inner') === 0) {
        const d = el[k]
        if (d && typeof d === 'object' && d.dataType === 'node' && d.dataIndex != null) {
          info = d
          break
        }
      }
    }
    if (info) {
      const item = props.attractions[info.dataIndex]
      if (item) nodeScreen.push({ id: item.id, x: el.transform[4], y: el.transform[5] })
    }
  }
}

function onZrMousemove(e) {
  // 先看命中的元素（门框边框），镂空/空白处再用邻近检测兜底
  const byTarget = nodeIdFromElement(e.target)
  if (byTarget != null) {
    hoveredId.value = byTarget
    return
  }
  const x = e.offsetX ?? e.zrX
  const y = e.offsetY ?? e.zrY
  if (x == null || y == null) {
    hoveredId.value = null
    return
  }
  let best = null
  let bd = 24  // 门框半宽 + 余量
  for (const n of nodeScreen) {
    const d = Math.hypot(n.x - x, n.y - y)
    if (d < bd) {
      bd = d
      best = n
    }
  }
  hoveredId.value = best ? best.id : null
}

// 从 Element Plus 的 CSS 变量取当前主题色，保证图表跟随深浅色和主色
function css(name) {
  return getComputedStyle(document.documentElement).getPropertyValue(name).trim()
}

function render() {
  if (!chart) return
  const textColor = css('--el-text-color-primary') || '#606266'
  const hintColor = css('--el-text-color-secondary') || '#909399'
  const nodeColor = css('--el-color-primary') || '#5b8ff9'
  const edgeColor = softColor()  // 普通道路连线：淡主色，不再用中性灰

  const nodes = props.attractions.map((a) => {
    const color = props.highlightNodes[a.id] || nodeColor
    const node = {
      id: String(a.id),
      name: a.name,
      x: a.posx * 10,
      y: a.posy * 10,
      label: { show: true, position: 'bottom', fontSize: 12, color: textColor },
    }
    const logo = tintedLogo(color)
    if (logo) {
      node.symbol = 'image://' + logo
      node.symbolSize = [40, 40]
    } else {
      // logo 未加载完成时的兜底：普通圆点
      node.symbol = 'circle'
      node.symbolSize = 24
      node.itemStyle = { color }
    }
    return node
  })

  const links = props.edges.map((e) => ({
    source: String(e.from),
    target: String(e.to),
    lineStyle: { color: edgeColor, width: 1.5 },
    label: { show: true, formatter: String(e.distance), fontSize: 10, color: hintColor },
  }))

  for (const h of props.highlightEdges) {
    links.push({
      source: String(h.from),
      target: String(h.to),
      lineStyle: {
        color: h.color || '#f56c6c',
        width: h.width || 3.5,
        curveness: 0.22,
        type: h.dashed ? 'dashed' : 'solid',
      },
      symbol: h.directed ? ['none', 'arrow'] : 'circle',
      symbolSize: h.directed ? 9 : 0,
      label: h.label
        ? { show: true, formatter: h.label, fontSize: 11, color: h.color || '#f56c6c', fontWeight: 'bold' }
        : { show: false },
    })
  }

  chart.setOption(
    {
      aria: { enabled: true },
      // 动画开关：开 → 节点错峰弹入 + 更新平滑过渡；关 → 即时呈现
      animation: animEnabled(),
      animationDuration: animEnabled() ? 600 : 0,
      animationDelay: animEnabled() ? (idx) => idx * 8 : 0,
      animationDurationUpdate: animEnabled() ? 400 : 0,
      animationEasingUpdate: 'cubicOut',
      series: [
        {
          type: 'graph',
          layout: 'none',
          roam: true,
          data: nodes,
          links,
          emphasis: { focus: 'adjacency' },
          z: 3,
        },
      ],
    },
    true
  )
  syncNodePositions()  // 供悬停邻近检测使用（roam 后 finished 事件会再同步）
}

function onResize() {
  chart && chart.resize()
}

onMounted(() => {
  chart = echarts.init(el.value)
  // mousemove 一定会触发；门框镂空处靠邻近检测兜底
  chart.getZr().on('mousemove', onZrMousemove)
  // 漫游/动画结束后节点屏幕位置会变，重新同步
  chart.getZr().on('finished', syncNodePositions)
  render()
  resizeObserver = new ResizeObserver(onResize)
  resizeObserver.observe(el.value)
})

onBeforeUnmount(() => {
  resizeObserver && resizeObserver.disconnect()
  chart && chart.dispose()
  chart = null
  hoveredId.value = null
})

// 数据或主题（深浅色/主色）变化都重绘
watch(
  () => [props.attractions, props.edges, props.highlightEdges, props.highlightNodes, isDark(), theme.hue, theme.anim],
  () => {
    hoveredId.value = null
    render()
  },
  { deep: true }
)
</script>

<style scoped>
.graph-wrap {
  position: relative;
  width: 100%;
  min-height: 300px;
}

.graph-canvas {
  width: 100%;
  height: 100%;
}

/* 信息卡尺寸全部乘 --gcard-scale（外观设置里可调，默认 1.25） */
.node-card {
  position: absolute;
  top: 12px;
  right: 12px;
  max-width: calc(260px * var(--gcard-scale, 1.25));
  z-index: 20;
  padding: calc(10px * var(--gcard-scale, 1.25)) calc(12px * var(--gcard-scale, 1.25));
  border-radius: calc(10px * var(--gcard-scale, 1.25));
  background: var(--glass-bg-strong);
  -webkit-backdrop-filter: blur(20px) saturate(160%);
  backdrop-filter: blur(20px) saturate(160%);
  border: 1px solid var(--glass-border);
  box-shadow: var(--glass-shadow);
  pointer-events: none;
}

.node-card-title {
  font-size: calc(14px * var(--gcard-scale, 1.25));
  font-weight: 600;
  color: var(--el-text-color-primary);
}

.node-card-code {
  font-size: calc(11px * var(--gcard-scale, 1.25));
  font-weight: 400;
  color: var(--el-text-color-secondary);
  margin-left: 6px;
}

.node-card-intro {
  font-size: calc(12px * var(--gcard-scale, 1.25));
  line-height: 1.6;
  color: var(--el-text-color-secondary);
  margin-top: calc(4px * var(--gcard-scale, 1.25));
}

.node-card-enter-active,
.node-card-leave-active {
  transition: opacity 0.18s ease, transform 0.18s ease;
}

.node-card-enter-from,
.node-card-leave-to {
  opacity: 0;
  transform: translateY(-6px);
}

.hover-debug {
  position: fixed;
  top: 72px;
  left: 10px;
  z-index: 9999;
  font-size: 14px;
  font-weight: 600;
  font-family: Menlo, Consolas, monospace;
  color: var(--el-color-danger);
  background: var(--el-bg-color);
  border: 2px solid var(--el-color-danger);
  border-radius: 8px;
  padding: 6px 10px;
  pointer-events: none;
}
</style>
