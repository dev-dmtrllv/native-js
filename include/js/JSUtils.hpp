#pragma once

#include "framework.hpp"
#include "concepts.hpp"

namespace NativeJS::JS
{
	class BaseEnv;

	v8::Local<v8::Number> number(const BaseEnv& env, uint8_t number);
	v8::Local<v8::Number> number(const BaseEnv& env, uint16_t number);
	v8::Local<v8::Number> number(const BaseEnv& env, uint32_t number);
	v8::Local<v8::BigInt> number(const BaseEnv& env, uint64_t number);
	v8::Local<v8::Number> number(const BaseEnv& env, int8_t number);
	v8::Local<v8::Number> number(const BaseEnv& env, int16_t number);
	v8::Local<v8::Number> number(const BaseEnv& env, int32_t number);
	v8::Local<v8::BigInt> number(const BaseEnv& env, int64_t number);
	v8::Local<v8::Number> number(const BaseEnv& env, float number);
	v8::Local<v8::Number> number(const BaseEnv& env, double number);

	v8::Local<v8::String> string(const BaseEnv& env, const char* str);
	v8::Local<v8::String> string(const BaseEnv& env, const std::string& str);
	v8::Local<v8::String> string(const BaseEnv& env, std::string&& str);

	std::string parseString(const BaseEnv& env, v8::Local<v8::Value> val);
	bool parseString(const BaseEnv& env, v8::Local<v8::Value> val, std::string& target);

	bool getFromObject(const BaseEnv& env, v8::Local<v8::Value> val, const char* key, v8::Local<v8::Value>& out);
	bool getFromObject(const BaseEnv& env, v8::Local<v8::Value> val,  v8::Local<v8::Value> key, v8::Local<v8::Value>& out);
	v8::MaybeLocal<v8::Value> getFromObject(const BaseEnv& env, v8::Local<v8::Value> val, const char* key);
	v8::MaybeLocal<v8::Value> getFromObject(const BaseEnv& env, v8::Local<v8::Value> val,  v8::Local<v8::Value> key);

	template<typename T>
	bool getFromObject(v8::Local<v8::Context> ctx, v8::Local<v8::Value> val, v8::Local<v8::Value> key, v8::Local<T>& out)
	{
		v8::Local<v8::Object> obj;
		if (val->ToObject(ctx).ToLocal(&obj))
		{
			v8::Local<v8::Value> val;
			if(obj->Get(ctx, key).ToLocal(&val))
			{
				out = val.As<T>();
				return true;
			}
		}
		return false;
	}

	v8::Local<v8::Array> mapStringArray(const BaseEnv& env, std::vector<const char*>& vec);
	v8::Local<v8::Array> mapStringArray(const BaseEnv& env, std::vector<std::string>& vec);

	template<typename T>
	T* getInternalPointer(const v8::FunctionCallbackInfo<v8::Value> &args, size_t i = 0)
	{
		return static_cast<T*>(args.This()->GetInternalField(i).As<v8::External>()->Value());
	}

	void setInternalPointer(const v8::FunctionCallbackInfo<v8::Value> &args, void* pointer, size_t i = 0);

	template<typename T>
	T* parseExternal(const BaseEnv& env, v8::Local<v8::External> external) { return static_cast<T*>(external->Value()); }

	template<typename T>
	T* parseExternal(const BaseEnv& env, v8::Local<v8::Value> val) { return val->IsExternal() ? static_cast<T*>(val.As<v8::External>()->Value()) : nullptr; }
	

}