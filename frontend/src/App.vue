<template>
  <el-config-provider :locale="elLocale">
    <el-container class="app">
      <el-header class="app-header">
        <div class="brand">
          <el-icon :size="22" :color="primaryColor" aria-hidden="true"><MapLocation /></el-icon>
          <span class="brand-title">{{ t('app.title') }}</span>
          <span class="brand-sub">{{ t('app.subtitle') }}</span>
        </div>
        <div class="header-right">
          <el-tag
            :type="headerTag.type"
            effect="dark"
            size="small"
            :class="{ pulsing: store.detecting }"
          >
            {{ headerTag.text }}
          </el-tag>
          <el-tooltip :content="t('app.tipMock')" placement="bottom">
            <el-switch
              v-model="store.mock"
              :disabled="store.detecting"
              inline-prompt
              :active-text="t('app.mock')"
              :inactive-text="t('app.real')"
              :aria-label="t('app.tipMock')"
              @change="onMockChange"
            />
          </el-tooltip>
          <el-tooltip :content="t('app.tipRefresh')" placement="bottom">
            <el-button circle size="small" :loading="store.detecting" :aria-label="t('app.tipRefresh')" @click="detectBackend()">
              <el-icon v-if="!store.detecting" aria-hidden="true"><Refresh /></el-icon>
            </el-button>
          </el-tooltip>

          <el-popover placement="bottom-end" :width="270" trigger="click">
            <template #reference>
              <el-button circle size="small" :aria-label="t('settings.title')">
                <el-icon aria-hidden="true"><Setting /></el-icon>
              </el-button>
            </template>
            <div class="settings-panel">
              <div class="settings-label">{{ t('settings.theme') }}</div>
              <div
                class="theme-bar"
                role="slider"
                :aria-label="t('settings.theme')"
                :aria-valuemin="0"
                :aria-valuemax="355"
                :aria-valuenow="theme.hue"
                :aria-valuetext="t('settings.hueLabel', { h: theme.hue })"
                tabindex="0"
                @keydown.left.prevent="stepHue(-5)"
                @keydown.down.prevent="stepHue(-5)"
                @keydown.right.prevent="stepHue(5)"
                @keydown.up.prevent="stepHue(5)"
                @keydown.home.prevent="setThemeHue(0)"
                @keydown.end.prevent="setThemeHue(355)"
                @pointerdown.prevent="onBarDown"
                @pointermove="onBarMove"
                @pointerup="dragging = false"
                @pointerleave="dragging = false"
              >
                <span class="theme-bar-marker" :style="{ left: markerPos }" aria-hidden="true"></span>
              </div>
              <div class="theme-selected">
                <span class="theme-chip" :style="{ background: currentColor }" aria-hidden="true"></span>
                <span>{{ t('settings.hueLabel', { h: theme.hue }) }}</span>
              </div>

              <div class="settings-label">{{ t('settings.cardSize') }}</div>
              <el-slider
                :model-value="theme.cardScale"
                :min="0.75"
                :max="2"
                :step="0.05"
                :format-tooltip="(v) => Math.round(v * 100) + '%'"
                :aria-label="t('settings.cardSize')"
                @input="setCardScale"
              />

              <div class="settings-row">
                <span>{{ t('settings.anim') }}</span>
                <el-switch :model-value="theme.anim" :aria-label="t('settings.anim')" @change="setAnim" />
              </div>

              <div class="settings-label">{{ t('settings.mode') }}</div>
              <el-segmented :model-value="theme.mode" :options="modeOptions" @change="setThemeMode" />

              <div class="settings-label">{{ t('settings.lang') }}</div>
              <el-segmented :model-value="locale" :options="langOptions" @change="onLangChange" />
            </div>
          </el-popover>
        </div>
      </el-header>
      <el-container class="app-body">
        <el-aside width="216px" class="app-aside">
          <el-menu router :default-active="route.path" class="app-menu">
            <el-menu-item index="/"><el-icon aria-hidden="true"><HomeFilled /></el-icon>{{ t('nav.home') }}</el-menu-item>
            <el-menu-item index="/manage"><el-icon aria-hidden="true"><Location /></el-icon>{{ t('nav.manage') }}</el-menu-item>
            <el-menu-item index="/matrix"><el-icon aria-hidden="true"><Grid /></el-icon>{{ t('nav.matrix') }}</el-menu-item>
            <el-menu-item index="/tour"><el-icon aria-hidden="true"><Guide /></el-icon>{{ t('nav.tour') }}</el-menu-item>
            <el-menu-item index="/cycle"><el-icon aria-hidden="true"><RefreshLeft /></el-icon>{{ t('nav.cycle') }}</el-menu-item>
            <el-menu-item index="/path"><el-icon aria-hidden="true"><Share /></el-icon>{{ t('nav.path') }}</el-menu-item>
            <el-menu-item index="/mst"><el-icon aria-hidden="true"><Connection /></el-icon>{{ t('nav.mst') }}</el-menu-item>
          </el-menu>
        </el-aside>
        <el-main class="app-main">
          <router-view v-slot="{ Component }">
            <transition name="page" mode="out-in">
              <component :is="Component" v-if="store.loaded" />
            </transition>
          </router-view>
          <div v-if="!store.loaded && (store.detecting || store.loading)" class="loading-box">
            <el-icon class="is-loading" :size="28" aria-hidden="true"><Loading /></el-icon>
            <span>{{ store.detecting ? t('app.loadingDetect') : t('app.loadingData') }}</span>
          </div>
          <el-empty v-if="!store.loaded && !store.detecting && !store.loading" :description="t('app.loadFailed')">
            <el-button type="primary" @click="detectBackend()">{{ t('app.retry') }}</el-button>
          </el-empty>
        </el-main>
      </el-container>
    </el-container>
  </el-config-provider>
