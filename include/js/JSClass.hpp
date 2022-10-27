#pragma once

#include "framework.hpp"
#include "concepts.hpp"
#include "utils.hpp"
#include "js/JSUtils.hpp"

#define JS_METHOD_DECL(__NAME__) private: v8::Persistent<v8::Function> __NAME__##_; \
public: void __NAME__(std::initializer_list<v8::Local<v8::Value>> args = {});

#define JS_METHOD_IMPL(__NAME__) void __NAME__(std::initializer_list<v8::Local<v8::Value>> args) { __NAME__##_.Get(env_.isolate())->CallAsFunction(env_.context(), value_.Get(env_.isolate()), args.size(), const_cast<v8::Local<v8::Value>*>(args.begin())); }

#define JS_CLASS_BODY(__NAME__) public: \
	__NAME__(const Env& env) : Class(env) { } \
	~__NAME__() {  } \
protected: \
	virtual void create(const ClassBuilder& builder);

#define JS_CLASS_METHOD(__NAME__) static void __NAME__##__WRAPPED__(const JS::Env& env, const v8::FunctionCallbackInfo<v8::Value>& args); \
static void __NAME__(const v8::FunctionCallbackInfo<v8::Value>& args);

#define JS_CLASS_METHOD_IMPL(__NAME__) void __NAME__(const v8::FunctionCallbackInfo<v8::Value>& args) { __NAME__##__WRAPPED__(JS::Env::fromArgs(args), args); } \
void __NAME__##__WRAPPED__(const JS::Env& env, const v8::FunctionCallbackInfo<v8::Value>& args) 

#define JS_CREATE_CLASS(__NAME__) void __NAME__::create(const ClassBuilder& builder)

namespace NativeJS
{
	namespace JS
	{
		class Env;

		class ObjectWrapper
		{
		public:
			ObjectWrapper(const Env& env);
			virtual ~ObjectWrapper() = 0;

			void wrap(v8::Local<v8::Value> value);

			const Env& env();
			v8::Local<v8::Value> value();

			template<typename T = ObjectWrapper>
				requires Concepts::Extends<ObjectWrapper, T>
			void setWeak(typename v8::WeakCallbackInfo<T>::Callback callback)
			{
				value_.SetWeak<T>(static_cast<T*>(this), callback, v8::WeakCallbackType::kParameter);
			}

		protected:
			virtual void initializeProps() = 0;

			void loadMethod(v8::Persistent<v8::Function>& member, const char* objKey);
			void loadMethod(v8::Persistent<v8::Function>& member, v8::Local<v8::Function> val);
			void loadMethod(v8::Persistent<v8::Function>& member, v8::Local<v8::Value> val);

			void loadProp(v8::Persistent<v8::Value>& member, const char* objKey);
			void loadProp(v8::Persistent<v8::Value>& member, v8::Local<v8::Value> val);

			const Env& env_;
			v8::Persistent<v8::Value> value_;
		};

		class ClassBuilder
		{
		private:
			static void emptyFunction(const v8::FunctionCallbackInfo<v8::Value>& args);
		public:
			ClassBuilder(const Env& env, void* self);

			const ClassBuilder& setStatic(const char* key, v8::Local<v8::Value> val, bool readonly = false) const;

			const ClassBuilder& setStaticMethod(const char* name, v8::FunctionCallback callback = ClassBuilder::emptyFunction, v8::Local<v8::Value> data = v8::Local<v8::Value>()) const;

			const ClassBuilder& setStaticMethod(const char* name, v8::FunctionCallback callback, void* data) const;

			const ClassBuilder& set(const char* key, v8::Local<v8::Value> val, bool readonly = false, bool isPrivate = false) const;

			const ClassBuilder& setMethod(const char* name, v8::FunctionCallback callback = ClassBuilder::emptyFunction, v8::Local<v8::Value> data = v8::Local<v8::Value>(), v8::PropertyAttribute attr = v8::PropertyAttribute::None) const;
			
			const ClassBuilder& setMethod(const char* name, v8::FunctionCallback callback, void* data) const;

			const ClassBuilder& setInternalFieldCount(const int count) const;

			const ClassBuilder& setConstructor(v8::FunctionCallback callback, v8::Local<v8::Value> data = v8::Local<v8::Value>()) const;

			const ClassBuilder& setConstructor(v8::FunctionCallback callback, void* data) const;

			v8::Local<v8::Function> getClass() const;

		private:
			void* self_;
			const Env& env_;
			mutable size_t internalFieldCount_;
			v8::Local<v8::FunctionTemplate> template_;
		};

		class Class
		{
		public:
			Class(const Env& env);
			Class(const Class& env) = delete;
			Class(Class&& env) = delete;
			virtual ~Class() = 0;

			void initialize();
			v8::Local<v8::Function> getClass() const;

			v8::MaybeLocal<v8::Value> instantiate(const std::vector<v8::Local<v8::Value>>& args = {}) const;

			v8::MaybeLocal<v8::Value> instantiate(std::vector<v8::Local<v8::Value>>&& args = {}) const;

		protected:
			virtual void create(const ClassBuilder& builder) = 0;

		public:
			const Env& env;

		protected:
			v8::Persistent<v8::Function> persistent_;
			size_t internalFieldCout_;
		};

		template<typename ClassType>
		static v8::Local<v8::Function> createJsClass(const Env& env)
		{
			ClassType c(env);
			c.initialize();
			return c.getClass();
		}
	}
}