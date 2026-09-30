import { createHashRouter, Navigate, RouterProvider } from "react-router-dom";
import { InstanceActionsProvider } from "./components/InstanceActions";
import { Layout } from "./components/Layout";
import { ToastProvider } from "./components/Toasts";
import { ConsoleRequestListener } from "./pages/ConsolePage";
import { HomePage } from "./pages/HomePage";
import { InstancesPage } from "./pages/InstancesPage";
import { CreateInstancePage } from "./pages/CreateInstancePage";
import { InstanceDetailsPage } from "./pages/InstanceDetailsPage";
import { ModsPage } from "./pages/ModsPage";
import { WorldsPage } from "./pages/WorldsPage";
import { DownloadsPage } from "./pages/DownloadsPage";
import { AccountsPage } from "./pages/AccountsPage";
import { ConsolePage } from "./pages/ConsolePage";
import { SettingsPage } from "./pages/SettingsPage";
import { AboutPage } from "./pages/AboutPage";

// Hash routing: the production bundle is loaded from a custom scheme, where
// history-API paths would not map to files.
const router = createHashRouter([
  {
    element: (
      <InstanceActionsProvider>
        <ConsoleRequestListener />
        <Layout />
      </InstanceActionsProvider>
    ),
    children: [
      { index: true, element: <HomePage /> },
      { path: "instances", element: <InstancesPage /> },
      { path: "instances/new", element: <CreateInstancePage /> },
      { path: "instances/:id", element: <InstanceDetailsPage /> },
      { path: "instances/:id/:tab", element: <InstanceDetailsPage /> },
      { path: "mods", element: <ModsPage /> },
      { path: "worlds", element: <WorldsPage /> },
      { path: "downloads", element: <DownloadsPage /> },
      { path: "accounts", element: <AccountsPage /> },
      { path: "console", element: <ConsolePage /> },
      { path: "console/:id", element: <ConsolePage /> },
      { path: "settings", element: <SettingsPage /> },
      { path: "about", element: <AboutPage /> },
      { path: "*", element: <Navigate to="/" replace /> },
    ],
  },
]);

export function App() {
  return (
    <ToastProvider>
      <RouterProvider router={router} />
    </ToastProvider>
  );
}
