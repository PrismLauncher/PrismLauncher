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

#include "WindowsAppContainer.h"

#include <aclapi.h>
#include <securitybaseapi.h>
#include <shellapi.h>
#include <userenv.h>

#include "ExpectedHelpers.h"

namespace {
template <typename F>
void runLater(F f)
{
    QMetaObject::invokeMethod(qApp, [f] { f(); }, Qt::QueuedConnection);
}
}  // namespace

constexpr auto g_prismCapabilityName = "prismlauncher.grantWinstaWriteAttributesCapability_NydcoL6P93pwtIlf";

WindowsAppContainer::WindowsAppContainer(const PSID appContainerSid) : m_appContainerSid{ appContainerSid, &FreeSid }
{
    Q_ASSERT(IsValidSid(m_appContainerSid.get()));

    // Reference:
    // https://github.com/Moulberry/PandoraLauncher/blob/4a3e4cd0296fbdbcdef564abbc2890db1176c818/crates/command/src/windows/appcontainer.rs#L41

    addCapability("internetClientServer");
    addCapability("privateNetworkClientServer");
    addCapability(g_prismCapabilityName);
}

std::expected<void, std::error_code> WindowsAppContainer::prepareStartupInfo()
{
    // Reference: https://learn.microsoft.com/en-us/windows/win32/secauthz/implementing-an-appcontainer#launching-the-appcontainer-or-lpac

    LPPROC_THREAD_ATTRIBUTE_LIST attributeList = nullptr;
    constexpr DWORD attributesCount = 1;
    SIZE_T attributesSize = 0;

    const BOOL result = InitializeProcThreadAttributeList(nullptr, attributesCount, 0, &attributesSize);
    // https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-initializeprocthreadattributelist#remarks
    Q_ASSERT(result == FALSE);
    if (const auto error = GetLastError(); error != ERROR_INSUFFICIENT_BUFFER) {
        return UNEXPECTED_WIN32_ERROR(error);
    }

    attributeList = static_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(HeapAlloc(GetProcessHeap(), 0, attributesSize));
    if (!attributeList) {
        return UNEXPECTED_WIN32_ERROR(ERROR_NOT_ENOUGH_MEMORY);
    }
    runLater([attributeList] { DeleteProcThreadAttributeList(attributeList); });

    if (InitializeProcThreadAttributeList(attributeList, attributesCount, 0, &attributesSize) == FALSE) {
        return UNEXPECTED_WIN32_ERROR_LAST;
    }

    auto* capabilities = new SECURITY_CAPABILITIES{};
    runLater([capabilities] {
        delete capabilities;
    });
    capabilities->AppContainerSid = m_appContainerSid.get();

    TRY(buildCapabilities(&capabilities->Capabilities, &capabilities->CapabilityCount));

    if (UpdateProcThreadAttribute(attributeList, 0, PROC_THREAD_ATTRIBUTE_SECURITY_CAPABILITIES, capabilities,
                                  sizeof(SECURITY_CAPABILITIES), nullptr, nullptr) == FALSE) {
        return UNEXPECTED_WIN32_ERROR_LAST;
    }

    m_startupInfo = std::make_unique<STARTUPINFOEXW>();
    m_startupInfo->lpAttributeList = attributeList;

    return {};
}

std::expected<std::unique_ptr<WindowsAppContainer>, HRESULT> WindowsAppContainer::create()
{
    // Reference: https://learn.microsoft.com/en-us/windows/win32/secauthz/implementing-an-appcontainer#creating-the-profile

    const std::wstring containerName = appContainerId();
    const std::wstring displayName = BuildConfig.LAUNCHER_DISPLAYNAME.toStdWString();
    const std::wstring description = QObject::tr("Minecraft (Sandbox)").toStdWString();

    PSID appContainerSid{};  // MEMORY: Ownership is passed to created WindowsAppContainer
    HRESULT hr = CreateAppContainerProfile(containerName.c_str(), displayName.c_str(), description.c_str(), nullptr, 0, &appContainerSid);

    if (FAILED(hr)) {
        if (hr != HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS)) {
            return std::unexpected(hr);
        }
        hr = DeriveAppContainerSidFromAppContainerName(containerName.c_str(), &appContainerSid);
        if (FAILED(hr)) {
            return std::unexpected(hr);
        }
    }

    return std::unique_ptr<WindowsAppContainer>(new WindowsAppContainer(appContainerSid));
}

