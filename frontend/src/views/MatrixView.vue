<template>
  <el-card shadow="never">
    <template #header>
      <div class="card-title">
        <el-icon aria-hidden="true"><Grid /></el-icon>{{ t('matrix.title') }}
        <el-button style="margin-left: auto" size="small" :loading="loading" :aria-label="t('matrix.refresh')" @click="load">
          <el-icon aria-hidden="true"><Refresh /></el-icon>&nbsp;{{ t('matrix.refresh') }}
        </el-button>
      </div>
    </template>

    <div v-if="data" class="hint" style="margin-bottom: 12px">{{ t('matrix.hint') }}</div>

    <MatrixTable v-if="data" :ids="data.ids" :names="data.names" :matrix="data.matrix" />

    <el-empty v-if="!data && !loading" :description="t('matrix.empty')" :image-size="80" />
  </el-card>
</template>

<script setup>
import { onMounted, ref } from 'vue'
import * as api from '../api'
import { t } from '../i18n'
import MatrixTable from '../components/MatrixTable.vue'

const data = ref(null)
const loading = ref(false)

async function load() {
  loading.value = true
  try {
    data.value = await api.getMatrix()
  } finally {
    loading.value = false
  }
}

onMounted(load)
</script>
