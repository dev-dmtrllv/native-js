#pragma once

#include "framework.hpp"

namespace NativeJS
{
	using Hash = uint64_t;

	struct Hasher
	{
	private:
		constexpr static Hash hashString(const char* str)
		{
			uint64_t hash = 5381;
			int c = 0;

			while ((c = *str++))
				hash = ((hash << 5) + hash) * 33 + c;

			return hash;
		}

	public:
		template<typename T>
		constexpr static Hash hash() noexcept { return hashString(typeid(T).name()); }
		constexpr static Hash hash(const char* str) noexcept { return hashString(str); }
		constexpr static Hash hash(std::string&& str) noexcept { return hashString(str.c_str()); }
		constexpr static Hash hash(const std::string& str) noexcept { return hashString(str.c_str()); }
		constexpr static bool check(Hash& hashStr, const char* str) noexcept { return hash(str) == hashStr; }
	};
}