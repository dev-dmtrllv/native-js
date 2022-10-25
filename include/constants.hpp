#pragma once

#include "framework.hpp"

namespace NativeJS
{
	constexpr static size_t MAX_QUEUE_SIZE = 1024;

#ifdef _WINDOWS
	constexpr static size_t ASYNC_UI_WORK = WM_USER + 1;
	constexpr static size_t BLOCKING_UI_WORK = WM_USER + 2;
	constexpr static size_t UI_EVENT_RESULT = WM_USER + 3;
#endif
}