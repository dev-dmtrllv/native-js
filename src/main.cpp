#include "framework.hpp"

#include "App.hpp"

int main(int argc, char** argv)
{
	using namespace NativeJS;

	App& app = App::createFromArgs(argc, argv);

	app.run();

	return App::terminate();

	return 0;
}

#ifdef _WINDOWS
void setupWinConsole(bool bindStdIn = true, bool bindStdOut = true, bool bindStdErr = true);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	setupWinConsole(true, true, true);
	int argc = 0;
	wchar_t** argv = CommandLineToArgvW(lpCmdLine, &argc);

	std::vector<std::string> argvList;

	char buffer[512];

	for (size_t i = 0; i < argc; i++)
	{
		const wchar_t* arg = argv[i];
		size_t converted = 0;
		errno_t err = wcstombs_s(&converted, buffer, arg, 512);
		if (err != 0)
			return err;

		argvList.emplace_back(std::string(buffer));
	}

	std::vector<char*> parsedArgv;

	for (size_t i = 0; i < argc; i++)
		parsedArgv.emplace_back(argvList[i].data());

	const int exitCode = main(argc, parsedArgv.data());

	LocalFree(argv);

	return 0;
}

void setupWinConsole(bool bindStdIn, bool bindStdOut, bool bindStdErr)
{
	if (!AttachConsole(ATTACH_PARENT_PROCESS))	// try to hijack existing console of command line
		AllocConsole();							// or create your own.

	if (bindStdIn)
	{
		FILE* dummyFile;
		freopen_s(&dummyFile, "CONIN$", "r", stdin);
	}
	if (bindStdOut)
	{
		FILE* dummyFile;
		freopen_s(&dummyFile, "CONOUT$", "w", stdout);
	}
	if (bindStdErr)
	{
		FILE* dummyFile;
		freopen_s(&dummyFile, "CONOUT$", "w", stderr);
	}

	// Redirect unbuffered stdin from the current standard input handle
	if (bindStdIn)
	{
		HANDLE stdHandle = GetStdHandle(STD_INPUT_HANDLE);
		if (stdHandle != INVALID_HANDLE_VALUE)
		{
			int fileDescriptor = _open_osfhandle((intptr_t)stdHandle, _O_TEXT);
			if (fileDescriptor != -1)
			{
				FILE* file = _fdopen(fileDescriptor, "r");
				if (file != NULL)
				{
					int dup2Result = _dup2(_fileno(file), _fileno(stdin));
					if (dup2Result == 0)
					{
						setvbuf(stdin, NULL, _IONBF, 0);
					}
				}
			}
		}
	}

	// Redirect unbuffered stdout to the current standard output handle
	if (bindStdOut)
	{
		HANDLE stdHandle = GetStdHandle(STD_OUTPUT_HANDLE);
		if (stdHandle != INVALID_HANDLE_VALUE)
		{
			int fileDescriptor = _open_osfhandle((intptr_t)stdHandle, _O_TEXT);
			if (fileDescriptor != -1)
			{
				FILE* file = _fdopen(fileDescriptor, "w");
				if (file != NULL)
				{
					int dup2Result = _dup2(_fileno(file), _fileno(stdout));
					if (dup2Result == 0)
					{
						setvbuf(stdout, NULL, _IONBF, 0);
					}
				}
			}
		}
	}

	// Redirect unbuffered stderr to the current standard error handle
	if (bindStdErr)
	{
		HANDLE stdHandle = GetStdHandle(STD_ERROR_HANDLE);
		if (stdHandle != INVALID_HANDLE_VALUE)
		{
			int fileDescriptor = _open_osfhandle((intptr_t)stdHandle, _O_TEXT);
			if (fileDescriptor != -1)
			{
				FILE* file = _fdopen(fileDescriptor, "w");
				if (file != NULL)
				{
					int dup2Result = _dup2(_fileno(file), _fileno(stderr));
					if (dup2Result == 0)
					{
						setvbuf(stderr, NULL, _IONBF, 0);
					}
				}
			}
		}
	}

	// Clear the error state for each of the C++ standard stream objects. We need to do this, as attempts to access the
	// standard streams before they refer to a valid target will cause the iostream objects to enter an error state. In
	// versions of Visual Studio after 2005, this seems to always occur during startup regardless of whether anything
	// has been read from or written to the targets or not.
	if (bindStdIn)
	{
		std::wcin.clear();
		std::cin.clear();
	}
	if (bindStdOut)
	{
		std::wcout.clear();
		std::cout.clear();
	}
	if (bindStdErr)
	{
		std::wcerr.clear();
		std::cerr.clear();
	}
}
#endif