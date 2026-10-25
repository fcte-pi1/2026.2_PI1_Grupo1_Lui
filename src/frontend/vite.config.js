import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'
import { configDefaults } from 'vitest/config'

// https://vitejs.dev/config/
export default defineConfig({
  plugins: [react()],
  test: {
    // e2e/ tem os testes do Playwright, que rodam com npm run test:e2e.
    exclude: [...configDefaults.exclude, 'e2e/**'],
    coverage: {
      provider: 'v8',
      reporter: ['text', 'json-summary', 'html'],
      include: ['src/**/*.{js,jsx}'],
      // main.jsx apenas monta a aplicação no DOM; não há lógica a testar.
      exclude: ['src/**/*.test.{js,jsx}', 'src/main.jsx'],
    },
  },
})
