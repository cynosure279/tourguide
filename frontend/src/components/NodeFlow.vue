<template>
  <div class="node-flow">
    <template v-for="(id, i) in ids" :key="i">
      <el-tag
        :type="typeOf(id, i)"
        :effect="typeOf(id, i) === 'info' ? 'plain' : 'dark'"
        size="default"
      >
        {{ nameOf(id) }}<span class="node-code">&nbsp;#{{ id }}</span>
      </el-tag>
      <span v-if="i < ids.length - 1" class="arrow">➜</span>
    </template>
  </div>
</template>

<script setup>
import { computed } from 'vue'
import { nameOf } from '../store/graph'

const props = defineProps({
  ids: { type: Array, default: () => [] },
  firstSeenOnly: { type: Boolean, default: false },
})

const seenBefore = computed(() => {
  const seen = new Set()
  const res = new Set()
  for (const id of props.ids) {
    if (seen.has(id)) res.add(id)
    seen.add(id)
  }
  return res
})

function typeOf(id, i) {
  if (props.firstSeenOnly && seenBefore.value.has(id)) return 'info'
  if (i === props.ids.length - 1 && i > 0) return 'success'
  return 'primary'
}
</script>

<style scoped>
.node-code {
  font-size: 11px;
  opacity: 0.75;
}
</style>
