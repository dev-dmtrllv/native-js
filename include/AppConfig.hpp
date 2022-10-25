#pragma once

#include "framework.hpp"

namespace NativeJS
{
	namespace JS
	{
		class BaseEnv;
	}

	struct AppConfig
	{
		struct Entry {
			std::string file;
			std::string exportName = "default";
		};

		std::string name;
		std::string type;
		Entry entry;
		std::vector<std::string> resolve;
		
		AppConfig() {};

		void load(const JS::BaseEnv& env, v8::Local<v8::Object> obj);

		private:
			bool isLoaded_;
	};
}