import { createRouter, createWebHistory } from 'vue-router'

const routes = [
  { path: '/', name: 'home', component: () => import('../views/HomeView.vue'), meta: { title: 'nav.home' } },
  { path: '/manage', name: 'manage', component: () => import('../views/AttractionsView.vue'), meta: { title: 'nav.manage' } },
  { path: '/matrix', name: 'matrix', component: () => import('../views/MatrixView.vue'), meta: { title: 'nav.matrix' } },
  { path: '/tour', name: 'tour', component: () => import('../views/TourView.vue'), meta: { title: 'nav.tour' } },
  { path: '/cycle', name: 'cycle', component: () => import('../views/CycleView.vue'), meta: { title: 'nav.cycle' } },
  { path: '/path', name: 'path', component: () => import('../views/PathView.vue'), meta: { title: 'nav.path' } },
  { path: '/mst', name: 'mst', component: () => import('../views/MstView.vue'), meta: { title: 'nav.mst' } },
]

export default createRouter({
  history: createWebHistory(),
  routes,
})
