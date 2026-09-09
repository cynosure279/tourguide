<template>
  <div>
    <el-card shadow="never" class="page-card">
      <template #header>
        <div class="card-title"><el-icon aria-hidden="true"><Connection /></el-icon>{{ t('mst.title') }}</div>
      </template>

      <el-form inline>
        <el-form-item :label="t('mst.algo')">
          <el-radio-group v-model="algo" @change="load">
            <el-radio-button value="prim">{{ t('mst.prim') }}</el-radio-button>
            <el-radio-button value="kruskal">{{ t('mst.kruskal') }}</el-radio-button>
          </el-radio-group>
        </el-form-item>
        <el-form-item>
          <el-button type="primary" :loading="loading" @click="load">{{ t('mst.generate') }}</el-button>
          <el-button :loading="comparing" @click="compare">{{ t('mst.compare') }}</el-button>
        </el-form-item>
      </el-form>

      <div class="hint" style="margin-bottom: 10px">{{ t('mst.hint', { algo }) }}</div>

      <template v-if="result">
        <el-alert
          :type="result.spanning ? 'success' : 'warning'"
          :closable="false"
          show-icon
          style="margin-bottom: 14px"
          :title="
            result.spanning
              ? t('mst.done', { n: result.edges.length, m: store.attractions.length, v: result.totalCost })
              : t('mst.forest', { n: result.edges.length, v: result.totalCost })
          "
        />

        <el-table :data="result.edges" size="small" border max-height="300">
          <el-table-column type="index" :label="t('mst.colIdx')" width="60" align="center" />
          <el-table-column :label="t('mst.colRoad')" min-width="220">
            <template #default="{ row }">{{ nameOf(row.from) }}（#{{ row.from }}） — {{ nameOf(row.to) }}（#{{ row.to }}）</template>
          </el-table-column>
          <el-table-column prop="distance" :label="t('mst.colDist')" width="120" align="center" />
        </el-table>

        <div v-if="compareInfo" class="hint" style="margin-top: 12px">
          {{ t('mst.primCost', { v: compareInfo.prim }) }}；
          {{ t('mst.kruskalCost', { v: compareInfo.kruskal }) }}；
          <b>{{ Math.abs(compareInfo.prim - compareInfo.kruskal) < 1e-9 ? t('mst.consistent') : t('mst.inconsistent') }}</b>
        </div>
      </template>
    </el-card>

    <el-card v-if="result" shadow="never">
      <template #header>
        <div class="card-title"><el-icon aria-hidden="true"><MapLocation /></el-icon>{{ t('mst.viz') }}</div>
      </template>
      <GraphCanvas
        :attractions="store.attractions"
        :edges="store.edges"
        :highlight-edges="highlightEdges"
        :highlight-nodes="highlightNodes"
        height="560px"
      />
      <div class="hint">
        {{ t('mst.vizHint', { v: result.totalCost, saved: (store.edges.reduce((s, e) => s + e.distance, 0) - result.totalCost).toFixed(1) }) }}
      </div>
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

const algo = ref('prim')
const result = ref(null)
const loading = ref(false)
const comparing = ref(false)
const compareInfo = ref(null)

const highlightEdges = computed(() => {
  if (!result.value) return []
  return result.value.edges.map((e) => ({ ...e, color: tone(45), width: 4 }))
})

const highlightNodes = computed(() => {
  if (!result.value) return {}
  const map = {}
  const covered = new Set()
  for (const e of result.value.edges) {
    covered.add(e.from)
    covered.add(e.to)
  }
  for (const id of covered) map[id] = tone(45)
  return map
})

async function load() {
  loading.value = true
  try {
    result.value = await api.getMst(algo.value)
  } catch (err) {
    ElMessage.error(err.message)
  } finally {
    loading.value = false
  }
}

async function compare() {
  comparing.value = true
  try {
    const [p, k] = await Promise.all([api.getMst('prim'), api.getMst('kruskal')])
    compareInfo.value = { prim: p.totalCost, kruskal: k.totalCost }
  } catch (err) {
    ElMessage.error(err.message)
  } finally {
    comparing.value = false
  }
}

watch(
  () => store.attractions,
  (list) => {
    if (list.length && result.value === null) load()
  },
  { immediate: true }
)
</script>
