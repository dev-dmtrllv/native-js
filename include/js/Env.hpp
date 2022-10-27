#pragma once

#include "framework.hpp"
#include "js/JSApp.hpp"
#include "Event.hpp"
#include "js/JSEnvClasses.hpp"
#include "js/BaseEnv.hpp"
#include "js/Timeout.hpp"
#include "PersistentList.hpp"

namespace NativeJS
{
	class App;
	class Worker;

	namespace JS
	{
		class App;
		class Env : public BaseEnv
		{
		private:
			static void defaultAsyncResolver(const NativeJS::WorkEvent& e);

		public:
			struct Scope
			{
				Scope(const BaseEnv& env);
				~Scope();

			private:
				v8::Isolate::Scope isolateScope_;
				v8::HandleScope handleScope_;
				v8::Context::Scope contextScope_;
			};

			static inline const Env& fromIsolate(v8::Isolate* isolate) { return *static_cast<Env*>(isolate->GetData(0)); }
			static inline const Env& fromContext(v8::Local<v8::Context> ctx) { return fromIsolate(ctx->GetIsolate()); }
			static inline const Env& fromArgs(const v8::FunctionCallbackInfo<v8::Value>& args) { return fromIsolate(args.GetIsolate()); }

			static v8::MaybeLocal<v8::Module> importModule(v8::Local<v8::Context> context, v8::Local<v8::String> specifier, v8::Local<v8::FixedArray> import_assertions, v8::Local<v8::Module> referrer);

			Env(NativeJS::App& app, NativeJS::Worker* worker, const std::filesystem::path& entry, NativeJS::Worker* parentWorker = nullptr);
			Env(NativeJS::App& app, NativeJS::Worker* worker, std::filesystem::path&& entry, NativeJS::Worker* parentWorker = nullptr);
			Env(const Env&) = delete;
			Env(Env&&) = delete;
			~Env();

			inline v8::Local<v8::External> externalRef() const { return externalRef_.Get(isolate()); }
			inline NativeJS::Worker& worker() const { return *worker_; }
			inline v8::Local<v8::Symbol> internalSymbol() const { return internalSymbol_.Get(isolate()); }

			v8::MaybeLocal<v8::Value> getJsonData(const int moduleHash) const;
			v8::MaybeLocal<v8::Module> loadModule(const char* filePath) const;
			v8::MaybeLocal<v8::Module> loadJsonModule(const char* filePath) const;
			void loadEntryModule() const;

			v8::Local<v8::Promise> doAsyncWork(WorkCallback work, ResolverCallback resolver = Env::defaultAsyncResolver, void* data = nullptr, bool onMainThread = false) const;
			bool doBlockingWork(WorkCallback work, void* data, bool onMainThread) const;

			v8::Local<v8::Promise> sendMessageToWorker(NativeJS::Worker* receiver, std::string&& message) const;
			v8::Local<v8::Value> createEvent(Event* event) const;
			bool addTimeout(v8::Local<v8::Function> func, v8::Local<v8::Value> ms, v8::Local<v8::Value> timeoutObj, v8::Local<v8::Value> loopVal, size_t& index) const;
			void resolveTimeout(const size_t index) const;
			void removeTimeout(size_t index) const;

				void initializeJSApp(v8::Local<v8::Value> value) const;
			bool isJsAppInitialized() const;
			JS::App& jsApp() const;
			const JS::EnvClasses& getJsClasses() const;

			void addJsWorker(NativeJS::Worker* worker, v8::Local<v8::Value> jsWorker) const;
			void removeJsWorker(NativeJS::Worker* worker) const;
			JS::Worker& getJsWorker() const;
			bool getJsParentWorker(JS::Worker*& worker) const;


			bool isSelfWorker(NativeJS::Worker* worker) const;
			void emitMessage(MessageEvent& e);

		private:
			void initialize(NativeJS::Worker* worker);

		private:
			NativeJS::Worker* parentWorker_;
			NativeJS::Worker* worker_;
			std::filesystem::path entry_;
			v8::Eternal<v8::External> externalRef_;
			v8::Eternal<v8::Symbol> internalSymbol_;
			v8::Eternal<v8::Module> nativeJSModule_;

			JS::EnvClasses jsClasses_;
			mutable JS::App jsApp_;
			mutable JS::Worker jsSelfWorker_;
			mutable std::unordered_map<NativeJS::Worker*, JS::Worker> jsWorkers_;
			mutable bool isJsAppInitialized_;

			mutable std::unordered_map<int, std::string> modulesPaths_;
			mutable std::unordered_map<std::string, v8::Persistent<v8::Module>*> modules_;
			mutable std::unordered_map<int, v8::Persistent<v8::Value>*> jsonModules_;

			mutable PersistentList<Timeout> timeouts_;
		};
	}
}