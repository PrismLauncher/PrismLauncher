// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Octol1ttle <l1ttleofficial@outlook.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#ifdef Q_OS_WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include <expected>

template <typename T>
struct LocalFreeDeleter {
    void operator()(T* ptr) { LocalFree(ptr); }  // NOLINT(*-multi-level-implicit-pointer-conversion)
};

template <typename T>
using LocalPtr = std::unique_ptr<T, LocalFreeDeleter<T>>;

class WindowsAppContainer final {
   public:
    enum class AccessMode : std::uint8_t {
        Traverse,
        Read,
        ReadWrite,
    };

    static std::expected<std::unique_ptr<WindowsAppContainer>, HRESULT> create();
    static std::expected<void, HRESULT> deleteProfile();

    ~WindowsAppContainer() = default;

    std::expected<void, std::error_code> grantFSAccess(const QString& path, AccessMode mode);
    std::expected<void, std::error_code> finalizeSetup(QProcess* process);
    void addCapability(const QString& capability);

   private:
    static std::wstring appContainerId();
    static std::expected<LocalPtr<std::remove_pointer_t<PSID>>, std::error_code> getCapabilitySid(const QString& name);
    static std::expected<bool, std::error_code> aclsEqual(PACL acl1, PACL acl2);
    static std::expected<void, std::error_code> grantWindowStationWritePermissions();

    explicit WindowsAppContainer(PSID appContainerSid);

    std::expected<void, std::error_code> addGrantToFileAcl(const QString& path, DWORD permissions, DWORD inheritance) const;
    std::expected<void, std::error_code> grantParentsTraverseAccess(const QString& path);
    std::expected<void, std::error_code> elevateToRetryGrants();

    std::expected<void, std::error_code> buildCapabilities(PSID_AND_ATTRIBUTES* capabilities, DWORD* capabilityCount) const;
    std::expected<void, std::error_code> prepareStartupInfo();

    void attach(QProcess* process);

   private:
    std::unique_ptr<std::remove_pointer_t<PSID>, decltype(&FreeSid)> m_appContainerSid;
    QStringList m_requestedCapabilities;
    QStringList m_allowedTraversePaths;
    QStringList m_deniedTraversePaths;
    std::unique_ptr<STARTUPINFOEXW> m_startupInfo;
};