std::expected<void, HRESULT> WindowsAppContainer::deleteProfile()
{
    const auto name = appContainerId();
    if (const HRESULT hr = DeleteAppContainerProfile(name.c_str()); hr != ERROR_SUCCESS) {
        return std::unexpected(hr);
    }
    return {};
}

void WindowsAppContainer::attach(QProcess* process)
{
    process->setCreateProcessArgumentsModifier([this](QProcess::CreateProcessArguments* args) {
        Q_ASSERT(m_startupInfo);
        m_startupInfo->StartupInfo = *args->startupInfo;
        m_startupInfo->StartupInfo.cb = sizeof(STARTUPINFOEXW);

        args->startupInfo = &m_startupInfo->StartupInfo;
        args->flags |= EXTENDED_STARTUPINFO_PRESENT;

        runLater([this] {
            m_startupInfo.reset();
        });
    });
}

std::expected<void, std::error_code> WindowsAppContainer::grantFSAccess(const QString& path, const AccessMode mode)
{
    DWORD permissions{};
    QString modeString;
    switch (mode) {
        case AccessMode::Traverse:
            permissions = FILE_LIST_DIRECTORY | SYNCHRONIZE;
            modeString = "traverse";
            break;
        case AccessMode::Read:
            permissions = FILE_GENERIC_READ | FILE_GENERIC_EXECUTE;
            modeString = "read";
            break;
        case AccessMode::ReadWrite:
            permissions = FILE_ALL_ACCESS;
            modeString = "read-write";
            break;
    }

    const DWORD inheritance = mode != AccessMode::Traverse ? SUB_CONTAINERS_AND_OBJECTS_INHERIT : NO_INHERITANCE;
    if (const auto result = addGrantToFileAcl(path, permissions, inheritance); !result) {
        if (result.error().value() != ERROR_ACCESS_DENIED) {
            return std::unexpected{ result.error() };
        }

        qWarning() << "Access denied when granting container" << modeString << "access to" << path;
        if (mode != AccessMode::Read) {
            return std::unexpected{ result.error() };
        }
        // This may be alright. The file may already have permissions set for ALL APPLICATION PACKAGES, so we shouldn't fail right away.
    } else {
        qInfo() << "Granted container" << modeString << "access to" << path;
    }

    if (mode == AccessMode::Traverse) {
        return {};
    }

    return grantParentsTraverseAccess(path);
}

std::expected<void, std::error_code> WindowsAppContainer::finalizeSetup(QProcess* process)
{
    Q_ASSERT_X(!m_allowedTraversePaths.isEmpty(), "WindowsAppContainer::finalizeSetup", "No filesystem access has been granted. Call grantFSAccess");

    const auto containerFolder = QString("%1/Packages/%2/AC").arg(qEnvironmentVariable("LOCALAPPDATA"), appContainerId());
    TRY(grantParentsTraverseAccess(containerFolder));
    TRY(grantWindowStationWritePermissions());
    TRY(elevateToRetryGrants());
    TRY(prepareStartupInfo());

    attach(process);

    return {};
}

void WindowsAppContainer::addCapability(const QString& capability)
{
    m_requestedCapabilities << capability;
}

std::wstring WindowsAppContainer::appContainerId()
{
    return BuildConfig.LAUNCHER_APPID.toStdWString();
}

