#include "main.hpp"

HANDLE CreatePipe()
{
	std::cout << "Creating an instance of a named pipe..." << std::endl;

	return CreateNamedPipe(
		L"\\\\.\\pipe\\scriptpipe", // name of the pipe
		PIPE_ACCESS_OUTBOUND, // 1-way pipe -- send only
		PIPE_TYPE_BYTE, // send data as a byte stream
		1, // only allow 1 instance of this pipe
		0, // no outbound buffer
		0, // no inbound buffer
		0, // use default wait time
		nullptr // use default security attributes
	);
}

void WaitForClient(HANDLE pipe)
{
	std::cout << "Waiting for a client to connect to the pipe..." << std::endl;

	const bool result = ConnectNamedPipe(pipe, nullptr);
	if (!result) {
		std::cout << "Failed to make connection on named pipe." << std::endl;
		// look up error code here using GetLastError()
		CloseHandle(pipe); // close the pipe
		system("pause");
	}
	std::cout << "Connected to client. Awaiting input..." << std::endl;
}

bool SendData(HANDLE pipe, const char* message)
{
	std::cout << "Sending data to pipe..." << std::endl;

	// This call blocks until a client process reads all the data
	const auto* data = message;

	DWORD numBytesWritten = 0;

	const auto result = WriteFile(
		pipe, // handle to our outbound pipe
		data, // data to send
		strlen(data) * sizeof(char*), // length of data to send (bytes)
		&numBytesWritten, // will store actual amount of data sent
		nullptr // not using overlapped IO
	);

	if (result)
		std::cout << "Number of bytes sent: " << numBytesWritten << std::endl;
	else
		std::cout << "Failed to send data." << std::endl;

	return result;
}

void GetInput(HANDLE pipe)
{
	std::string input;
	do
	{
		std::getline(std::cin, input);
		if (!input.empty() && input != "exit")
			SendData(pipe, input.c_str());

	} while (input != "exit");
}

int _main()
{
	auto* const pipe = CreatePipe();

	WaitForClient(pipe);

	GetInput(pipe);

	CloseHandle(pipe);

	return std::getchar();
}