</template>

<script setup>
import { computed, onMounted, ref, watchEffect } from 'vue'
import { useRoute } from 'vue-router'
import { store, detectBackend, setMockMode } from './store/graph'
import { t, elLocale, locale, setLocale } from './i18n'
import { theme, setThemeMode, setThemeHue, stepHue, setCardScale, setAnim, baseColor } from './store/theme'

const route = useRoute()

onMounted(() => detectBackend(true))

// 页面标题跟随语言和路由
watchEffect(() => {
  const name = route.meta.title ? t(route.meta.title) : ''
  document.title = name ? `${t('app.title')} - ${name}` : t('app.title')
})

const primaryColor = computed(() => {
  // 读取 theme 的两个响应式字段，让颜色跟随主题即时变化
  theme.hue
  theme.mode
  return getCss('--el-color-primary') || '#8e7f9f'
})

const headerTag = computed(() => {
  if (store.detecting) return { type: 'info', text: t('app.tagDetecting') }
  if (store.mock) return { type: 'warning', text: t('app.tagMock') }
  if (store.online) return { type: 'success', text: t('app.tagOnline') }
  return { type: 'danger', text: t('app.tagOffline') }
})

const modeOptions = computed(() => [
  { label: t('settings.modeAuto'), value: 'auto' },
  { label: t('settings.modeLight'), value: 'light' },
  { label: t('settings.modeDark'), value: 'dark' },
])

// 主题色条：连续色相渐变，点击/拖动取色
const dragging = ref(false)
const currentColor = computed(() => {
  theme.hue
  theme.mode
  return baseColor()
})
const markerPos = computed(() => {
  theme.hue
  return (theme.hue / 360) * 100 + '%'
})

function hueFromPointer(e) {
  const rect = e.currentTarget.getBoundingClientRect()
  const ratio = Math.min(1, Math.max(0, (e.clientX - rect.left) / rect.width))
  setThemeHue(Math.min(355, Math.round((ratio * 355) / 5) * 5))
}

function onBarDown(e) {
  dragging.value = true
  hueFromPointer(e)
}

function onBarMove(e) {
  if (dragging.value) hueFromPointer(e)
}

const langOptions = [
  { label: '简体中文', value: 'zh-CN' },
  { label: 'English', value: 'en-US' },
  { label: '日本語', value: 'ja-JP' },
]

function onLangChange(v) {
  setLocale(v)
}

function onMockChange(v) {
  setMockMode(v)
}

function getCss(name) {
  return getComputedStyle(document.documentElement).getPropertyValue(name).trim()
}
</script>

<style scoped>
.app {
  height: 100vh;
}

.app-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  background: var(--glass-bg);
  -webkit-backdrop-filter: blur(16px) saturate(150%);
  backdrop-filter: blur(16px) saturate(150%);
  border-bottom: 1px solid var(--glass-border);
  box-shadow: var(--glass-shadow);
}

.brand {
  display: flex;
  align-items: center;
  gap: 8px;
}

.brand-title {
  font-size: 17px;
  font-weight: 700;
}

.brand-sub {
  font-size: 12px;
  color: var(--el-text-color-secondary);
  margin-left: 4px;
}

.header-right {
  display: flex;
  align-items: center;
  gap: 12px;
}

.app-body {
  height: calc(100vh - 60px);
}

.app-aside {
  background: var(--glass-bg);
  -webkit-backdrop-filter: blur(16px) saturate(150%);
  backdrop-filter: blur(16px) saturate(150%);
  border-right: 1px solid var(--glass-border);
}

.app-menu {
  border-right: none;
  padding-top: 8px;
  background: transparent;
}

.app-main {
  overflow: auto;
}

.loading-box {
  height: 100%;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 10px;
  color: var(--el-text-color-secondary);
}

.settings-panel {
  padding: 2px 2px 6px;
}

.settings-label {
  font-size: 12px;
  color: var(--el-text-color-secondary);
  margin: 8px 0 6px;
}

.settings-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  font-size: 13px;
  margin: 10px 0 6px;
}

/* 连续色相渐变条：点选/拖动取色，圆点标记当前色相 */
.theme-bar {
  position: relative;
  height: 26px;
  border-radius: 8px;
  border: 1px solid var(--el-border-color);
  background: linear-gradient(
    to right,
    hsl(0, 16%, 56%), hsl(30, 16%, 56%), hsl(60, 16%, 56%), hsl(90, 16%, 56%),
    hsl(120, 16%, 56%), hsl(150, 16%, 56%), hsl(180, 16%, 56%), hsl(210, 16%, 56%),
    hsl(240, 16%, 56%), hsl(270, 16%, 56%), hsl(300, 16%, 56%), hsl(330, 16%, 56%),
    hsl(360, 16%, 56%)
  );
  cursor: pointer;
  touch-action: none;
}

.theme-bar-marker {
  position: absolute;
  top: 50%;
  transform: translate(-50%, -50%);
  width: 15px;
  height: 15px;
  border-radius: 50%;
  background: #fff;
  border: 2px solid rgba(0, 0, 0, 0.35);
  box-shadow: 0 1px 3px rgba(0, 0, 0, 0.3);
  pointer-events: none;
}

.theme-selected {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-top: 8px;
  font-size: 12px;
  color: var(--el-text-color-secondary);
}

.theme-chip {
  width: 16px;
  height: 16px;
  border-radius: 4px;
  border: 1px solid var(--el-border-color);
}

.pulsing {
  animation: pulse 1.2s ease-in-out infinite;
}

@keyframes pulse {
  0%,
  100% {
    opacity: 1;
  }
  50% {
    opacity: 0.5;
  }
}
</style>
