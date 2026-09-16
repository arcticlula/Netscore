import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'

// https://vite.dev/config/
// Deployed to GitHub Pages at https://arcticlula.github.io/Netscore/
export default defineConfig({
  base: '/Netscore/',
  plugins: [vue()],
})
