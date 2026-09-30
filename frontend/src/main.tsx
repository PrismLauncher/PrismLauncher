import { StrictMode } from "react";
import { createRoot } from "react-dom/client";
import { App } from "./App";
import { checkContract, materialmc } from "./api/client";
import "./styles/global.css";

createRoot(document.getElementById("root") as HTMLElement).render(
  <StrictMode>
    <App />
  </StrictMode>,
);

if (import.meta.env.DEV && materialmc.connected) {
  void checkContract();
}
