#pragma once

#include "framework.hpp"

namespace NativeJS
{
	template<typename T>
	class StackAllocator
	{
		using Ptr = T*;

	public:
		StackAllocator(uint32_t capacity) :
			capacity_(capacity),
			sp_(capacity),
			buffer_(static_cast<T*>(malloc(capacity * sizeof(T)))),
			freeStack_(new Ptr[capacity])
		{
			for (uint32_t i = 0; i < capacity_; i++)
				freeStack_[i] = &buffer_[i];
		}

		StackAllocator(const StackAllocator& other) = delete;
		StackAllocator(StackAllocator&& other) = delete;

		~StackAllocator()
		{
			for (uint32_t i = sp_; i < capacity_; i++)
			{
				T* ref = &buffer_[i];
				free(ref);
			}
			::free(buffer_);
			delete[] freeStack_;
		};

		template<class... Args>
			requires std::constructible_from<T, Args...>
		T* alloc(Args&&... args)
		{
			assert(sp_ > 0);
			return std::construct_at(freeStack_[--sp_], std::forward<Args>(args)...);
		}

		T* alloc(const T& item)
		{
			assert(sp_ > 0);
			return new (freeStack_[--sp_]) T(item);
		}

		T* alloc(T&& item)
		{
			assert(sp_ > 0);
			return new (freeStack_[--sp_]) T(item);
		}

		void free(T*& ptr)
		{
			assert(ptr >= buffer_ && ptr <= (buffer_ + (sizeof(T) * (capacity_ - 1))));
			assert(sp_ <= capacity_);
			ptr->~T();
			freeStack_[sp_++] = ptr;
			ptr = nullptr;
		}

		uint32_t size() const { return capacity_ - sp_; }
		uint32_t capacity() const { return capacity_; }

	private:
		T* buffer_;
		Ptr* freeStack_;
		uint32_t sp_;
		const uint32_t capacity_;
	};
}