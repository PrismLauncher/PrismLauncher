/**
 * Global launcher settings exposed to the web UI.
 *
 * The backend keeps an explicit allow-list (`SettingsApi.cpp`). Keys marked
 * "sensitive" there (anything that ends up on a command line: Java path, JVM
 * arguments, wrapper/pre-launch/post-exit commands) are only written after the
 * user confirms the change in a native dialog; a rejected confirmation makes
 * `settings.set` fail with `PERMISSION_DENIED`.
 */
export interface LauncherSettings {
  // Folders
  InstanceDir: string;
  DownloadsDir: string;
  // Java
  JavaPath: string;
  JvmArgs: string;
  MinMemAlloc: number;
  MaxMemAlloc: number;
  PermGen: number;
  AutomaticJavaSwitch: boolean;
  AutomaticJavaDownload: boolean;
  IgnoreJavaCompatibility: boolean;
  // Game window
  LaunchMaximized: boolean;
  MinecraftWinWidth: number;
  MinecraftWinHeight: number;
  // Console
  ShowConsole: boolean;
  AutoCloseConsole: boolean;
  ShowConsoleOnError: boolean;
  ConsoleMaxLines: number;
  // Behaviour
  CloseAfterLaunch: boolean;
  QuitAfterGameStop: boolean;
  ShowGameTime: boolean;
  RecordGameTime: boolean;
  // Custom commands (sensitive)
  PreLaunchCommand: string;
  WrapperCommand: string;
  PostExitCommand: string;
  // Network
  NumberOfConcurrentDownloads: number;
  NumberOfConcurrentTasks: number;
  RequestTimeout: number;
}

export type LauncherSettingKey = keyof LauncherSettings;

export interface SettingsChangedEvent {
  keys: LauncherSettingKey[];
}
