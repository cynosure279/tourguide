<template>
  <div>
    <el-row :gutter="16">
      <el-col :span="6" v-for="s in stats" :key="s.label">
        <el-card shadow="never" class="page-card">
          <div class="stat-value" :style="{ color: s.color }">{{ s.value }}</div>
          <div class="stat-label">{{ s.label }}</div>
        </el-card>
      </el-col>
    </el-row>

    <el-card shadow="never" class="page-card">
      <template #header>
        <div class="card-title"><el-icon aria-hidden="true"><MapLocation /></el-icon>{{ t('home.distribution') }}</div>
      </template>
      <GraphCanvas :attractions="store.attractions" :edges="store.edges" height="520px" />
      <div class="hint">{{ t('home.distHint') }}</div>
    </el-card>

    <el-card shadow="never">
      <template #header>
        <div class="card-title"><el-icon aria-hidden="true"><Menu /></el-icon>{{ t('home.functions') }}</div>
      </template>
      <el-row :gutter="12">
        <el-col :span="8" v-for="f in features" :key="f.path">
          <a
            class="feature-item"
            :href="f.path"
            role="link"
            :tabindex="0"
            :aria-label="`${f.name}：${f.desc}`"
            @click.prevent="$router.push(f.path)"
            @keydown.enter.prevent="$router.push(f.path)"
            @keydown.space.prevent="$router.push(f.path)"
          >
            <el-icon :size="20" :color="f.color" aria-hidden="true"><component :is="f.icon" /></el-icon>
            <div>
              <div class="feature-name">{{ f.name }}</div>
              <div class="hint">{{ f.desc }}</div>
            </div>
          </a>
        </el-col>
      </el-row>
    </el-card>
  </div>
</template>

<script setup>
import { computed, ref, watch, onBeforeUnmount } from 'vue'
import { store } from '../store/graph'
import { t } from '../i18n'
import { tone, cssVar, animEnabled } from '../store/theme'
import GraphCanvas from '../components/GraphCanvas.vue'

const totalMileage = computed(() => store.edges.reduce((s, e) => s + e.distance, 0).toFixed(1))

// 数字滚动：从 0 数到目标值（动画关掉时直接取目标值）
const display = ref({ attractions: 0, edges: 0, mileage: '0.0' })
let rafId = 0
function animateStats() {
  cancelAnimationFrame(rafId)
  const targets = {
    attractions: store.attractions.length,
    edges: store.edges.length,
    mileage: +totalMileage.value,
  }
  if (!animEnabled()) {
    display.value = { attractions: targets.attractions, edges: targets.edges, mileage: targets.mileage.toFixed(1) }
    return
  }
  const t0 = performance.now()
  const dur = 600
  const step = (now) => {
    const p = Math.min(1, (now - t0) / dur)
    const e = 1 - Math.pow(1 - p, 3)  // easeOutCubic
    display.value = {
      attractions: Math.round(targets.attractions * e),
      edges: Math.round(targets.edges * e),
      mileage: (targets.mileage * e).toFixed(1),
    }
    if (p < 1) rafId = requestAnimationFrame(step)
  }
  rafId = requestAnimationFrame(step)
}
watch([() => store.attractions.length, () => store.edges.length, () => totalMileage.value], animateStats, { immediate: true })
onBeforeUnmount(() => cancelAnimationFrame(rafId))

const stats = computed(() => [
  { label: t('home.statAttractions'), value: display.value.attractions, color: tone(56) },
  { label: t('home.statEdges'), value: display.value.edges, color: tone(44) },
  { label: t('home.statMileage'), value: display.value.mileage, color: tone(30) },
  {
    label: t('home.statMode'),
    value: store.mock ? t('home.modeMock') : t('home.modeOnline'),
    color: store.mock ? cssVar('--el-color-warning', '#e6a23c') : cssVar('--el-color-success', '#67c23a'),
  },
])

const features = [
  { path: '/manage', icon: 'Location', color: tone(56), name: t('home.f.manage'), desc: t('home.f.manageDesc') },
  { path: '/matrix', icon: 'Grid', color: tone(50), name: t('home.f.matrix'), desc: t('home.f.matrixDesc') },
  { path: '/tour', icon: 'Guide', color: tone(44), name: t('home.f.tour'), desc: t('home.f.tourDesc') },
  { path: '/cycle', icon: 'RefreshLeft', color: tone(60), name: t('home.f.cycle'), desc: t('home.f.cycleDesc') },
  { path: '/path', icon: 'Share', color: tone(38), name: t('home.f.path'), desc: t('home.f.pathDesc') },
  { path: '/mst', icon: 'Connection', color: tone(48), name: t('home.f.mst'), desc: t('home.f.mstDesc') },
]
</script>

<style scoped>
.feature-item {
  display: flex;
  gap: 12px;
  align-items: flex-start;
  padding: 14px;
  border: 1px solid var(--el-border-color-lighter);
  border-radius: 8px;
  margin-bottom: 12px;
  cursor: pointer;
  text-decoration: none;
  color: inherit;
  transition: border-color 0.2s, box-shadow 0.2s;
}

.feature-item:hover {
  border-color: var(--el-color-primary);
  box-shadow: 0 2px 12px var(--el-box-shadow-lighter);
}

.feature-name {
  font-weight: 600;
  font-size: 14px;
  margin-bottom: 2px;
}
</style>
