// SPDX-FileCopyrightText: 2026 Alberto Valls Ortiz <24375270+albvalort@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <array>

// Ids of the phases of a Minecraft launch, in order. Steps can be inserted before or after them with
// LaunchTask::insertStep. Phases made of several steps share the id. The ones marked optional are only present when
// their setting or situation applies.
namespace LaunchStepIds {

inline constexpr const char* Header = "header";
inline constexpr const char* CreateFolders = "create-folders";
inline constexpr const char* LookupServer = "lookup-server";       // optional
inline constexpr const char* PreLoadCommand = "pre-load-command";  // optional
inline constexpr const char* LoadMeta = "load-meta";
inline constexpr const char* Java = "java";
inline constexpr const char* PreLaunchCommand = "pre-launch-command";  // optional
inline constexpr const char* Update = "update";
inline constexpr const char* JarMods = "jar-mods";
inline constexpr const char* ScanMods = "scan-mods";
inline constexpr const char* CheckMemory = "check-memory";
inline constexpr const char* InstanceInfo = "instance-info";
inline constexpr const char* Natives = "natives";
inline constexpr const char* Assets = "assets";
inline constexpr const char* Launch = "launch";
inline constexpr const char* PostExitCommand = "post-exit-command";  // optional
inline constexpr const char* QuitAfterStop = "quit-after-stop";      // optional

inline constexpr std::array all = { Header,           CreateFolders, LookupServer, PreLoadCommand,  LoadMeta,     Java,
                                    PreLaunchCommand, Update,        JarMods,      ScanMods,        CheckMemory,  InstanceInfo,
                                    Natives,          Assets,        Launch,       PostExitCommand, QuitAfterStop };

}  // namespace LaunchStepIds
