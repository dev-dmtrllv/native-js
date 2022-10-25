#pragma once

#include "js/JSClass.hpp"

namespace NativeJS
{
	namespace JS
	{
		class Env;

		class Timeout : public ObjectWrapper
		{
		public:
			static const char* RESOLVER_KEY;

			Timeout(const Env& env, std::chrono::milliseconds resolveTime, const bool loop);
			Timeout(const Timeout&) = delete;
			Timeout(Timeout&&) = delete;
			virtual ~Timeout();

			virtual void initializeProps();
			bool shouldResolve(std::chrono::milliseconds time);
			void setIndex(size_t index);
			
			JS_METHOD_DECL(resolve);
		
			inline size_t index() const { return index_; }

			inline bool loop() const { return loop_; }

		private:
			size_t index_;
			const bool loop_;
			std::chrono::milliseconds resolveTime_;
		};

		class TimeoutClass : public Class
		{
			JS_CLASS_BODY(TimeoutClass);

		private:
			JS_CLASS_METHOD(ctor);
			JS_CLASS_METHOD(reset);
			JS_CLASS_METHOD(cancel);
		};
	}
}