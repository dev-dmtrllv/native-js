#include "framework.hpp"
#include "js/Env.hpp"
#include "App.hpp"
#include "js/JSUtils.hpp"
#include "js/JSObject.hpp"
#include "js/NativeJSModule.hpp"
#include "js/JSGlobals.hpp"

namespace NativeJS::JS
{
	Env::Scope::Scope(const BaseEnv& env) :
		isolateScope_(env.isolate()),
		handleScope_(env.isolate()),
		contextScope_(env.context())
	{ }

	Env::Scope::~Scope() { }

	Env::Env(NativeJS::App& app, NativeJS::Worker* worker, const std::filesystem::path& entry, NativeJS::Worker* parentWorker) :
		BaseEnv(app),
		worker_(worker),
		parentWorker_(parentWorker),
		isJsAppInitialized_(false),
		entry_(entry),
		nativeJSModule_(),
		jsApp_(*this),
		jsSelfWorker_(*this),
		jsClasses_(*this)
	{
		initialize(worker);
	}

	Env::Env(NativeJS::App& app, NativeJS::Worker* worker, std::filesystem::path&& entry, NativeJS::Worker* parentWorker) :
		BaseEnv(app),
		worker_(worker),
		parentWorker_(parentWorker),
		isJsAppInitialized_(false),
		entry_(entry),
		jsApp_(*this),
		jsSelfWorker_(*this),
		jsClasses_(*this)
	{
		initialize(worker);
	}

	void Env::initialize(NativeJS::Worker* worker)
	{
		Scope scope(*this);

		externalRef_.Set(isolate(), v8::External::New(isolate(), this));

		assert(isolate()->GetNumberOfDataSlots() != 0);
		isolate()->SetData(0, this);

		internalSymbol_.Set(isolate(), v8::Symbol::New(isolate(), string(*this, "INTERNAL")));

		// initialize all the js objects/classes/functions
		jsClasses_.initialize();

		// expose globals
		JS::Object global(*this, context()->Global());
		JS::JSGlobals::expose(*this, global);

		// create the native-js module
		nativeJSModule_.Set(isolate(), JS::NativeJSModule::create(*this));

		jsSelfWorker_.wrap(jsClasses_.workerClass.instantiate({ v8::External::New(isolate(), worker_) }).ToLocalChecked());
		if (parentWorker_ != nullptr)
		{
			jsWorkers_.emplace(parentWorker_, *this);
			JS::Worker& w = jsWorkers_.at(parentWorker_);
			w.wrap(jsClasses_.workerClass.instantiate({ v8::External::New(isolate(), parentWorker_) }).ToLocalChecked());
			w.setWeak([](const v8::WeakCallbackInfo<ObjectWrapper>& data)
			{
				const Env& env = data.GetParameter()->env();
				NativeJS::Worker* worker = static_cast<NativeJS::Worker*>(data.GetParameter()->value().As<v8::Object>()->GetInternalField(0).As<v8::External>()->Value());
				env.removeJsWorker(worker);
			});
		}
	}

	Env::~Env()
	{
		for (auto& [first, second] : jsWorkers_)
		{
			if (first->parentWorker_ == worker_)
			{
				int exitCode = 0;
				if (first->terminate(exitCode))
				{
					app().logger().info("Child Worker exited with code ", exitCode);
				}
				else
				{
					puts(":(");
				}
			}
		}
	}

