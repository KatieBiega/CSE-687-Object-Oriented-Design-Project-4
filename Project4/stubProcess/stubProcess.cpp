#define WIN32_LEAN_AND_MEAN


#include <Windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <iostream>
#include <algorithm>
#include <ws2def.h>

using std::string;
using std::mbstowcs;
using std::cout;

STARTUPINFO sim;
PROCESS_INFORMATION pim;

STARTUPINFO sir;
PROCESS_INFORMATION pir;

// link to required libraries
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

// define buffer size and port number
#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "12345"

int main()
{
    
    PCSTR localAddress = "127.0.0.1"; //define the server address
    //sockaddr* clientAddress = "127.0.0.1";

    //Clear memory for map process startup pointers
    ZeroMemory(&sim, sizeof(sim));
    sim.cb = sizeof(sim);
    ZeroMemory(&pim, sizeof(pim));

    //Clear memory for reduce process startup pointers
    ZeroMemory(&sir, sizeof(sir));
    sir.cb = sizeof(sir);
    ZeroMemory(&pir, sizeof(pir));

    char bufferChars[DEFAULT_BUFLEN] = {0}; // contents of the buffer (empty by default)
    int bufferLength = DEFAULT_BUFLEN; // maximum length of the buffer

    int action = 0; // determines process to execute; 1 should mean map, 2 should mean reduce, 3 should terminate the stub process, 0 or any oher value should perform no action
    int runCode = 0; // runtime and/or error code for winsock functions

    string commandLineArguments ="\0"; // this should be retrieved from the controller (client) ex. "1needinputdirectoryhere needtempdirectoryhere needoutputdirectoryhere\0"

    int bytesReceived = 0;

    //variables used to convert string to LPWSTR
    wchar_t* wtemp = (wchar_t*)malloc(10);
    size_t commandLength = 0;

    struct addrinfo listener; // address info for the listener socket
    struct addrinfo* result = NULL;

    WSADATA wsaData;


    runCode = WSAStartup(MAKEWORD(2, 2), &wsaData); // start the winsock server, get a return code for the startup function, and terminate process if startup fails
    if (runCode != 0) {
        cout << "Stub process WSAStartup failed with error: " << runCode << "\n";
        return 1;
    }
    else {
        cout << "Winsock server startup successful.\n";
    }

    //set attributes for "listener" address. to be used for StubListener
    ZeroMemory(&listener, sizeof(listener));
    listener.ai_family = AF_INET; // use IPv4
    listener.ai_socktype = SOCK_STREAM; // use a stream socket
    listener.ai_protocol = IPPROTO_TCP; // specify TCP protocol
    listener.ai_flags = AI_PASSIVE;

    //verify that the address parameters are set correctly
    runCode = getaddrinfo(localAddress, DEFAULT_PORT, &listener, &result);
    if (runCode != 0) {
        cout << "Failed to establish address info for stub process server listener- error: " << runCode << "\n";
        WSACleanup();
        return 1;
    }
    else {
        cout << "Server address resolved successfully.\n";
        cout << "Server (local) address: " << localAddress << "\n";
    }

    SOCKET stubListener = INVALID_SOCKET; // this socket waits for a signal to begin receiving data
    stubListener = socket(listener.ai_family, listener.ai_socktype, listener.ai_protocol);

    if (stubListener == INVALID_SOCKET) //check if stubListener socket is still invalid after setting parameters, and close process if so
    {
        cout << "Stub process error at socket(): " << WSAGetLastError() << "\n";
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }
    else {
        cout << "Stub listener socket initialized successfully.\n";
    }


    runCode = bind(stubListener, result->ai_addr, (int)result->ai_addrlen); // attempt to bind the stubListener socket for TCP and end process if failure
    if (runCode == SOCKET_ERROR) {
        cout << "Stub process socket bind failed with error: " << runCode << "\n";
        freeaddrinfo(result);
        closesocket(stubListener);
        WSACleanup();
        return 1;
    }
    else {
        cout << "Stub listener bind successful.\n";
    }

    if (listen(stubListener, SOMAXCONN) == SOCKET_ERROR) //begin listening 
    {
        cout << "Stub server listen failed with error: " << WSAGetLastError() << "\n";
        closesocket(stubListener);
        WSACleanup();
        return 1;
    }
    else {
        cout << "Stub listen function call successful. Waiting for signal from controller...\n";
    }

    SOCKET stubReceiver = INVALID_SOCKET; // this socket accepts input from the controller

    stubReceiver = accept(stubListener, NULL, NULL);    //attempt to accept a client socket
    if (stubReceiver == INVALID_SOCKET) {
        printf("accept failed: %d\n", WSAGetLastError());
        closesocket(stubListener);
        WSACleanup();
        return 1;
    }
    else {
        cout << "Stub accepted connection from controller.\n";
    }


    // keep getting input from controller until connection is broken or the controller forcibly terminates the stub process
    do {
        cout << "Receiving input.\n";
        bytesReceived = recv(stubListener, bufferChars, bufferLength, 0);
        if (bytesReceived > 0) {

            cout << "Bytes received:" << runCode << "\n";

            // need to parse buffer string and convert to arguments and action variable
            action = bufferChars[0] - '0';// first character of the buffer should be the action integer

            commandLineArguments = bufferChars;// set command line arguments as equal to the buffer character array, then remove the first character
            commandLineArguments.substr(1, commandLineArguments.length() - 2);

            if (action == 1) // map process
            {
                commandLineArguments = "\\mapProcess.exe " + commandLineArguments;
                wtemp = (wchar_t*)malloc(4 * commandLineArguments.size());
                mbstowcs(wtemp, commandLineArguments.c_str(), commandLength); //includes null
                LPWSTR args = wtemp;

                cout << "Attempting to create map process...\n";
                // Start the child map process. 
                if (!CreateProcess(
                    NULL,           // module name (not used here; command line only)
                    args,           // Command line
                    NULL,           // Process handle not inheritable
                    NULL,           // Thread handle not inheritable
                    FALSE,          // Set handle inheritance to FALSE
                    0,              // No creation flags
                    NULL,           // Use parent's environment block
                    NULL,           // Use parent's starting directory 
                    &sim,           // Pointer to STARTUPINFO structure
                    &pim)           // Pointer to PROCESS_INFORMATION structure
                    )
                {
                    cout << "CreateProcess for mapper failed" << GetLastError() << "\n";

                }
                else {
                    cout << "Map process was created successfully; waiting for process to complete.\n";
                    WaitForSingleObject(pim.hProcess, INFINITE);
                }
                free(wtemp); // free memory of wtemp
                CloseHandle(pim.hProcess);
                CloseHandle(pim.hThread);
            }

            else if (action == 2) // reduce process
            {
                commandLineArguments = "\\reduceProcess.exe " + commandLineArguments;
                wtemp = (wchar_t*)malloc(4 * commandLineArguments.size());
                mbstowcs(wtemp, commandLineArguments.c_str(), commandLength); //includes null
                LPWSTR args = wtemp;

                cout << "Attempting to create reduce process...\n";
                // Start the child reduce process. 
                if (!CreateProcess(
                    NULL, // module name (not used here; command line only)
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
                    cout << "Reduce process was created successfully; waiting for process to complete.\n";
                    WaitForSingleObject(pir.hProcess, INFINITE);
                }
                free(wtemp); // free memory of wtemp
                CloseHandle(pir.hProcess);
                CloseHandle(pir.hThread);
            }

            else if (action == 3) // forced shutdown of stub process by controller
            {
                cout << "Exiting process due to client command.\n";
                exit(0);
            }

            else
            {
                //do nothing
            }
        }
        else if (bytesReceived == 0)
            cout << "Closing the connection...";
        else {
            cout << "Failed to receive data. Error: " << WSAGetLastError() << "\n",
            closesocket(stubReceiver);
            WSACleanup();
            return 1;
        }

    } while (runCode > 0);


}