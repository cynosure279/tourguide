<template>
  <div>
    <el-card shadow="never" class="page-card">
      <template #header>
        <div class="card-title">
          <el-icon aria-hidden="true"><Location /></el-icon>{{ t('manage.title') }}
          <span class="hint" style="margin-left: auto">{{ t('manage.count', { n: store.attractions.length }) }}</span>
        </div>
      </template>

      <el-form :inline="true" :model="attrForm" class="add-form">
        <el-form-item :label="t('manage.addForm.code')">
          <el-input v-model="attrForm.code" :placeholder="t('manage.addForm.codePh')" style="width: 90px" />
        </el-form-item>
        <el-form-item :label="t('manage.addForm.name')">
          <el-input v-model="attrForm.name" :placeholder="t('manage.addForm.namePh')" style="width: 150px" />
        </el-form-item>
        <el-form-item :label="t('manage.addForm.intro')">
          <el-input v-model="attrForm.intro" :placeholder="t('manage.addForm.introPh')" style="width: 260px" />
        </el-form-item>
        <el-form-item :label="t('manage.addForm.x')">
          <el-input-number v-model="attrForm.posx" :min="0" :max="100" style="width: 100px" />
        </el-form-item>
        <el-form-item :label="t('manage.addForm.y')">
          <el-input-number v-model="attrForm.posy" :min="0" :max="100" style="width: 100px" />
        </el-form-item>
        <el-form-item>
          <el-button type="primary" :loading="adding" @click="onAddAttraction">{{ t('manage.addForm.add') }}</el-button>
        </el-form-item>
      </el-form>

      <el-table :data="store.attractions" size="small" border max-height="360">
        <el-table-column prop="id" :label="t('manage.col.id')" width="70" align="center" />
        <el-table-column prop="code" :label="t('manage.col.code')" width="80" align="center" />
        <el-table-column prop="name" :label="t('manage.col.name')" width="140" />
        <el-table-column prop="intro" :label="t('manage.col.intro')" show-overflow-tooltip />
        <el-table-column :label="t('manage.col.pos')" width="120" align="center">
          <template #default="{ row }">({{ row.posx }}, {{ row.posy }})</template>
        </el-table-column>
        <el-table-column :label="t('manage.col.op')" width="90" align="center">
          <template #default="{ row }">
            <el-button type="danger" link size="small" @click="onDeleteAttraction(row)">{{ t('manage.col.delete') }}</el-button>
          </template>
        </el-table-column>
      </el-table>
    </el-card>

    <el-card shadow="never" class="page-card">
      <template #header>
        <div class="card-title">
          <el-icon aria-hidden="true"><Connection /></el-icon>{{ t('manage.edges') }}
          <span class="hint" style="margin-left: auto">{{ t('manage.edgeCount', { n: store.edges.length }) }}</span>
        </div>
      </template>

      <el-form :inline="true" :model="edgeForm" class="add-form">
        <el-form-item :label="t('manage.edgeForm.from')">
          <el-select v-model="edgeForm.from" :placeholder="t('manage.edgeForm.from')" style="width: 150px">
            <el-option v-for="a in store.attractions" :key="a.id" :label="a.name" :value="a.id" />
          </el-select>
        </el-form-item>
        <el-form-item :label="t('manage.edgeForm.to')">
          <el-select v-model="edgeForm.to" :placeholder="t('manage.edgeForm.to')" style="width: 150px">
            <el-option v-for="a in store.attractions" :key="a.id" :label="a.name" :value="a.id" />
          </el-select>
        </el-form-item>
        <el-form-item :label="t('manage.edgeForm.dist')">
          <el-input-number v-model="edgeForm.distance" :min="0.1" :step="0.1" style="width: 130px" />
        </el-form-item>
        <el-form-item>
          <el-button type="primary" :loading="addingEdge" @click="onAddEdge">{{ t('manage.edgeForm.add') }}</el-button>
        </el-form-item>
      </el-form>

      <el-table :data="store.edges" size="small" border max-height="360">
        <el-table-column :label="t('manage.edgeCol.road')" min-width="220">
          <template #default="{ row }">
            {{ nameOf(row.from) }}（#{{ row.from }}） — {{ nameOf(row.to) }}（#{{ row.to }}）
          </template>
        </el-table-column>
        <el-table-column prop="distance" :label="t('manage.edgeCol.dist')" width="110" align="center" />
        <el-table-column :label="t('manage.edgeCol.delete')" width="90" align="center">
          <template #default="{ row }">
            <el-button type="danger" link size="small" @click="onDeleteEdge(row)">{{ t('manage.col.delete') }}</el-button>
          </template>
        </el-table-column>
      </el-table>
    </el-card>

    <el-card shadow="never">
      <div class="card-title"><el-icon aria-hidden="true"><RefreshLeft /></el-icon>{{ t('manage.maintenance') }}</div>
      <div class="preset-bar">
        <el-button type="warning" plain @click="onReset">{{ t('manage.reset') }}</el-button>
        <el-button @click="onExport">{{ t('manage.export') }}</el-button>
        <el-button @click="pickFile.click()">{{ t('manage.import') }}</el-button>
        <input ref="pickFile" type="file" accept=".json,application/json" style="display: none" @change="onImport" />
        <span class="hint" style="margin-left: 12px">{{ t('manage.current', { name: store.presetName || '合肥' }) }}</span>
      </div>
      <div class="hint" style="margin-top: 6px">{{ t('manage.maintHint') }}</div>
    </el-card>
  </div>
