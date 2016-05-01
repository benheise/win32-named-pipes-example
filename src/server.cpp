
#include <iostream>
#include <windows.h>
#include <thread>
#include <time.h>

// Use std namespace for ease
using namespace std;

/*
Setup global poison pill, after
much thought about how to properly handle
start, stop, exit, restart. I found that a
pill would be a great way to handle all of
these methods and properly exit a thread once
this variable has been set. Pill will follow:

0 = stop - dont write / output data (pause)
1 = star - starts the cappture / continue
2 = restart - restart and rebuild files
3 = exit - set poison pill / kill thread main will exit
*/
int pill = 0;
extern int pill;


// A function to build the pipe
int buildPipe() {
	HANDLE pipe = CreateNamedPipe(
		L"\\\\.\\pipe\\my_pipe",
		PIPE_ACCESS_DUPLEX, // client to server - server to client
		PIPE_TYPE_MESSAGE, // byte stream, do we need others?
		1, // nMaxInstances - 1
		0, // nOutBufferSize - none
		0, // nInBufferSize  - none
		0, // nDefaultTimeOut - 50 milliseconds. 
		NULL // security attributes - def
		);

	if (pipe == NULL || pipe == INVALID_HANDLE_VALUE) {
		DWORD lastError = GetLastError();
		wcout << "[!] Failed to create server pipe: " << lastError << endl;
		return 1;
	}

	cout << "[*] Pipe server waiting for command..." << endl;

	// blocking call till client connects / sends data
	BOOL message = ConnectNamedPipe(
		pipe, // the handle to the pipe
		NULL // lpOverlapped - not FILE_FLAG_OVERLAPPED
		);
	if (!message) {
		DWORD lastError = GetLastError();
		wcout << "[!] Failed to create connection to named pipe: " << lastError << endl;
		CloseHandle(pipe);
		return 1;
	}
	// build a while loop with blocking logic
	while (1) {
		// send message to client and block till read by client
		const wchar_t *data = L"---- Winscok Capture waiting for command ----\n";
		DWORD numBytesWritten = 0;
		BOOL write = WriteFile(
			pipe,
			data, // bytes to send
			wcslen(data) * sizeof(wchar_t), // byte size
			&numBytesWritten,
			NULL
			);
		if (write) {
			wcout << "[*] Server sent bytes: " << numBytesWritten << endl;
		}
		else {
			DWORD lastError = GetLastError();
			wcout << "[!] Failed to send data: " << lastError << endl;
			// break to the top of the loop
			continue;
		}
		// now block for data from the pipe
		wchar_t buffer[128];
		DWORD numBytesRead = 0;
		BOOL fSuccess = ReadFile(
			pipe,        // handle to pipe 
			buffer,    // buffer to receive data 
			127 * sizeof(wchar_t), // size of buffer 
			&numBytesRead, // number of bytes read 
			NULL);        // not overlapped I/O 

						  // create pill logic
		if (fSuccess) {
			//buffer[numBytesRead / sizeof(wchar_t)] = '\0'; // null-tem string
			wcout << "[*] Server recived bytes: " << numBytesRead << endl;
			for (int i = 0; i < numBytesRead; i++) {
				cout << buffer[i] << endl;
			}
			wcout << "[*] Server message recived: " << buffer << endl;
			if (buffer) {
				if (buffer[0] = '0') {
					//0 = stop - dont write / output data(pause)
					wcout << "[!] Server recived stop message - now pausing" << endl;
					pill = 0;
				}
				if (buffer[0] = '1') {
					//1 = star - starts the cappture / continue
					wcout << "[!] Server start message - now continuing" << endl;
					pill = 1;
				}
				if (buffer[0] = '2') {
					//2 = restart - restart and rebuild files
					wcout << "[!] Server restart message - now restarting" << endl;
					pill = 2;
				}
				if (buffer[0] = '3') {
					//3 = exit - set poison pill / kill thread main will exit
					wcout << "[!] Server exit message - now exiting" << endl;
					pill = 3;
					break;
				}
				else {
					wcout << "[!] Server recived invalid request!" << endl;
				}
			}
		}
		else {
			DWORD lastError = GetLastError();
			wcout << "[!] Failed to read data from client: " << lastError << endl;
		}

	} // end of while loop
	try {
		cout << "[!] Pipe server shuting down" << endl;
		CloseHandle(pipe);
	}
	catch (int e) {
		cout << "[*] Pipe server already flushed: " << e << endl;
	}
	// In C++ code, you should return from your thread function.
	// ExitThread, bypasses garbage clean up
	return 0;

}

// A threaded function for the async pipe
bool threadedPipe() {
	try {
		cout << "[*] Starting pipe server! " << endl;
		std::thread first(buildPipe);     // spawn new thread that calls function
		cout << "[*] Pipe thread started! " << endl;
		first.detach();
		return true;
	}
	catch (int e) {
		cout << "[*] Failed to start pipe server: " << e << endl;
		return false;
	}
}

int main() {
	bool t = threadedPipe();
	while (1) {
		Sleep(1 * 1000);
	}
}