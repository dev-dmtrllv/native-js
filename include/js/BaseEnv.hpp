#pragma once

#include "framework.hpp"

namespace NativeJS
{
	class App;
	class Worker;

	namespace JS
	{
		class App;
		class BaseEnv
		{
		public:
			BaseEnv(NativeJS::App& app);
			BaseEnv(const BaseEnv&) = delete;
			BaseEnv(BaseEnv&&) = delete;
			~BaseEnv();

			inline NativeJS::App& app() const { return app_; }
			inline v8::Isolate* isolate() const { return isolate_; }
			inline v8::Local<v8::Context> context() const { return context_.Get(isolate_); }

			void throwException(const char* error) const;

		private:
			NativeJS::App& app_;
			v8::Isolate::CreateParams createParams_;
			v8::Isolate* isolate_;
			v8::Eternal<v8::Context> context_;
		};
	}
}