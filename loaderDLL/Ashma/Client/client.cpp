#include "client.hpp"


HANDLE _CreatePipe()
{
	std::cout << "Connecting to pipe..." << std::endl;

	// Open the named pipe
	// Most of these parameters aren't very relevant for pipes.
	return CreateFile(
		L"\\\\.\\pipe\\scriptpipe",
		GENERIC_READ, // only need read access
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		nullptr
	);
}

const char* ReadData(HANDLE pipe)
{
	std::cout << "Reading data from pipe..." << std::endl;

	// The read operation will block until there is data to read
	char buffer[99999];
	std::string Script;
	DWORD numBytesRead = 0;

	const auto result = ReadFile(
		pipe,
		buffer, // the data from the pipe will be put here
		127 * sizeof(char*), // number of bytes allocated
		&numBytesRead, // this will store number of bytes actually read
		nullptr // not using overlapped IO
	);

	if (result) {
		buffer[numBytesRead / sizeof(char*)] = '\0'; // null terminate the string
		std::cout << "Number of bytes read: " << numBytesRead << std::endl;
		std::cout << "Message: " << buffer << std::endl;
		Script.append(buffer);
		return buffer;
	}
	else {
		std::cout << "Communication terminated." << std::endl;
	}
	return "=+Failed+=";
}

void ReadInput(void* const pipe)
{
	while (true)
	{
		const auto* message = ReadData(pipe);
		if (strcmp(message, "=+Failed+=") == 0)
			break;
	}
}

int main_pipe()
{
	auto* const pipe = _CreatePipe();

	ReadInput(pipe);

	CloseHandle(pipe);

	return 0;
}