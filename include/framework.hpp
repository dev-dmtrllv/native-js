#pragma once

// ----------------  WINDOWS  ---------------- //
#ifdef _WINDOWS

#define NOMINMAX

#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <shellapi.h>
#include <dwmapi.h>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "dbghelp.lib")
#pragma comment(lib, "shlwapi.lib")

#endif
// ----------------  WINDOWS  ---------------- //



// -----------  STANDARD INCLUDES  ----------- //
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <string>
#include <vector>
#include <errno.h>
#include <thread>
#include <cassert>
#include <concepts>
#include <fstream>
#include <streambuf>
#include <filesystem>
#include <optional>
#include <queue>
#include <set>
#include <limits>
#include <algorithm>
#include <stack>
#include <queue>
#include <semaphore>
// -----------  STANDARD INCLUDES  ----------- //




// -----------------  V8  ----------------- //
#include <v8.h>
#include <libplatform/libplatform.h>

#pragma comment(lib, "v8_monolith.lib")
#pragma comment(lib, "v8_libbase.lib")
#pragma comment(lib, "v8_libplatform.lib")
#pragma comment(lib, "v8_snapshot.lib")
// -----------------  V8  ----------------- //



// --------------  VULKAN  ---------------- //
#define VK_USE_PLATFORM_WIN32_KHR 1

#include "vulkan/vulkan.hpp"

#pragma comment(lib, "vulkan-1.lib")
// --------------  VULKAN  ---------------- //



// ----------  Typedefs & Macros ---------- //
using Path = std::filesystem::path;

#undef ERROR // name colission
// ----------  Typedefs & Macros ---------- //