#include "framework.hpp"
#include "Logger.hpp"

namespace NativeJS
{
	constexpr size_t formatBufferSize = 1024;

#define BLACK			0
#define BLUE			1
#define GREEN			2
#define CYAN			3
#define RED				4
#define MAGENTA			5
#define BROWN			6
#define LIGHTGRAY		7
#define DARKGRAY		8
#define LIGHTBLUE		9
#define LIGHTGREEN		10
#define LIGHTCYAN		11
#define LIGHTRED		12
#define LIGHTMAGENTA	13
#define YELLOW			14
#define WHITE			15


#ifndef _WINDOWS 
	const char* Logger::DEFAULT_COLOR = "\033[39m\033[49m";
	const char* Logger::INFO_COLOR = "\033[34m";
	const char* Logger::WARN_COLOR = "\033[33m";
	const char* Logger::ERROR_COLOR = "\033[31m";
	const char* Logger::DEBUG_COLOR = "\033[30m";
#else
	WORD Logger::DEFAULT_COLOR = Logger::createColor(15, 0);
	WORD Logger::INFO_COLOR = Logger::createColor(9);
	WORD Logger::WARN_COLOR = Logger::createColor(14);
	WORD Logger::ERROR_COLOR = Logger::createColor(4);
	WORD Logger::DEBUG_COLOR = Logger::createColor(1);
#endif

	std::unordered_map<std::string, Logger*> Logger::loggers_ = std::unordered_map<std::string, Logger*>();
	std::queue<Logger::LogInfo> Logger::logQueue_;
	std::mutex Logger::mutex_;
	std::condition_variable Logger::cv_;
	bool Logger::shouldTerminate_ = false;
	std::optional<std::thread> Logger::logHandlerThread_;

	std::string& Logger::date()
	{
		static std::optional<std::string> dateString;
		if (!dateString.has_value())
		{
			std::stringstream ss;
			std::time_t t = std::time(0);
			std::tm* now = std::localtime(&t);
			ss << (now->tm_year + 1900) << '-' << (now->tm_mon + 1) << '-' << now->tm_mday;
			dateString = ss.str();
		}
		return dateString.value();
	}

	Logger& Logger::get(const char* path, const char* name)
	{
		std::filesystem::path logPath;

		if (path == nullptr)
			logPath = std::filesystem::current_path() / "logs";

		if (!std::filesystem::exists(logPath))
			std::filesystem::create_directory(logPath);

		std::string fileName;

		if (name == nullptr)
			fileName = Logger::date();
		else
			fileName = std::string(name) + "-" + Logger::date();

		if (!loggers_.contains(fileName))
		{
			size_t version = 0;

			bool exists;

			std::filesystem::path logFilePath;

			do
			{
				std::string versionString = version == 0 ? "" : (std::string("-") + std::to_string(version));
				std::string genName = fileName + versionString + ".log";

				logFilePath = logPath / genName;

				exists = std::filesystem::exists(logFilePath);

				if (exists)
					version++;

			} while (exists);

			loggers_[fileName] = new Logger(logFilePath.string().c_str());
		}

		return *loggers_[fileName];
	}

	void Logger::terminate()
	{
		shouldTerminate_ = true;

		if (logHandlerThread_.has_value())
		{
			cv_.notify_one();
			if (logHandlerThread_.value().joinable())
				logHandlerThread_.value().join();
			logHandlerThread_.reset();
		}

		loggers_.clear();

		shouldTerminate_ = false;
	}

	Logger::Logger(const char* path) : logFile_(path)
	{
#ifdef _WINDOWS
		hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
#endif
		if (!logHandlerThread_.has_value())
			logHandlerThread_.emplace(std::thread([&]()
		{
			time_t rawtime;
			struct tm* timeinfo;
			char timeBuf[10] = {};

			while (!shouldTerminate_)
			{
				std::unique_lock<std::mutex> lock(mutex_);
				cv_.wait(lock);
				lock.unlock();

				while (logQueue_.size() > 0)
				{
					Logger::LogInfo& info = logQueue_.front();

					if (info.isNewLine)
					{
						time(&rawtime);
						timeinfo = localtime(&rawtime);
						strftime(timeBuf, 10, "%T", timeinfo);

						info.logger->logFile_ << "[" << timeBuf << "] ";
					}

					info.logger->logFile_ << info.data;
					info.logger->logFile_.flush();
					lock.lock();
					logQueue_.pop();
					lock.unlock();
				}
			}

			debug("Log thread terminated!");
		}));
	}

	Logger::~Logger()
	{
		if (logFile_.is_open())
			logFile_.close();
	}

	void Logger::logRest(char* str)
	{
		printf("%s", str);
		forward(str);
	}

	void Logger::logRest(const char* str)
	{
		printf("%s", str);
		forward(str);
	}

	void Logger::logRest(std::string& str)
	{
		printf("%s", str.c_str());
		forward(str.c_str());
	}

	void Logger::log(const char* str)
	{
		log(std::string(str));
	}

	void Logger::log(std::string& str)
	{
		char formatBuffer[formatBufferSize];

		// prevent buffer overflow
		if (str.size() > formatBufferSize)
		{
			str.resize(formatBufferSize - 2);
			str[formatBufferSize - 5] = '.';
			str[formatBufferSize - 4] = '.';
			str[formatBufferSize - 3] = '.';
			str[formatBufferSize - 2] = '\0';
		}

		sprintf(formatBuffer, "%s\n", str.c_str());
		printf("%s", formatBuffer);
		forward(formatBuffer);
	}

	void Logger::log(std::string&& str)
	{
		log(str);
	}

	// void Logger::log(std::string&& str)
	// {
	// 	char formatBuffer[formatBufferSize];

	// 	// prevent buffer overflow
	// 	if (str.size() > formatBufferSize)
	// 	{
	// 		str.resize(formatBufferSize - 2);
	// 		str[formatBufferSize - 5] = '.';
	// 		str[formatBufferSize - 4] = '.';
	// 		str[formatBufferSize - 3] = '.';
	// 		str[formatBufferSize - 2] = '\0';
	// 	}

	// 	sprintf(formatBuffer, "%s\n", str.c_str());
	// 	printf("%s", formatBuffer);
	// 	forward(formatBuffer);
	// }

	void Logger::forward(const char* str, bool newLine)
	{
		std::unique_lock<std::mutex> lock(mutex_);
		Logger::LogInfo info = {
			.isNewLine = newLine,
			.logger = this,
			.data = str
		};
		logQueue_.push(info);
		cv_.notify_one();
	}

	void Logger::forward(std::string& str, bool newLine)
	{
		std::unique_lock<std::mutex> lock(mutex_);
		Logger::LogInfo info = {
			.isNewLine = newLine,
			.logger = this,
			.data = str
		};
		logQueue_.push(info);
		cv_.notify_one();
	}
}