#pragma once

#include "framework.hpp"

namespace NativeJS
{
	enum class LogSeverity
	{
		DEFAULT = 0,
		INFO = 1,
		WARNING = 2,
		ERROR = 3,
		DEBUG = 4
	};

	class Logger
	{
	private:
		static inline constexpr WORD createColor(int foreground, int background = 0)
		{
			return ((background & 0x0F) << 4) + (foreground & 0x0F);
		}

		struct LogInfo
		{
			bool isNewLine = false;
			Logger* logger;
			std::string data;
		};

		static std::optional<std::thread> logHandlerThread_;
		static std::mutex mutex_;
		static std::condition_variable cv_;

		static std::unordered_map<std::string, Logger*> loggers_;
		static std::queue<LogInfo> logQueue_;

		static bool shouldTerminate_;

		static std::string& date();

	public:
		static Logger& get(const char* path = nullptr, const char* name = nullptr);

		static void terminate();

	private:
#ifndef _WINDOWS 
		static const char* DEFAULT_COLOR;
		static const char* INFO_COLOR;
		static const char* WARN_COLOR;
		static const char* ERROR_COLOR;
		static const char* DEBUG_COLOR;
#else
		static WORD DEFAULT_COLOR;
		static WORD INFO_COLOR;
		static WORD WARN_COLOR;
		static WORD ERROR_COLOR;
		static WORD DEBUG_COLOR;
#endif

		std::string path_;
		std::ofstream logFile_;

#ifdef _WINDOWS
		HANDLE hConsole;
#endif

		void forward(const char* str, bool newLine = false);
		void forward(std::string& str, bool newLine = false);

	public:
		Logger(const char* path);
		~Logger();

	private:
		void logRest(char* str);
		void logRest(const char* str);
		void logRest(std::string& str);

		template<typename T>
		void logRest(T& other)
		{
			std::string s = std::to_string(other);
			logRest(s);
		}

		template<typename T, typename... Rest>
		void logRest(T str, Rest... rest)
		{
			logRest(str);
			logRest(rest...);
		}

		void log(const char* str);
		void log(std::string& str);
		void log(std::string&& str);
		// void log(std::string str);

		template<typename T>
		void log(T str)
		{
			std::string s = std::to_string(str);
			log(s);
		}

		template<typename T, typename... Ts>
		void log(T str, Ts... rest)
		{
			printf("%s", str);
			forward(str);
			log(rest...);
		}

	public:
		template<typename T>
		void info(T str)
		{
#ifdef _WINDOWS
			SetConsoleTextAttribute(hConsole, INFO_COLOR);
			printf("[INFO] ");
			SetConsoleTextAttribute(hConsole, DEFAULT_COLOR);
#endif
			forward("[INFO] ", true);
			log(str);
		}

		template<typename T>
		void warn(T str)
		{
#ifdef _WINDOWS
			SetConsoleTextAttribute(hConsole, WARN_COLOR);
			printf("[WARN] ");
			SetConsoleTextAttribute(hConsole, DEFAULT_COLOR);
#endif
			forward("[WARN] ", true);
			log(str);
		}

		template<typename T>
		void error(T str)
		{
#ifdef _WINDOWS
			SetConsoleTextAttribute(hConsole, ERROR_COLOR);
			printf("[ERROR] ");
			SetConsoleTextAttribute(hConsole, DEFAULT_COLOR);
#endif
			forward("[ERROR] ", true);
			log(str);
		}

		template<typename T>
		void debug(T str)
		{
#ifdef _DEBUG
#ifdef _WINDOWS
			SetConsoleTextAttribute(hConsole, DEBUG_COLOR);
			printf("[DEBUG] ");
			SetConsoleTextAttribute(hConsole, DEFAULT_COLOR);
#endif
			forward("[DEBUG] ", true);
			log(str);
#endif
		}

		template<typename T, typename... Ts>
		void info(T str, Ts... rest)
		{
#ifdef _WINDOWS
			SetConsoleTextAttribute(hConsole, INFO_COLOR);
			printf("[INFO] ");
			SetConsoleTextAttribute(hConsole, DEFAULT_COLOR);
			printf(str);
#endif
			forward("[INFO] ", true);
			forward(str);
			logRest(rest...);
			printf("\n");
			forward("\n");
		}

		template<typename T, typename... Ts>
		void warn(T str, Ts... rest)
		{
#ifdef _WINDOWS
			SetConsoleTextAttribute(hConsole, WARN_COLOR);
			printf("[WARN] ");
			SetConsoleTextAttribute(hConsole, DEFAULT_COLOR);
			printf(str);
#endif
			forward("[WARN] ", true);
			forward(str);
			logRest(rest...);
			printf("\n");
			forward("\n");
		}

		template<typename T, typename... Ts>
		void error(T str, Ts... rest)
		{
#ifdef _WINDOWS
			SetConsoleTextAttribute(hConsole, ERROR_COLOR);
			printf("[ERROR] ");
			SetConsoleTextAttribute(hConsole, DEFAULT_COLOR);
			printf(str);
#endif
			forward("[ERROR] ", true);
			forward(str);
			logRest(rest...);
			printf("\n");
			forward("\n");
		}

		template<typename T, typename... Ts>
		void debug(T str, Ts... rest)
		{
#ifdef _DEBUG
#ifdef _WINDOWS
			SetConsoleTextAttribute(hConsole, DEBUG_COLOR);
			printf("[DEBUG] ");
			SetConsoleTextAttribute(hConsole, DEFAULT_COLOR);
			printf(str);
#endif
			forward("[DEBUG] ", true);
			forward(str);
			logRest(rest...);
			printf("\n");
			forward("\n");
#endif
		}

		template<typename... Ts>
		void log(LogSeverity type, Ts... rest)
		{
			switch (type)
			{
				case LogSeverity::INFO:
					info(rest...);
					break;
				case LogSeverity::WARNING:
					warn(rest...);
					break;
				case LogSeverity::ERROR:
					error(rest...);
					break;
				case LogSeverity::DEBUG:
					debug(rest...);
					break;
				case LogSeverity::DEFAULT:
				default:
#ifdef _DEBUG
					debug(rest...);
#else
					info(rest...);
#endif
					break;

			}
		}

		template<typename T>
		void log(LogSeverity type, T str)
		{
			switch (type)
			{
				case LogSeverity::INFO:
					info(str);
					break;
				case LogSeverity::WARNING:
					warn(str);
					break;
				case LogSeverity::ERROR:
					error(str);
					break;
				case LogSeverity::DEBUG:
					debug(str);
					break;
				case LogSeverity::DEFAULT:
				default:
#ifdef _DEBUG
					debug(str);
#else
					info(str);
#endif
					break;
			}
		}
	};
}