std::expected<LocalPtr<std::remove_pointer_t<PSID>>, std::error_code> WindowsAppContainer::getCapabilitySid(const QString& name)
{
    // Reference: https://learn.microsoft.com/en-us/windows/win32/secauthz/implementing-an-appcontainer#constructing-the-capabilities

    const std::wstring wName = name.toStdWString();

    LocalPtr<PSID> groupSids{};
    DWORD groupSidCount{};
    LocalPtr<PSID> capabilitySids{};
    DWORD capabilitySidCount{};

    if (DeriveCapabilitySidsFromName(wName.c_str(), std::out_ptr(groupSids), &groupSidCount, std::out_ptr(capabilitySids),
                                     &capabilitySidCount) == FALSE) {
        return UNEXPECTED_WIN32_ERROR_LAST;
    }

    // NOLINTBEGIN(*-pro-bounds-pointer-arithmetic)
    LocalFree(groupSids.get()[0]);
    const PSID capability = capabilitySids.get()[0];
    // NOLINTEND(*-pro-bounds-pointer-arithmetic)

    return LocalPtr<std::remove_pointer_t<PSID>>(capability);
}

std::expected<bool, std::error_code> WindowsAppContainer::aclsEqual(PACL acl1, PACL acl2)
{
    // Reference:
    // https://github.com/Moulberry/PandoraLauncher/blob/4a3e4cd0296fbdbcdef564abbc2890db1176c818/crates/command/src/windows/appcontainer.rs#L445
    if (acl1->AceCount != acl2->AceCount) {
        return false;
    }

    for (WORD i = 0; i < acl1->AceCount; ++i) {
        void* ace1{};
        void* ace2{};
        if (GetAce(acl1, i, &ace1) == FALSE || GetAce(acl2, i, &ace2) == FALSE) {
            return UNEXPECTED_WIN32_ERROR_LAST;
        }

        const auto* header1 = static_cast<PACE_HEADER>(ace1);
        const auto* header2 = static_cast<PACE_HEADER>(ace2);
        if (header1->AceSize != header2->AceSize || header1->AceFlags != header2->AceFlags || header1->AceType != header2->AceType) {
            return false;
        }

        const std::span ace1Data{ static_cast<std::byte*>(ace1), header1->AceSize };
        const std::span ace2Data{ static_cast<std::byte*>(ace2), header2->AceSize };
        if (!std::ranges::equal(ace1Data, ace2Data)) {
            return false;
        }
    }

    return true;
}

std::expected<void, std::error_code> WindowsAppContainer::buildCapabilities(PSID_AND_ATTRIBUTES* capabilities, DWORD* capabilityCount) const
{
    Q_ASSERT_X(!m_requestedCapabilities.isEmpty(), "WindowsAppContainer::buildCapabilities", "No capabilities have been requested");

    // Reference: https://learn.microsoft.com/en-us/windows/win32/secauthz/implementing-an-appcontainer#constructing-the-capabilities

    *capabilities = nullptr;
    *capabilityCount = 0;

    std::vector<PSID> capabilitySids{};
    capabilitySids.reserve(m_requestedCapabilities.size());
    auto freeSids = [&capabilitySids] {
        for (auto* sid : capabilitySids) {
            LocalFree(sid);
        }
    };

    for (const auto& capability : m_requestedCapabilities) {
        auto sidResult = getCapabilitySid(capability);
        if (!sidResult) {
            freeSids();
            return std::unexpected{ sidResult.error() };
        }
        auto* sid = sidResult.value().release(); // scary!

        capabilitySids.emplace_back(sid);
    }

    auto* localCapabilities =
        static_cast<PSID_AND_ATTRIBUTES>(HeapAlloc(GetProcessHeap(), 0, capabilitySids.size() * sizeof(SID_AND_ATTRIBUTES)));
    if (!localCapabilities) {
        freeSids();
        return UNEXPECTED_WIN32_ERROR(ERROR_NOT_ENOUGH_MEMORY);
    }

    for (size_t i = 0; i < capabilitySids.size(); ++i) {
        // NOLINTBEGIN(*-pro-bounds-pointer-arithmetic)
        localCapabilities[i].Sid = capabilitySids.at(i);
        localCapabilities[i].Attributes = SE_GROUP_ENABLED;
        // NOLINTEND(*-pro-bounds-pointer-arithmetic)
    }

    *capabilities = localCapabilities;
    *capabilityCount = static_cast<DWORD>(capabilitySids.size());
    return {};
}