	/*static*/ v8::MaybeLocal<v8::Module> Env::importModule(v8::Local<v8::Context> context, v8::Local<v8::String> specifier, v8::Local<v8::FixedArray> import_assertions, v8::Local<v8::Module> referrer)
	{
		const int id = referrer->ScriptId();

		Env& env = *static_cast<Env*>(context->GetIsolate()->GetData(0));

		std::string import = JS::parseString(env, specifier);

		std::replace(import.begin(), import.end(), '\\', '/');

		if (import.compare("native-js") == 0)
			return v8::MaybeLocal<v8::Module>(env.nativeJSModule_.Get(env.isolate()));

		std::filesystem::path importPath = import;
		std::filesystem::path fromPath;

		if (!importPath.is_relative())
		{
			fromPath = std::filesystem::path("");
		}
		else if (env.modulesPaths_.contains(id))
		{
			fromPath = (std::filesystem::path(env.modulesPaths_.at(id)) / "..").lexically_normal();
		}
		else
		{
			fromPath = env.app().rootDir();
		}

		if (importPath.is_relative())
			importPath = fromPath / importPath;

		if (!std::filesystem::exists(importPath))
		{
			bool isValid = false;
			for (const std::string& t : env.app().appConfig().resolve)
			{
				std::filesystem::path checkPath = importPath;
				checkPath += t;
				if (std::filesystem::exists(checkPath))
				{
					importPath = checkPath;
					isValid = true;
					break;
				}
			}

			if (!isValid)
			{
				env.app().logger().warn("Could not resolve import path for ", importPath.string().c_str(), "!");
				return v8::MaybeLocal<v8::Module>();
			}
		}

		const std::string ext = importPath.extension().string();

		v8::MaybeLocal<v8::Module> module;

		if (ext.compare(".json") == 0)
		{
			module = env.loadJsonModule(importPath.string().c_str());
		}
		else
		{
			module = env.loadModule(importPath.string().c_str());
		}

		return v8::MaybeLocal<v8::Module>(module);
	}

