#include "framework.hpp"
#include "js/Env.hpp"
#include "js/JSObject.hpp"

namespace NativeJS::JS
{
	Object::Object(const Env& env, v8::Local<v8::Value> obj) : Object(env, obj.IsEmpty() ? v8::Object::New(env.isolate()) : obj->ToObject(env.context()).ToLocalChecked()) { }
	Object::Object(const Env& env, v8::Local<v8::Object> obj) : env(env), v8Object_(obj.IsEmpty() ? v8::Object::New(env.isolate()) : obj) { }

	v8::MaybeLocal<v8::Value> Object::operator [](const char* key) const
	{
		return v8Object_->Get(env.context(), v8::String::NewFromUtf8(env.isolate(), key).ToLocalChecked());
	}

	v8::MaybeLocal<v8::Value> Object::operator [](const std::string& key) const
	{
		return this->operator[](key.c_str());
	}

	v8::MaybeLocal<v8::Value> Object::operator [](std::string&& key) const
	{
		return this->operator[](key.c_str());
	}

	bool Object::set(const char* key, v8::Local<v8::Value> val, v8::PropertyAttribute attributes) const
	{
		return v8Object_->DefineOwnProperty(env.context(), v8::String::NewFromUtf8(env.isolate(), key).ToLocalChecked(), val, attributes).FromJust();
	}
	
	bool Object::set(v8::Local<v8::String> name,  v8::Local<v8::Value> val, v8::PropertyAttribute attributes) const
	{
		return v8Object_->DefineOwnProperty(env.context(), name, val, attributes).FromJust();
	}

	bool Object::set(const char* name, v8::FunctionCallback callback, v8::Local<v8::Value> data) const
	{
		return v8Object_->Set(env.context(), v8::String::NewFromUtf8(env.isolate(), name).ToLocalChecked(), v8::FunctionTemplate::New(env.isolate(), callback, data)->GetFunction(env.context()).ToLocalChecked()).FromJust();
	}

	bool Object::set(v8::Local<v8::String> name, v8::FunctionCallback callback, v8::Local<v8::Value> data) const
	{
		return v8Object_->Set(env.context(), name, v8::FunctionTemplate::New(env.isolate(), callback, data)->GetFunction(env.context()).ToLocalChecked()).FromJust();
	}
};