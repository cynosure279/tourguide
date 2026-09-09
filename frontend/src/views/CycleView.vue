<template>
  <div>
    <el-card shadow="never" class="page-card">
      <template #header>
        <div class="card-title"><el-icon aria-hidden="true"><RefreshLeft /></el-icon>{{ t('cycle.title') }}</div>
      </template>

      <el-form inline>
        <el-form-item :label="t('cycle.start')">
          <el-select v-model="start" style="width: 180px" @change="load">
            <el-option v-for="a in store.attractions" :key="a.id" :label="`${a.name}（#${a.id}）`" :value="a.id" />
          </el-select>
        </el-form-item>
        <el-form-item>
          <el-button type="primary" :loading="loading" @click="load">{{ t('cycle.detect') }}</el-button>
        </el-form-item>
      </el-form>

      <div class="hint" style="margin-bottom: 10px">{{ t('cycle.hint', { start }) }}</div>

      <template v-if="result">
        <el-alert
          :type="result.hasCycle ? 'error' : 'success'"
          :closable="false"
          show-icon
          style="margin-bottom: 14px"
        >
          <template v-if="result.hasCycle">
            {{ t('cycle.hasCycle', { n: result.cycles.length }) }}
            <span v-for="(c, i) in result.cycles" :key="i" class="cycle-chip">
              {{ c.map(nameOf).join(' → ') }} → {{ nameOf(c[0]) }}
            </span>
          </template>
          <template v-else>{{ t('cycle.noCycle') }}</template>
        </el-alert>

        <el-descriptions :column="2" border size="small">
          <el-descriptions-item :label="t('cycle.topo')">
            <span v-if="result.topoOrder.length">{{ result.topoOrder.map(nameOf).join(' → ') }}</span>
            <span v-else class="hint">{{ t('cycle.topoEmpty') }}</span>
          </el-descriptions-item>
          <el-descriptions-item :label="t('cycle.cyclic')">
            {{ result.cyclicVertices.length ? result.cyclicVertices.map(nameOf).join('、') : t('cycle.none') }}
          </el-descriptions-item>
        </el-descriptions>

        <div class="hint" style="margin-top: 10px">{{ t('cycle.note') }}</div>
      </template>
    </el-card>

    <el-card v-if="result" shadow="never">
      <template #header>
        <div class="card-title"><el-icon aria-hidden="true"><MapLocation /></el-icon>{{ t('cycle.viz') }}</div>
      </template>
      <GraphCanvas
        :attractions="store.attractions"
        :edges="store.edges"
        :highlight-edges="highlightEdges"
        :highlight-nodes="highlightNodes"
        height="560px"
      />
      <div class="hint">{{ t('cycle.vizHint') }}</div>
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

const start = ref(null)
const result = ref(null)
const loading = ref(false)

const highlightEdges = computed(() => {
  if (!result.value) return []
  const edges = []
  for (const c of result.value.cycles) {
    for (let i = 0; i < c.length; i++) {
      edges.push({ from: c[i], to: c[(i + 1) % c.length], directed: true, color: tone(40), width: 3 })
    }
  }
  return edges
})

const highlightNodes = computed(() => {
  if (!result.value) return {}
  const map = {}
  for (const id of result.value.cyclicVertices) map[id] = tone(40)
  return map
})

async function load() {
  if (start.value === null) return
  loading.value = true
  try {
    result.value = await api.getCycle(start.value)
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

<style scoped>
.cycle-chip {
  display: inline-block;
  background: var(--el-color-danger-light-9);
  color: var(--el-color-danger);
  border: 1px solid var(--el-color-danger-light-7);
  border-radius: 4px;
  padding: 1px 8px;
  margin: 2px 4px;
  font-size: 12px;
}
</style>
