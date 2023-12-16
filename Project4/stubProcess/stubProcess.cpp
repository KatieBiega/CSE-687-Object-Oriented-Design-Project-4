#define WIN32_LEAN_AND_MEAN


#include <Windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <iostream>
#include <algorithm>

using std::string;
using std::mbstowcs;
using std::cout;


//memory allocation for map process startup
STARTUPINFO sim;
PROCESS_INFORMATION pim;

ZeroMemory(&sim, sizeof(sim));
sim.cb = sizeof(sim);
ZeroMemory(&pim, sizeof(pim));

//memory allocation for reduce process startup
STARTUPINFO sir;
PROCESS_INFORMATION pir;

ZeroMemory(&sir, sizeof(sir));
sir.cb = sizeof(sir);
ZeroMemory(&pir, sizeof(pir));


// link to required libraries
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "909"

int main()
{
	int action = 0; // determines process to execute; 1 should mean map, 2 should mean reduce, 3 should terminate the stub process, 0 or any oher value should perform no action
    int runCode = 0; // runtime and/or error code for winstock functions

    string commandLineArguments = "needexehere needinputdirectoryhere needtempdirectoryhere needoutputdirectoryhere\0"; // this should be retrieved from the controller (client)

    //variables used to convert string to LPWSTR
    wchar_t* wtemp = (wchar_t*)malloc(10);
    size_t commandLength = 0;

	WSADATA wsaData;

    SOCKET stubListener = INVALID_SOCKET; // this socket waits
    stubListener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	SOCKET stubReceiver = INVALID_SOCKET; // stub server only needs to receive integer (to select child process to create) and string from controller (for process arguments)
    stubReceiver = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);


    
    runCode = WSAStartup(MAKEWORD(2, 2), &wsaData); // start the winstock server, get a return code for the startup function, and terminate process if startup fails
    if (runCode != 0) {
        cout << "WSAStartup failed with error: " << runCode << "\n";
        return 1;
    }


    if (action == 1) // map process
    {

        wtemp = (wchar_t*)malloc(4 * commandLineArguments.size());
        mbstowcs(wtemp, commandLineArguments.c_str(), commandLength); //includes null
        LPWSTR args = wtemp;

        // Start the child map process. 
        if (!CreateProcess(
            NULL, // module name
            args,        // Command line
            NULL,           // Process handle not inheritable
            NULL,           // Thread handle not inheritable
            FALSE,          // Set handle inheritance to FALSE
            0,              // No creation flags
            NULL,           // Use parent's environment block
            NULL,           // Use parent's starting directory 
            &sim,            // Pointer to STARTUPINFO structure
            &pim)           // Pointer to PROCESS_INFORMATION structure
            )
        {
            cout << "CreateProcess for mapper failed" << GetLastError() << "\n";

        }
        else {
            cout << "Map process was created successfully...\n";
            WaitForSingleObject(pim.hProcess, INFINITE);
        }
        free(wtemp); // free memory of wtemp
        CloseHandle(pim.hProcess);
        CloseHandle(pim.hThread);
    }

    else if (action == 2) // reduce process
    {
        wtemp = (wchar_t*)malloc(4 * commandLineArguments.size());
        mbstowcs(wtemp, commandLineArguments.c_str(), commandLength); //includes null
        LPWSTR args = wtemp;

        // Start the child reduce process. 
        if (!CreateProcess(
            NULL, // module name
            args,        // Command line
            NULL,           // Process handle not inheritable
            NULL,           // Thread handle not inheritable
            FALSE,          // Set handle inheritance to FALSE
            0,              // No creation flags
            NULL,           // Use parent's environment block
            NULL,           // Use parent's starting directory 
            &sir,            // Pointer to STARTUPINFO structure
            &pir)           // Pointer to PROCESS_INFORMATION structure
            )
        {
            cout << "CreateProcess for reducer failed" << GetLastError() << "\n";

        }
        else {
            cout << "Reduce process was created successfully...\n";
            WaitForSingleObject(pir.hProcess, INFINITE);
        }
        free(wtemp); // free memory of wtemp
        CloseHandle(pir.hProcess);
        CloseHandle(pir.hThread);
    }

    else if (action == 3) 
    {
        exit(0); // terminate the stub process
    }

    else
    {
        //do nothing
    }


}