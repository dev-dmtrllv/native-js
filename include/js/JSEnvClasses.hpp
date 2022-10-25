#pragma once

#include "js/JSWindow.hpp"
#include "js/JSApp.hpp"
#include "js/JSEvent.hpp"
#include "js/JSWorker.hpp"
#include "js/Timeout.hpp"

namespace NativeJS
{
	namespace JS
	{
		class Env;

		struct EnvClasses
		{
			WindowClass windowClass;
			AppClass appClass;
			EventClass eventClass;
			WorkerClass workerClass;
			TimeoutClass timeoutClass;

			EnvClasses(const Env& env);

			void initialize();

			inline bool isInitialized() const { return isInitialized_; };

		private:
			bool isInitialized_;
		};
	}
}