import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

// https://vitejs.dev/config/
export default defineConfig({
  plugins: [react()],
  test: {
    coverage: {
      provider: 'v8',
      reporter: ['text', 'json-summary', 'html'],
      include: ['src/**/*.{js,jsx}'],
      // main.jsx apenas monta a aplicação no DOM; não há lógica a testar.
      exclude: ['src/**/*.test.{js,jsx}', 'src/main.jsx'],
    },
  },
})
