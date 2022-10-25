#pragma once

#include "framework.hpp"

namespace NativeJS
{
	namespace Utils
	{
		const std::wstring toWString(const char*);

		void formatPath(std::string& path);
		void formatPath(char* path);

		size_t getCurrentThreadID();

		template<typename T>
		constexpr T clamp(T val, T min, T max)
		{
			return val < min ? min : val > max ? max : val;
		}

		template <typename ... Args>
		constexpr bool isVoidFunction(void(Args ...)) { return true; }

		template <typename R, typename ... Args>
		constexpr bool isVoidFunction(R(Args ...)) { return false; }

		template <typename T, typename R, typename ... Args>
		constexpr bool returnsType(R(Args ...)) { return std::is_same_v<T, R>(); }

		template<typename T>
		T now()
		{
			return std::chrono::duration_cast<T>(std::chrono::system_clock::now().time_since_epoch());
		}
	}
}