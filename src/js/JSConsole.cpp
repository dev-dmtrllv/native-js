#include "framework.hpp"
#include "js/JSConsole.hpp"
#include "js/Env.hpp"
#include "App.hpp"
#include "js/JSUtils.hpp"
#include "Logger.hpp"

namespace NativeJS
{
	namespace JS
	{
		namespace Console
		{
			namespace
			{
				constexpr size_t indentSize = 4;

				char spaceBuf_[128] = { 0 };

				inline std::string getSpaces(const size_t count)
				{
					for (size_t i = 0; i < count; i++)
						spaceBuf_[i] = ' ';
					spaceBuf_[count] = '\0';
					return std::string(spaceBuf_);
				}
			}

			std::string parse(const Env& env, v8::Local<v8::Value> val, size_t indentCount, bool isObjectVal, bool skipIndent)
			{
				using namespace v8;

				const size_t spaces = skipIndent ? 0 : indentCount * indentSize;

				std::string parsedString = getSpaces(spaces);

				if (val->IsFunction())
				{
					std::string funcName = JS::parseString(env, val.As<v8::Function>()->GetName());
					if (!isObjectVal)
						parsedString += "function: " + funcName;
					else
						parsedString += "function";
				}
				else if (val->IsPromise())
				{
					v8::Local<v8::Promise> promise = val.As<Promise>();

					switch (promise->State())
					{
						case Promise::PromiseState::kFulfilled:
							parsedString += "Promise [fulfilled]: ";
							break;
						case Promise::PromiseState::kPending:
							parsedString += "Promise [pending...]";
							break;
						case Promise::PromiseState::kRejected:
							parsedString += "Promise [rejected]: ";
							break;
					}

					if (promise->State() != Promise::PromiseState::kPending)
						parsedString += parse(env, promise->Result(), indentCount + 1, true, true);

				}
				else if (val->IsArray())
				{
					Local<Array> arr = val.As<Array>();
					const uint32_t l = arr->Length();
					if (l == 0)
					{
						parsedString += "Array(0)";
					}
					else
					{
						parsedString += "Array(" + std::to_string(l) + ") [\n";
						for (uint32_t i = 0; i < l; i++)
							parsedString += parse(env, arr->Get(env.context(), i).ToLocalChecked(), indentCount + 1, true);
						parsedString += getSpaces(indentCount * indentSize) + "]";
					}
				}
				else if (val->IsObject())
				{
					Local<v8::Object> obj = val.As<v8::Object>();
					Local<Array> keys = obj->GetPropertyNames(env.context()).ToLocalChecked();

					const uint32_t l = keys->Length();
					parsedString += +"[" + parseString(env, obj->GetConstructorName()) + "] {\n";
					const size_t innerIndent = indentCount + 1;
					const size_t innerSpaces = innerIndent * indentSize;
					std::string innerSpacesString = getSpaces(innerSpaces);
					for (uint32_t i = 0; i < l; i++)
					{
						Local<Value> key = keys->Get(env.context(), i).ToLocalChecked();
						Local<Value> val = obj->Get(env.context(), key).ToLocalChecked();

						String::Utf8Value keyValue(env.isolate(), key);
						parsedString += innerSpacesString + *keyValue + ": " + parse(env, val, innerIndent, true, true);
					}

					parsedString += getSpaces(indentCount * indentSize) + "}";
				}
				else if (val->IsNumber())
				{
					if (val->IsUint32())
					{
						parsedString += std::to_string(val->Uint32Value(env.context()).ToChecked());
					}
					else if (val->IsInt32())
					{
						parsedString += std::to_string(val->Int32Value(env.context()).ToChecked());
					}
					else
					{
						parsedString += std::to_string(val->NumberValue(env.context()).ToChecked());
					}
				}
				else
				{
					String::Utf8Value utf8Value(env.isolate(), val);
					if (isObjectVal)
						parsedString += "\"" + std::string(*utf8Value) + "\"";
					else
						parsedString += std::string(*utf8Value);
				}

				if (isObjectVal)
					parsedString += ",\n";

				return parsedString;
			}

			void logValue(const Env& env, v8::Local<v8::Value> val)
			{
				std::string str = parse(env, val);
				const size_t l = str.size();
				if (str.c_str()[l] == '\n')
					str.data()[l] = '\0';
				env.app().logger().info(str);
			}

			JS_CALLBACK(log)
			{
				const int length = args.Length();

				std::string logString;

				for (int i = 0; i < length; i++)
				{
					std::string str = parse(env, args[i]);
					const size_t l = str.size();
					if (str.c_str()[l] == '\n')
						str.data()[l] = '\0';

					logString += str;

					if (i != length - 1)
						logString += ", ";
				}

				env.app().logger().info(logString);
			}

			JS_CALLBACK(clear)
			{
#ifdef _WINDOWS
				COORD topLeft = { 0, 0 };
				HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
				CONSOLE_SCREEN_BUFFER_INFO screen;
				DWORD written;

				GetConsoleScreenBufferInfo(console, &screen);
				FillConsoleOutputCharacterA(
					console, ' ', screen.dwSize.X * screen.dwSize.Y, topLeft, &written
				);
				FillConsoleOutputAttribute(
					console, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE,
					screen.dwSize.X * screen.dwSize.Y, topLeft, &written
				);
				SetConsoleCursorPosition(console, topLeft);
#else
				puts("\x1B[2J\x1B[H");
#endif
			}


			void expose(const Env& env, JS::Object& global)
			{
				auto console = JS::Object(env);
				console.set("log", Console::log);
				console.set("clear", Console::clear);
				global.set("console", *console);
			}
		}
	};
};