	v8::MaybeLocal<v8::Module> Env::loadJsonModule(const char* filePath) const
	{
		using namespace v8;

		TryCatch tryCatcher(isolate());

		const std::string path(filePath);

		if (modules_.contains(path))
			return v8::MaybeLocal<v8::Module>();

		std::ifstream is(filePath);
		std::string jsonString((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());

		std::vector<v8::Local<v8::String>> exports({ JS::string(*this, "default") });

		Local<Module> module = Module::CreateSyntheticModule(isolate(), JS::string(*this, filePath), exports, [](Local<Context> context, Local<Module> module)
		{
			Env& env = *static_cast<Env*>(context->GetIsolate()->GetData(0));
			module->SetSyntheticModuleExport(context->GetIsolate(), v8::String::NewFromUtf8(context->GetIsolate(), "default").ToLocalChecked(), env.getJsonData(module->GetIdentityHash()).ToLocalChecked());
			return MaybeLocal<Value>(v8::True(env.isolate()));
		});

		Local<Value> json = v8::JSON::Parse(context(), JS::string(*this, jsonString)).ToLocalChecked();

		jsonModules_.emplace(module->GetIdentityHash(), new Persistent<Value>(isolate(), json));
		modules_.emplace(path, new Persistent<Module>(isolate(), module));

		Maybe<bool> result = module->InstantiateModule(context(), Env::importModule);

		if (result.IsNothing())
		{
			app().logger().error("Can't instantiate module.");
			return v8::MaybeLocal<v8::Module>();
		}
		else
		{
			MaybeLocal<Value> result = module->Evaluate(context());
			return v8::MaybeLocal<v8::Module>(module);
		}
	}

	v8::MaybeLocal<v8::Value> Env::getJsonData(const int moduleHash) const
	{
		if (jsonModules_.contains(moduleHash))
			return v8::MaybeLocal<v8::Value>(jsonModules_.at(moduleHash)->Get(isolate()));
		return v8::MaybeLocal<v8::Value>();
	}

	v8::MaybeLocal<v8::Module> Env::loadModule(const char* filePath) const
	{
		using namespace v8;

		std::filesystem::path path(filePath);

		if (path.is_relative())
			path = (app().rootDir() / path).lexically_normal();

		std::string p = (path / "..").lexically_normal().string();
		std::replace(p.begin(), p.end(), '\\', '/');
		std::string fileName = path.filename().string();

		std::ifstream is(path);
		std::string code = std::string("const __dirname = \"") + p + "\"; const __filename = \"" + fileName + "\";" + std::string((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());

		TryCatch tryCatcher(isolate());

		Local<String> sourceStr = JS::string(*this, code.c_str());

		ScriptOrigin origin(isolate(), JS::string(*this, path.string().c_str()), 0, 0, true, -1, v8::Local<v8::Value>(), false, false, true);

		ScriptCompiler::Source source(sourceStr, origin);
		MaybeLocal<Module> maybeModule = ScriptCompiler::CompileModule(isolate(), &source);

		if (maybeModule.IsEmpty())
		{
			if (tryCatcher.HasCaught())
			{
				std::string exception = JS::parseString(*this, tryCatcher.Exception());
				app().logger().warn("Got exception while loading module ", filePath, "!\n", exception);
			}

			app().logger().warn("Module ", path.string(), " is empty!");
			return v8::MaybeLocal<v8::Module>();
		}
		else if (tryCatcher.HasCaught())
		{
			std::string exception = JS::parseString(*this, tryCatcher.Exception());
			app().logger().warn("Got exception while loading module ", filePath, "!\n", exception);
		}
		else
		{
			Local<Module> module = maybeModule.ToLocalChecked();

			modulesPaths_.emplace(module->ScriptId(), path.string());
			modules_.emplace(path.string(), new Persistent<Module>(isolate(), module));

			Maybe<bool> result = module->InstantiateModule(context(), importModule);

			if (result.IsNothing())
			{
				app().logger().warn("Could not instantiate module ", path.string(), "!");
				return module;
			}
			else
			{
				module->Evaluate(context());
				return module;
			}
		}
	}

	void Env::loadEntryModule() const
	{
		using namespace v8;

		TryCatch tryCatcher(isolate());

		std::filesystem::path path(entry_);

		if (path.is_relative())
			path = (app().rootDir() / path).lexically_normal();

		std::string p = path.string();
		std::replace(p.begin(), p.end(), '\\', '/');

		std::string entry = path.string() + ".native.entry";

		std::string code;

		if (parentWorker_ == nullptr)
		{
			code = std::format("import entry from \"{}\"; {}", p, "entry(process.args);");
		}
		else
		{
			code = std::format("import entry from \"{}\"; {}", p, "entry(Worker.getParentWorker(), process.args);");
		}

		Local<String> sourceStr = JS::string(*this, code);
		ScriptOrigin origin(isolate(), JS::string(*this, entry), 0, 0, true, -1, v8::Local<v8::Value>(), false, false, true);
		ScriptCompiler::Source source(sourceStr, origin);

		MaybeLocal<Module> maybeModule = ScriptCompiler::CompileModule(isolate(), &source);

		if (maybeModule.IsEmpty())
		{
			app().logger().warn("Could not compile module ", p, "!");
			return;
		}
		else if (tryCatcher.HasCaught())
		{
			std::string exception = JS::parseString(*this, tryCatcher.Exception());
			app().logger().warn("Got exception while loading module ", p, "!\n", exception);
		}
		else
		{
			Local<Module> module = maybeModule.ToLocalChecked();

			modulesPaths_.emplace(module->ScriptId(), entry);
			modules_.emplace(entry, new Persistent<Module>(isolate(), module));

			Maybe<bool> result = module->InstantiateModule(context(), importModule);

			if (result.IsNothing())
			{
				app().logger().warn("Could not instantiate module ", p, "!");
			}
			else
			{
				module->Evaluate(context());
			}
		}
	}

	void Env::defaultAsyncResolver(const NativeJS::WorkEvent& e)
	{
		e.resolvePromise();
	}

	v8::Local<v8::Promise> Env::doAsyncWork(WorkCallback work, ResolverCallback resolver, void* data, bool onMainThread) const
	{
		AsyncEvent* event = worker_->events_.create<AsyncEvent>(*worker_, work, resolver == nullptr ? defaultAsyncResolver : resolver, data);
		app().postEvent(event, onMainThread);
		return event->promise();
	}

	bool Env::doBlockingWork(WorkCallback work, void* data, bool onMainThread) const
	{
		return worker_->doBlockingWork(work, data, onMainThread);
	}

	v8::Local<v8::Promise> Env::sendMessageToWorker(NativeJS::Worker* receiver, std::string&& message) const
	{
		MessageEvent* event = worker_->events_.create<MessageEvent>(worker_, receiver, std::forward<std::string>(message));
		receiver->postEvent(event);
		return event->promise();
	}

	void Env::emitMessage(MessageEvent& e)
	{
		NativeJS::Worker* w = std::addressof(e.sender());
		if (jsWorkers_.contains(w))
		{
			JS::Worker& jsWorker = jsWorkers_.at(w);
			jsWorker.emitMessage(e.message());
		}
	}

	void Env::addJsWorker(NativeJS::Worker* worker, v8::Local<v8::Value> jsWorker) const
	{
		jsWorkers_.emplace(worker, *this);
		JS::Worker& w = jsWorkers_.at(worker);
		w.wrap(jsClasses_.workerClass.instantiate({ v8::External::New(isolate(), worker) }).ToLocalChecked());
		w.setWeak([](const v8::WeakCallbackInfo<ObjectWrapper>& data)
		{
			puts("weak callback called");
			const Env& env = data.GetParameter()->env();
			NativeJS::Worker* worker = static_cast<NativeJS::Worker*>(data.GetParameter()->value().As<v8::Object>()->GetInternalField(0).As<v8::External>()->Value());
			env.removeJsWorker(worker);
		});
	}

	void Env::removeJsWorker(NativeJS::Worker* worker) const
	{
		if (jsWorkers_.contains(worker))
			jsWorkers_.erase(worker);
	}

	bool Env::addTimeout(v8::Local<v8::Function> func, v8::Local<v8::Value> msVal, v8::Local<v8::Value> timeoutObj, v8::Local<v8::Value> loopVal, size_t& index) const
	{
		if (!loopVal.IsEmpty() && !loopVal->IsBoolean())
		{
			throwException("Provided loop argument is not a boolean!");
			return false;
		}

		int64_t ms = 0;

		if (!parseNumber(context(), msVal, ms))
		{
			throwException("Provided timeout is not a number or BigInt!");
			return false;
		}
		const bool loop = loopVal.IsEmpty() ? false : loopVal->BooleanValue(isolate());

		std::chrono::milliseconds resolveTime = Utils::now<std::chrono::milliseconds>() + std::chrono::milliseconds(ms);

		index = timeouts_.alloc(*this, resolveTime, loop);
		Timeout* t = timeouts_.at(index);
		t->setIndex(index);
		t->wrap(timeoutObj);
		app().postEvent(worker_->events_.create<TimeoutEvent>(*this, index, resolveTime, ms, loop), true);

		return true;
	}

	void Env::removeTimeout(size_t index) const
	{
		// timeouts_.free(index);
		app().postEvent(worker_->events_.create<TimeoutEvent>(*this, index, TimeoutEvent::Type::CANCEL), true);
	}

	// void Env::removeTimeout(size_t index) const
	// {
	// 	timeouts_.free(index);
	// 	app().postEvent(worker_->events_.create<TimeoutEvent>(*this, index, TimeoutEvent::Type::CANCEL), true);
	// }

	void Env::resolveTimeout(const size_t index) const
	{
		Timeout* timeout = timeouts_.at(index);
		timeout->resolve();
		if (!timeout->loop())
			timeouts_.free(index);
	}

	bool Env::isJsAppInitialized() const
	{
		return isJsAppInitialized_;
	}

	JS::App& Env::jsApp() const
	{
		return jsApp_;
	}

	void Env::initializeJSApp(v8::Local<v8::Value> value) const
	{
		jsApp_.wrap(value);
		isJsAppInitialized_ = true;
	}

	v8::Local<v8::Value> Env::createEvent(Event* event) const
	{
		return getJsClasses().eventClass.instantiate({ v8::External::New(isolate(), event) }).ToLocalChecked();
	}

	const JS::EnvClasses& Env::getJsClasses() const
	{
		return jsClasses_;
	}

	JS::Worker& Env::getJsWorker() const
	{
		return jsSelfWorker_;
	}


	bool Env::getJsParentWorker(JS::Worker*& worker) const
	{
		if (parentWorker_ != nullptr && jsWorkers_.contains(parentWorker_))
		{
			worker = std::addressof(jsWorkers_.at(parentWorker_));
			return true;
		}
		worker = nullptr;
		return false;
	}

	bool Env::isSelfWorker(NativeJS::Worker* worker) const
	{
		return worker == std::addressof(this->worker());
	}
}