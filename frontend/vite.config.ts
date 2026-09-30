import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";

// The production bundle is served by the C++ host from the `materialmc://app/` scheme,
// so every asset URL must be relative to index.html.
export default defineConfig({
  base: "./",
  plugins: [react()],
  server: {
    host: "127.0.0.1",
    port: 5173,
    strictPort: true,
  },
  build: {
    outDir: "dist",
    emptyOutDir: true,
    target: "es2022",
    sourcemap: false,
    // WebKitGTK / WebView2 / WKWebView all support modern ES modules.
    modulePreload: { polyfill: false },
    chunkSizeWarningLimit: 800,
  },
});