std::expected<void, std::error_code> WindowsAppContainer::grantParentsTraverseAccess(const QString& path)
{
    const QFileInfo fileInfo(path);
    QDir parent = fileInfo.dir();  // Note: always returns the *parent* dir, even when the object itself is a dir

    do {  // NOLINT(*-avoid-do-while)
        if (const QString parentPath = parent.path();
            !m_allowedTraversePaths.contains(parentPath) && !m_deniedTraversePaths.contains(parentPath)) {
            if (const auto result = grantFSAccess(parentPath, AccessMode::Traverse); !result) {
                if (result.error().value() == ERROR_ACCESS_DENIED) {
                    m_deniedTraversePaths << parentPath;
                    continue;
                }

                return std::unexpected{ result.error() };
            }

            m_allowedTraversePaths << parentPath;
        }
    } while (parent.cdUp());

    return {};
}

std::expected<void, std::error_code> WindowsAppContainer::addGrantToFileAcl(const QString& path,
                                                                            const DWORD permissions,
                                                                            const DWORD inheritance) const
{
    const std::wstring wPath = path.toStdWString();

    PACL existingDacl = nullptr;  // MEMORY: This is part of the security descriptor and should *not* be freed separately
    LocalPtr<SECURITY_DESCRIPTOR> unusedSd{};
    if (const DWORD error = GetNamedSecurityInfoW(wPath.c_str(), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr, &existingDacl,
                                                  nullptr, std::out_ptr(unusedSd));
        error != ERROR_SUCCESS) {
        return UNEXPECTED_WIN32_ERROR(error);
    }

    EXPLICIT_ACCESSW ea{};
    ea.grfAccessMode = GRANT_ACCESS;
    ea.grfAccessPermissions = permissions;
    ea.grfInheritance = inheritance;
    ea.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    ea.Trustee.pMultipleTrustee = nullptr;
    ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea.Trustee.TrusteeType = TRUSTEE_IS_GROUP;
    ea.Trustee.ptstrName = static_cast<LPWSTR>(m_appContainerSid.get());

    LocalPtr<ACL> newDacl{};
    if (const DWORD error = SetEntriesInAclW(1, &ea, existingDacl, std::out_ptr(newDacl)); error != ERROR_SUCCESS) {
        return UNEXPECTED_WIN32_ERROR(error);
    }

    // https://serverfault.com/a/965346
    if (inheritance == NO_INHERITANCE) {
        // Only compare ACLs if inheritance is disabled.
        // Sometimes children don't properly inherit the ACLs, so we need to be sure to apply them even if the parent is correct
        const auto equalResult = aclsEqual(existingDacl, newDacl.get());
        if (!equalResult.has_value()) {
            qWarning() << "Could not compare ACLs:" << equalResult.error().message();
        }
        if (equalResult.has_value() && equalResult.value()) {
            return {};
        }

        const LocalPtr<SECURITY_DESCRIPTOR> sd{ static_cast<SECURITY_DESCRIPTOR*>(LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH)) };

        if (InitializeSecurityDescriptor(sd.get(), SECURITY_DESCRIPTOR_REVISION) == FALSE) {
            return UNEXPECTED_WIN32_ERROR_LAST;
        }

        if (SetSecurityDescriptorDacl(sd.get(), TRUE, newDacl.get(), FALSE) == FALSE) {
            return UNEXPECTED_WIN32_ERROR_LAST;
        }

        if (SetFileSecurityW(wPath.c_str(), DACL_SECURITY_INFORMATION, sd.get()) == FALSE) {
            return UNEXPECTED_WIN32_ERROR_LAST;
        }

        return {};
    }

    if (const DWORD error = SetNamedSecurityInfoW(const_cast<LPWSTR>(wPath.c_str()), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
                                                  nullptr,  // NOLINT(*-pro-type-const-cast)
                                                  nullptr, newDacl.get(), nullptr);
        error != ERROR_SUCCESS) {
        return UNEXPECTED_WIN32_ERROR(error);
    }

    return {};
}

