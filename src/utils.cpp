#include "framework.hpp"
#include "utils.hpp"

namespace NativeJS
{
	namespace Utils
	{
		const std::wstring toWString(const char* str)
		{
			size_t length = strlen(str) + 1;
			std::wstring wc(length, L'\0');
			size_t numOfConverted = 0;
			mbstowcs_s(&numOfConverted, &wc[0], length, str, length);
			return wc;
		}

		void formatPath(std::string& path)
		{
			formatPath(path.data());
		}

		void formatPath(char* path)
		{
			char c;
			size_t i = 0;
			while ((c = path[i++]) != '\0')
			{
				if (c == '\\')
					path[i - 1] = '/';
			}
		}

		size_t getCurrentThreadID()
		{
#ifdef _WINDOWS
			return GetCurrentThreadId();
#endif
			return 0;
		}
	}
}