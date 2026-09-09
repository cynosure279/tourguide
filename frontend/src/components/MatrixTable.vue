<template>
  <div v-if="ids.length">
    <div class="sr-only">{{ t('matrixTable.caption', { rows: names.length, cols: names.length }) }}</div>
    <el-table
      :data="rows"
      size="small"
      border
      :max-height="maxHeight"
      :cell-style="cellStyle"
    >
      <el-table-column prop="name" width="132" align="center">
        <template #header>
          <span>{{ t('matrixTable.name') }}</span>
        </template>
      </el-table-column>
      <el-table-column v-for="(n, j) in names" :key="j" align="center" :width="92">
        <template #header>
          <span :title="`${n} (#${ids[j]})`">{{ n }}</span>
        </template>
        <template #default="{ $index }">
          {{ matrix[$index]?.[j] === null ? '—' : matrix[$index]?.[j] }}
        </template>
      </el-table-column>
    </el-table>
  </div>
  <el-empty v-else :description="t('common.empty')" :image-size="60" />
</template>

<script setup>
import { computed } from 'vue'
import { t } from '../i18n'
import { cssVar } from '../store/theme'

const props = defineProps({
  ids: { type: Array, default: () => [] },
  names: { type: Array, default: () => [] },
  matrix: { type: Array, default: () => [] },
  heat: { type: Boolean, default: false },
  maxHeight: { type: Number, default: 460 },
})

// 每行 = 一个景点 + 一行矩阵数据
const rows = computed(() =>
  props.names.map((n, i) => ({ name: n, cells: props.matrix[i] || [] }))
)

function maxValue() {
  let max = 0
  for (const row of props.matrix) for (const v of row) if (v !== null && v > max) max = v
  return max
}

function hexToRgb(hex) {
  const h = hex.replace('#', '')
  return [parseInt(h.slice(0, 2), 16), parseInt(h.slice(2, 4), 16), parseInt(h.slice(4, 6), 16)]
}

// 单元格着色：对角线/空占位/热力（热力色跟主题主色走）
function cellStyle({ rowIndex, columnIndex }) {
  if (columnIndex === 0) return null  // 名称列
  const i = rowIndex
  const j = columnIndex - 1
  const v = props.matrix[i]?.[j]
  if (i === j) {
    return { background: 'var(--el-fill-color-lighter)', color: 'var(--el-text-color-placeholder)' }
  }
  if (v === null || v === undefined) {
    return { color: 'var(--el-text-color-placeholder)' }
  }
  if (props.heat) {
    const alpha = Math.min(0.06 + (v / (maxValue() || 1)) * 0.5, 0.6)
    const [r, g, b] = hexToRgb(cssVar('--el-color-primary', '#409eff'))
    return { background: `rgba(${r}, ${g}, ${b}, ${alpha.toFixed(2)})` }
  }
  return null
}
</script>
