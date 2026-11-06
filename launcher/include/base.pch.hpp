#pragma once
#ifndef PRISM_PRECOMPILED_BASE_HEADERS_H
#define PRISM_PRECOMPILED_BASE_HEADERS_H

#ifdef Q_OS_WIN
#include <windows.h>

#include <consoleapi.h>
#include <dxgi1_6.h>
#include <fileapi.h>
#include <io.h>
#include <objbase.h>
#include <objidl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <versionhelpers.h>
#include <winnls.h>

#include <winrt/base.h>
#include <wrl/client.h>
#endif

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <utility>

#include <BuildConfig.h>
#include <FileSystem.h>
#include <Json.h>
#include <Version.h>

#endif  // PRISM_PRECOMPILED_BASE_HEADERS_H
