<template>
  <div>
    <el-card shadow="never" class="page-card">
      <template #header>
        <div class="card-title"><el-icon aria-hidden="true"><Guide /></el-icon>{{ t('tour.title') }}</div>
      </template>

      <el-form inline>
        <el-form-item :label="t('tour.start')">
          <el-select v-model="start" style="width: 180px" @change="load">
            <el-option v-for="a in store.attractions" :key="a.id" :label="`${a.name}（#${a.id}）`" :value="a.id" />
          </el-select>
        </el-form-item>
        <el-form-item>
          <el-button type="primary" :loading="loading" @click="load">{{ t('tour.generate') }}</el-button>
        </el-form-item>
      </el-form>

      <div v-if="tour" class="hint" style="margin-bottom: 10px">{{ t('tour.hint', { start }) }}</div>

      <template v-if="tour">
        <el-alert
          v-if="tour.unreachable.length"
          type="warning"
          :closable="false"
          show-icon
          style="margin-bottom: 12px"
        >
          {{ t('tour.unreachable') }}<b>{{ tour.unreachable.map(nameOf).join('、') }}</b>
        </el-alert>

        <div class="card-title" style="font-size: 14px">{{ t('tour.walk', { n: tour.walk.length }) }}</div>
        <NodeFlow :ids="tour.walk" first-seen-only />

        <div class="card-title" style="font-size: 14px; margin-top: 14px">{{ t('tour.tourEdges', { n: tour.edges.length }) }}</div>
        <div class="hint" style="margin-bottom: 8px">{{ t('tour.tourHint') }}</div>
      </template>
    </el-card>

    <el-card v-if="tour" shadow="never">
      <template #header>
        <div class="card-title"><el-icon aria-hidden="true"><MapLocation /></el-icon>{{ t('tour.viz') }}</div>
      </template>
      <GraphCanvas
        :attractions="store.attractions"
        :edges="store.edges"
        :highlight-edges="highlightEdges"
        :highlight-nodes="highlightNodes"
        height="560px"
      />
      <div class="hint">{{ t('tour.vizHint') }}</div>
    </el-card>
  </div>
</template>

<script setup>
import { computed, ref, watch } from 'vue'
import { ElMessage } from 'element-plus'
import { store, nameOf } from '../store/graph'
import { t } from '../i18n'
import { tone, cssVar } from '../store/theme'
import * as api from '../api'
import GraphCanvas from '../components/GraphCanvas.vue'
import NodeFlow from '../components/NodeFlow.vue'

const start = ref(null)
const tour = ref(null)
const loading = ref(false)

const treeEdgeSet = computed(() => {
  const s = new Set()
  if (!tour.value) return s
  const w = tour.value.walk
  if (w.length === 0) return s
  const visited = new Set([w[0]])
  for (let i = 1; i < w.length; i++) {
    if (!visited.has(w[i])) {
      s.add(`${w[i - 1]}->${w[i]}`)
      visited.add(w[i])
    }
  }
  return s
})

const highlightEdges = computed(() => {
  if (!tour.value) return []
  return tour.value.edges.map((e) => {
    const isTree = treeEdgeSet.value.has(`${e.from}->${e.to}`)
    return {
      from: e.from,
      to: e.to,
      directed: true,
      color: isTree ? tone(45) : tone(78),
      dashed: !isTree,
      width: isTree ? 3 : 2,
    }
  })
})

const highlightNodes = computed(() => {
  if (!tour.value) return {}
  const map = {}
  map[tour.value.startId] = tone(34)
  const visited = new Set(tour.value.walk)
  for (const id of tour.value.unreachable) map[id] = cssVar('--el-color-danger', '#f56c6c')
  for (const a of store.attractions) {
    if (map[a.id] === undefined && visited.has(a.id)) map[a.id] = tone(72)
  }
  return map
})

async function load() {
  if (start.value === null) return
  loading.value = true
  try {
    tour.value = await api.getTourRoute(start.value)
  } catch (err) {
    ElMessage.error(err.message)
  } finally {
    loading.value = false
  }
}

watch(
  () => store.attractions,
  (list) => {
    if (list.length && (start.value === null || !list.some((a) => a.id === start.value))) {
      start.value = Math.min(...list.map((a) => a.id))
      load()
    }
  },
  { immediate: true }
)
</script>
