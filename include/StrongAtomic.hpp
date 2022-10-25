#pragma once

#include "framework.hpp"
#include "concepts.hpp"

namespace NativeJS
{
	template<typename T>
		requires Concepts::IsAtomic<T>
	struct StrongAtomic
	{
		StrongAtomic() : value_() { }
		StrongAtomic(const T& val) : value_(val) { }
		StrongAtomic(T&& val) : value_(val) { }

		StrongAtomic(const StrongAtomic& other) : value_(other.get()) { }
		StrongAtomic(StrongAtomic&& other) : value_(other.get()) { }

		T get() const { return value_.load(std::memory_order::acquire); }

		void set(const T& val) { return value_.store(val, std::memory_order::release); }
		void set(T&& val) { return value_.store(val, std::memory_order::release); }

		std::atomic<T>& operator*() { return value_; }

	private:
		std::atomic<T> value_;
	};
};