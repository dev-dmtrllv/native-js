#pragma once

#include "framework.hpp"

namespace NativeJS
{
	template<typename T>
	struct Lazy
	{
	private:
		static T* firstGet(Lazy<T>& self)
		{
			assert(!self.value_.has_value());
			self.getter_ = initializedGet;
			self.value_.emplace(self.initializer_());
			return self.value_.value();
		}

		static T* initializedGet(Lazy<T>& self)
		{
			assert(self.value_.has_value());
			return self.value_.value();
		}
	public:
		using Initializer = std::function<T()>;

		using Getter = T * (*)(Lazy<T>&);

		Lazy(Initializer initializer) : value_(), getter_(firstGet), initializer_(initializer) { }
		Lazy(const Lazy& other) : value_(other.value_), getter_(other.getter_), initializer_(other.initializer) { }
		Lazy(Lazy&& other) : value_(other.value_), getter_(other.getter_), initializer_(other.initializer) { }

		T* get()
		{
			return getter_(*this);
		}

	private:
		Getter getter_;
		std::optional<T> value_;
		Initializer initializer_;
	};
}