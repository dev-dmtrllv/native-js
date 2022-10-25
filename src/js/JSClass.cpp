
#include "framework.hpp"
#include "js/JSClass.hpp"
#include "js/Env.hpp"

namespace NativeJS::JS
{
	ObjectWrapper::ObjectWrapper(const Env& env) : env_(env) { }
	ObjectWrapper::~ObjectWrapper() { }

	const Env& ObjectWrapper::env() { return env_; };
	v8::Local<v8::Value> ObjectWrapper::value() { return value_.Get(env_.isolate()); }

	void ObjectWrapper::wrap(v8::Local<v8::Value> value)
	{
		value_.Reset(env_.isolate(), value);
		initializeProps();
	}

	void ObjectWrapper::loadMethod(v8::Persistent<v8::Function>& member, const char* objKey)
	{
		loadMethod(member, getFromObject(env_, value(), objKey).ToLocalChecked().As<v8::Function>());
	}

	void ObjectWrapper::loadMethod(v8::Persistent<v8::Function>& member, v8::Local<v8::Function> val)
	{
		member.Reset(env_.isolate(), val);
	}

	void ObjectWrapper::loadMethod(v8::Persistent<v8::Function>& member, v8::Local<v8::Value> val)
	{
		loadMethod(member, val.As<v8::Function>());
	}

	void ObjectWrapper::loadProp(v8::Persistent<v8::Value>& member, const char* objKey)
	{
		loadProp(member, getFromObject(env_, value(), objKey).ToLocalChecked());
	}

	void ObjectWrapper::loadProp(v8::Persistent<v8::Value>& member, v8::Local<v8::Value> val)
	{
		member.Reset(env_.isolate(), val);
	}


	void ClassBuilder::emptyFunction(const v8::FunctionCallbackInfo<v8::Value>& args) { }

	ClassBuilder::ClassBuilder(const Env& env, void* self) :
		self_(self),
		env_(env),
		internalFieldCount_(0),
		template_(v8::FunctionTemplate::New(env_.isolate(), nullptr, v8::External::New(env_.isolate(), self)))
	{

	}

	const ClassBuilder& ClassBuilder::setStatic(const char* key, v8::Local<v8::Value> val, bool readonly) const
	{
		template_->Set(env_.isolate(), key, val, readonly ? v8::PropertyAttribute::ReadOnly : v8::PropertyAttribute::None);
		return *this;
	}

	const ClassBuilder& ClassBuilder::setStaticMethod(const char* name, v8::FunctionCallback callback, v8::Local<v8::Value> data) const
	{
		v8::Isolate* isolate = env_.isolate();

		if (data.IsEmpty())
			data = v8::External::New(isolate, self_);

		template_->Set(isolate, name, v8::FunctionTemplate::New(isolate, callback, data));
		return *this;
	}

	const ClassBuilder& ClassBuilder::setStaticMethod(const char* name, v8::FunctionCallback callback, void* data) const
	{
		setStaticMethod(name, callback, v8::External::New(env_.isolate(), data));
		return *this;
	}

	const ClassBuilder& ClassBuilder::set(const char* key, v8::Local<v8::Value> val, bool readonly, bool isPrivate) const
	{
		if (isPrivate)
			template_->PrototypeTemplate()->SetPrivate(v8::Private::New(env_.isolate(), string(env_, key)), val, readonly ? v8::PropertyAttribute::ReadOnly : v8::PropertyAttribute::None);
		else
			template_->PrototypeTemplate()->Set(env_.isolate(), key, val, readonly ? v8::PropertyAttribute::ReadOnly : v8::PropertyAttribute::None);
		return *this;
	}

	const ClassBuilder& ClassBuilder::setMethod(const char* name, v8::FunctionCallback callback, v8::Local<v8::Value> data, v8::PropertyAttribute attr) const
	{
		if (data.IsEmpty())
			data = v8::External::New(env_.isolate(), self_);
		template_->PrototypeTemplate()->Set(env_.isolate(), name, v8::FunctionTemplate::New(env_.isolate(), callback, data), attr);
		return *this;
	}

	const ClassBuilder& ClassBuilder::setMethod(const char* name, v8::FunctionCallback callback, void* data) const
	{
		setMethod(name, callback, v8::External::New(env_.isolate(), data));
		return *this;
	}

	const ClassBuilder& ClassBuilder::setInternalFieldCount(const int count) const
	{
		template_->InstanceTemplate()->SetInternalFieldCount(count);
		template_->PrototypeTemplate()->SetInternalFieldCount(count);
		internalFieldCount_ = count;
		return *this;
	}

	const ClassBuilder& ClassBuilder::setConstructor(v8::FunctionCallback callback, v8::Local<v8::Value> data) const
	{
		template_->SetCallHandler(callback, data);
		return *this;
	}

	const ClassBuilder& ClassBuilder::setConstructor(v8::FunctionCallback callback, void* data) const
	{
		template_->SetCallHandler(callback, v8::External::New(env_.isolate(), data));
		return *this;
	}

	v8::Local<v8::Function> ClassBuilder::getClass() const
	{
		return template_->GetFunction(env_.context()).ToLocalChecked();
	}

	Class::Class(const Env& env) :
		env(env),
		persistent_()
	{

	}

	Class::~Class() { }

	void Class::initialize()
	{
		if (!persistent_.IsEmpty())
			throw std::runtime_error("Class is already initialized!");
		ClassBuilder builder(env, this);
		create(builder);
		persistent_.Reset(env.isolate(), builder.getClass());
	}

	v8::Local<v8::Function> Class::getClass() const
	{
		assert(!persistent_.IsEmpty());
		return persistent_.Get(env.isolate());
	}

	v8::MaybeLocal<v8::Value> Class::instantiate(const std::vector<v8::Local<v8::Value>>& args) const
	{
		return getClass()->CallAsConstructor(env.context(), args.size(), const_cast<v8::Local<v8::Value>*>(args.data()));
	}

	v8::MaybeLocal<v8::Value> Class::instantiate(std::vector<v8::Local<v8::Value>>&& args) const
	{
		return getClass()->CallAsConstructor(env.context(), args.size(), args.data());
	}


}