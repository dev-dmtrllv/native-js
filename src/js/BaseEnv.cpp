#include "framework.hpp"
#include "js/BaseEnv.hpp"
#include "App.hpp"
#include "js/JSUtils.hpp"
#include "js/JSObject.hpp"
#include "js/JSConsole.hpp"
#include "js/NativeJSModule.hpp"
#include "js/JSProcess.hpp"

namespace NativeJS::JS
{
	BaseEnv::BaseEnv(NativeJS::App& app) :
		app_(app)
	{
		Logger& logger = app_.logger();
		logger.debug("Creating V8 Buffer Allocator...");
		createParams_.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();

		logger.debug("Creating V8 Isolate...");
		isolate_ = v8::Isolate::New(createParams_);

		v8::Isolate::Scope isolateScope(isolate_);
		v8::HandleScope handleScope(isolate_);

		v8::Local<v8::Context> ctx = v8::Context::New(isolate_);
		v8::Context::Scope contextScope(ctx);

		context_.Set(isolate_, ctx);
		
		assert(isolate_->GetNumberOfDataSlots() != 0);
		isolate_->SetData(0, this);
	}

	BaseEnv::~BaseEnv()
	{
		Logger& logger = app_.logger();
		logger.debug("Disposing Env...");

		logger.debug("Disposing V8::Isolate...");
		isolate_->Dispose();
		logger.debug("Disposing v8::ArrayBufferAllocator...");
		delete createParams_.array_buffer_allocator;
	}

	void BaseEnv::throwException(const char* error) const
	{
		isolate_->ThrowException(string(*this, error));
	}
}