</template>

<script setup>
import { reactive, ref } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { store, refresh, nameOf } from '../store/graph'
import { t } from '../i18n'
import * as api from '../api'

const nextCode = () => 'A' + String(store.attractions.length + 1).padStart(2, '0')

const attrForm = reactive({ code: nextCode(), name: '', intro: '', posx: 50, posy: 50 })
const edgeForm = reactive({ from: null, to: null, distance: 1 })
const adding = ref(false)
const addingEdge = ref(false)
const pickFile = ref(null)

async function onAddAttraction() {
  if (!attrForm.code || !attrForm.name) {
    ElMessage.warning(t('manage.errEmpty'))
    return
  }
  adding.value = true
  try {
    await api.addAttraction({ ...attrForm })
    ElMessage.success(t('manage.added', { name: attrForm.name }))
    attrForm.code = nextCode()
    attrForm.name = ''
    attrForm.intro = ''
    await refresh()
  } catch (err) {
    ElMessage.error(err.message)
  } finally {
    adding.value = false
  }
}

async function onDeleteAttraction(row) {
  try {
    await ElMessageBox.confirm(t('manage.confirmDelAttr', { name: row.name }), t('manage.confirmDelete'), { type: 'warning' })
  } catch {
    return
  }
  try {
    await api.deleteAttraction(row.id)
    ElMessage.success(t('manage.deleted'))
    await refresh()
  } catch (err) {
    ElMessage.error(err.message)
  }
}

async function onAddEdge() {
  if (edgeForm.from === null || edgeForm.to === null) {
    ElMessage.warning(t('manage.selectEnds'))
    return
  }
  if (edgeForm.from === edgeForm.to) {
    ElMessage.warning(t('manage.sameEnds'))
    return
  }
  addingEdge.value = true
  try {
    await api.addEdge({ ...edgeForm })
    ElMessage.success(t('manage.edgeAdded'))
    await refresh()
  } catch (err) {
    ElMessage.error(err.message)
  } finally {
    addingEdge.value = false
  }
}

async function onDeleteEdge(row) {
  try {
    await ElMessageBox.confirm(t('manage.confirmDelEdge', { from: nameOf(row.from), to: nameOf(row.to) }), t('manage.confirmDelete'), { type: 'warning' })
  } catch {
    return
  }
  try {
    await api.deleteEdge(row.from, row.to)
    ElMessage.success(t('manage.deleted'))
    await refresh()
  } catch (err) {
    ElMessage.error(err.message)
  }
}

async function onReset() {
  try {
    await ElMessageBox.confirm(t('manage.confirmReset'), t('manage.resetTitle'), { type: 'warning' })
  } catch {
    return
  }
  try {
    await api.resetGraph()
    ElMessage.success(t('manage.resetDone'))
    await refresh()
    await loadPresetName()
  } catch (err) {
    ElMessage.error(err.message)
  }
}

function downloadJson(data, filename) {
  const blob = new Blob([JSON.stringify(data, null, 2)], { type: 'application/json' })
  const url = URL.createObjectURL(blob)
  const a = document.createElement('a')
  a.href = url
  a.download = filename
  document.body.appendChild(a)
  a.click()
  a.remove()
  URL.revokeObjectURL(url)
}

async function onExport() {
  try {
    const preset = await api.exportGraph()
    downloadJson(preset, `${preset.name || 'preset'}.json`)
    ElMessage.success(t('manage.exportDone'))
  } catch (err) {
    ElMessage.error(err.message)
  }
}

// 把一份预设应用起来（真实/模拟模式都走 api.importGraph，由后端或 mock 各自处理）
async function applyPreset(preset, label) {
  try {
    await ElMessageBox.confirm(t('manage.confirmPreset', { label }), t('manage.confirmLoad'), { type: 'warning' })
  } catch {
    return
  }
  try {
    await api.importGraph(preset)
    ElMessage.success(t('manage.loaded', { label }))
    await refresh()
    await loadPresetName()
  } catch (err) {
    ElMessage.error(err.message)
  }
}

async function onImport() {
  const file = pickFile.value && pickFile.value.files && pickFile.value.files[0]
  if (!file) return
  try {
    const text = await file.text()
    const preset = JSON.parse(text)
    if (!Array.isArray(preset.attractions) || !Array.isArray(preset.edges)) {
      throw new Error(t('manage.fileBad'))
    }
    await applyPreset(preset, file.name)
  } catch (err) {
    ElMessage.error(t('manage.importFail', { msg: err.message }))
  } finally {
    pickFile.value.value = ''
  }
}

// 用导出接口拿当前预设名，显示在页面上
async function loadPresetName() {
  try {
    const p = await api.exportGraph()
    store.presetName = p.name
  } catch {
    /* 拿不到就不显示 */
  }
}
</script>

<style scoped>
.add-form {
  margin-bottom: 8px;
}
</style>
