<template>
  <div>
    <el-card shadow="never" class="page-card">
      <template #header>
        <div class="card-title"><el-icon aria-hidden="true"><Share /></el-icon>{{ t('path.title') }}</div>
      </template>

      <el-form inline>
        <el-form-item :label="t('path.from')">
          <el-select v-model="from" style="width: 180px">
            <el-option v-for="a in store.attractions" :key="a.id" :label="`${a.name}（#${a.id}）`" :value="a.id" />
          </el-select>
        </el-form-item>
        <el-form-item :label="t('path.to')">
          <el-select v-model="to" style="width: 180px">
            <el-option v-for="a in store.attractions" :key="a.id" :label="`${a.name}（#${a.id}）`" :value="a.id" />
          </el-select>
        </el-form-item>
        <el-form-item :label="t('path.algo')">
          <el-radio-group v-model="algo" @change="query">
            <el-radio-button v-for="o in algoOptions" :key="o.value" :value="o.value">{{ o.label }}</el-radio-button>
          </el-radio-group>
        </el-form-item>
        <el-form-item>
          <el-button type="primary" :loading="loading" @click="query">{{ t('path.query') }}</el-button>
          <el-button plain :aria-label="t('path.swap')" @click="swap">{{ t('path.swap') }}</el-button>
          <el-button :loading="comparing" @click="compareAll">{{ t('path.compareAll') }}</el-button>
        </el-form-item>
      </el-form>

      <div class="hint" style="margin-bottom: 10px">{{ t('path.hint', { from, to, algo }) }}</div>

      <template v-if="result">
        <el-result
          v-if="result.reachable"
          icon="success"
          :title="t('path.dist', { v: result.distance })"
          style="padding: 12px 0"
        >
          <template #sub-title>
            <div style="text-align: left">
              <div class="hint" style="margin-bottom: 6px">{{ t('path.route', { n: result.path.length }) }}</div>
              <NodeFlow :ids="result.path" />
            </div>
          </template>
        </el-result>
        <el-alert
          v-else
          type="warning"
          :closable="false"
          show-icon
          :title="t('path.unreachable', { from: nameOf(from), to: nameOf(to) })"
        />

        <div v-if="compareInfo" class="hint" style="margin-top: 10px">
          {{ compareText }}
          <b>{{ allConsistent ? t('path.consistent') : t('path.inconsistent') }}</b>
        </div>
      </template>
    </el-card>

    <el-card v-if="result && result.reachable" shadow="never" class="page-card">
      <template #header>
        <div class="card-title"><el-icon aria-hidden="true"><MapLocation /></el-icon>{{ t('path.viz') }}</div>
      </template>
      <GraphCanvas
        :attractions="store.attractions"
        :edges="store.edges"
        :highlight-edges="highlightEdges"
        :highlight-nodes="highlightNodes"
        height="560px"
      />
      <div class="hint">{{ t('path.vizHint', { route: result.path.map(nameOf).join(' → '), v: result.distance }) }}</div>
    </el-card>

    <el-card shadow="never">
      <template #header>
        <div class="card-title">
          <el-icon aria-hidden="true"><Grid /></el-icon>{{ t('path.allTitle') }}
          <el-button style="margin-left: auto" size="small" :loading="allLoading" @click="loadAll">
            <el-icon aria-hidden="true"><Refresh /></el-icon>&nbsp;{{ t('path.allBtn') }}
          </el-button>
        </div>
      </template>
      <div v-if="allData" class="hint" style="margin-bottom: 12px">{{ t('path.allHint') }}</div>
      <MatrixTable v-if="allData" :ids="allData.ids" :names="allData.names" :matrix="allData.matrix" heat />
      <el-empty v-if="!allData && !allLoading" :description="t('path.allEmpty')" :image-size="80" />
    </el-card>
  </div>
</template>

