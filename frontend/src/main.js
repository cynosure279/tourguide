import { createApp } from 'vue'
import ElementPlus from 'element-plus'
import 'element-plus/dist/index.css'
import 'element-plus/theme-chalk/dark/css-vars.css'
import * as Icons from '@element-plus/icons-vue'
import App from './App.vue'
import router from './router'
import { initTheme } from './store/theme'
import { initI18n } from './i18n'
import './style.css'

initTheme()
initI18n()

const app = createApp(App)
app.use(ElementPlus)
app.use(router)
for (const [name, comp] of Object.entries(Icons)) {
  app.component(name, comp)
}
app.mount('#app')