std::expected<void, std::error_code> WindowsAppContainer::elevateToRetryGrants()
{
    if (m_deniedTraversePaths.isEmpty()) {
        return {};
    }

    QStringList args;
    for (const QString& path : m_deniedTraversePaths) {
        args << QString("--internal-denied-traverse-path=\"%1\"").arg(path);
    }

    const std::wstring currentExe = QApplication::applicationFilePath().toStdWString();
    const std::wstring wArgs = args.join(' ').toStdWString();

    SHELLEXECUTEINFOW info{};
    info.cbSize = sizeof(info);
    info.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC;
    info.lpVerb = L"runas";
    info.lpFile = currentExe.c_str();
    info.lpParameters = wArgs.c_str();
    info.nShow = SW_HIDE;

    if (ShellExecuteExW(&info) == FALSE) {
        return UNEXPECTED_WIN32_ERROR_LAST;
    }

    if (info.hProcess != nullptr) {
        WaitForSingleObject(info.hProcess, INFINITE);
        CloseHandle(info.hProcess);
    }

    m_allowedTraversePaths << m_deniedTraversePaths;
    m_deniedTraversePaths.clear();

    return {};
}

std::expected<void, std::error_code> WindowsAppContainer::grantWindowStationWritePermissions()
{
    // Allows use of ClipCursor / SetCursorPos / etc.
    // Reference:
    // https://github.com/Moulberry/PandoraLauncher/blob/4a3e4cd0296fbdbcdef564abbc2890db1176c818/crates/command/src/windows/appcontainer.rs#L249

    auto prismCapabilitySidResult = getCapabilitySid(g_prismCapabilityName);
    if (!prismCapabilitySidResult) {
        return std::unexpected { prismCapabilitySidResult.error() };
    }
    const auto prismCapabilitySid = std::move(prismCapabilitySidResult.value());
    Q_ASSERT(IsValidSid(prismCapabilitySid.get()));

    auto* winsta = OpenWindowStationW(L"winsta0", FALSE, READ_CONTROL | WRITE_DAC);
    if (!winsta) {
        return UNEXPECTED_WIN32_ERROR_LAST;
    }
    runLater([winsta] { CloseWindowStation(winsta); });

    // TODO: a bit of code duplication from addGrantToFileAcl
    PACL existingDacl = nullptr;  // MEMORY: This is part of the security descriptor and should *not* be freed separately
    LocalPtr<SECURITY_DESCRIPTOR> unusedSd{};
    if (const DWORD error = GetSecurityInfo(winsta, SE_WINDOW_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr, &existingDacl, nullptr,
                                            std::out_ptr(unusedSd));
        error != ERROR_SUCCESS) {
        return UNEXPECTED_WIN32_ERROR(error);
    }

    EXPLICIT_ACCESSW ea{};
    ea.grfAccessMode = GRANT_ACCESS;
    ea.grfAccessPermissions = WINSTA_WRITEATTRIBUTES;
    ea.grfInheritance = NO_INHERITANCE;
    ea.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    ea.Trustee.pMultipleTrustee = nullptr;
    ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea.Trustee.TrusteeType = TRUSTEE_IS_GROUP;
    ea.Trustee.ptstrName = static_cast<LPWSTR>(prismCapabilitySid.get());

    LocalPtr<ACL> newDacl{};
    if (const DWORD error = SetEntriesInAclW(1, &ea, existingDacl, std::out_ptr(newDacl)); error != ERROR_SUCCESS) {
        return UNEXPECTED_WIN32_ERROR(error);
    }

    if (const DWORD error = SetSecurityInfo(winsta, SE_WINDOW_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr, newDacl.get(), nullptr);
        error != ERROR_SUCCESS) {
        return UNEXPECTED_WIN32_ERROR_LAST;
    }

    return {};
}