<script setup>
import { computed, ref, watch } from 'vue'
import { ElMessage } from 'element-plus'
import { store, nameOf } from '../store/graph'
import { t } from '../i18n'
import { tone } from '../store/theme'
import * as api from '../api'
import GraphCanvas from '../components/GraphCanvas.vue'
import NodeFlow from '../components/NodeFlow.vue'
import MatrixTable from '../components/MatrixTable.vue'

const from = ref(null)
const to = ref(null)
const result = ref(null)
const loading = ref(false)
const allData = ref(null)
const allLoading = ref(false)

// 算法选择 + 多算法互验
const ALGOS = ['dijkstra', 'dijkstra-heap', 'bellman-ford', 'spfa', 'floyd']
const algo = ref('dijkstra')
const comparing = ref(false)
const compareInfo = ref(null)  // [{algo, reachable, distance}]

const algoOptions = computed(() => [
  { value: 'dijkstra', label: t('path.algoDijkstra') },
  { value: 'dijkstra-heap', label: t('path.algoDijkstraHeap') },
  { value: 'bellman-ford', label: t('path.algoBellmanFord') },
  { value: 'spfa', label: t('path.algoSpfa') },
  { value: 'floyd', label: t('path.algoFloyd') },
])

function algoLabel(name) {
  const o = algoOptions.value.find((x) => x.value === name)
  return o ? o.label : name
}

const compareText = computed(() => {
  if (!compareInfo.value) return ''
  return compareInfo.value
    .map((r) => algoLabel(r.algo) + '：' + (r.distance === null ? t('path.unreachableShort') : r.distance + ' km'))
    .join('；')
})

const allConsistent = computed(() => {
  if (!compareInfo.value || compareInfo.value.length === 0) return false
  const first = compareInfo.value[0]
  return compareInfo.value.every(
    (r) => r.reachable === first.reachable && Math.abs((r.distance ?? -1) - (first.distance ?? -1)) < 1e-6
  )
})

const highlightEdges = computed(() => {
  if (!result.value || !result.value.reachable) return []
  const p = result.value.path
  const edges = []
  for (let i = 0; i + 1 < p.length; i++) {
    edges.push({ from: p[i], to: p[i + 1], directed: true, color: tone(45), width: 4 })
  }
  return edges
})

const highlightNodes = computed(() => {
  if (!result.value || !result.value.reachable) return {}
  const map = {}
  for (const id of result.value.path) map[id] = tone(45)
  if (from.value !== null) map[from.value] = tone(30)
  if (to.value !== null) map[to.value] = tone(82)
  return map
})

async function query() {
  if (from.value === null || to.value === null) {
    ElMessage.warning(t('path.errSelect'))
    return
  }
  loading.value = true
  try {
    result.value = await api.getShortest(from.value, to.value, algo.value)
    compareInfo.value = null  // 换了算法或起终点，旧对比作废
  } catch (err) {
    ElMessage.error(err.message)
  } finally {
    loading.value = false
  }
}

// 把 5 种算法都跑一遍，互相验证最短距离一致
async function compareAll() {
  if (from.value === null || to.value === null) {
    ElMessage.warning(t('path.errSelect'))
    return
  }
  comparing.value = true
  try {
    const results = await Promise.all(ALGOS.map((a) => api.getShortest(from.value, to.value, a)))
    compareInfo.value = results.map((r, i) => ({
      algo: ALGOS[i],
      reachable: r.reachable,
      distance: r.reachable ? r.distance : null,
    }))
  } catch (err) {
    ElMessage.error(err.message)
  } finally {
    comparing.value = false
  }
}

function swap() {
  const t = from.value
  from.value = to.value
  to.value = t
}

async function loadAll() {
  allLoading.value = true
  try {
    allData.value = await api.getAllDistances()
  } catch (err) {
    ElMessage.error(err.message)
  } finally {
    allLoading.value = false
  }
}

watch(
  () => store.attractions,
  (list) => {
    if (list.length) {
      if (from.value === null) from.value = list[0].id
      if (to.value === null) to.value = list[Math.min(5, list.length - 1)].id
    }
  },
  { immediate: true }
)
</script>
