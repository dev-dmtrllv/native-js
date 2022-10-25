#pragma once

#include "framework.hpp"

namespace NativeJS
{
	namespace Concepts
	{
		template <typename T>
		concept Numeric = (std::integral<T> || std::floating_point<T>)
			&& !std::same_as<T, bool>
			&& !std::same_as<T, char>
			&& !std::same_as<T, unsigned char>
			&& !std::same_as<T, char8_t>
			&& !std::same_as<T, char16_t>
			&& !std::same_as<T, char32_t>
			&& !std::same_as<T, wchar_t>
			&& !std::is_pointer_v<T>;

		template <typename BaseType, typename SubType>
		concept Extends = std::is_base_of<BaseType, SubType>::value;

		template<typename T>
		concept IsAtomic = sizeof(T) <= sizeof(size_t);
	}
}