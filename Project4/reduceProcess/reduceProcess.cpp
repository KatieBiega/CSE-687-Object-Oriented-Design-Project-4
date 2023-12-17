#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <vector>
#include <algorithm>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <tchar.h>
#include <strsafe.h>
#include <thread>

#include "File Management.h"
#include "../ReduceDLL/ReduceInterface.h"


// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "11112"
#define SERVER_ADDRESS "127.0.0.1"
#define BUF_SIZE 255

int reducer_end_flag = 0;

using std::stringstream;
using std::vector;
using std::string;
using std::wstring;
using std::to_string;
using std::getline;
using std::cout;
using std::cin;
using std::endl;
using std::thread;

DWORD WINAPI Reducer(LPVOID lpParam);

typedef ReduceInterface* (*CREATE_REDUCER) ();





// Sample custom data structure for threads to use.
// This is passed by void pointer so it can be any data type
// that can be passed using a single void pointer (LPVOID).
typedef struct MyData {
    string inputDirectory;
    string tempDirectory;
    string outputDirectory;
} MYDATA, * PMYDATA;

int main(int argc, char **argv)
{
    cout << "Test 1" << endl;
    // Directory inputs bieng passed by Stub process
    string inputDirectory = "0";
    string tempDirectory = "1";
    string outputDirectory = "2";

    if (argc = 3, argc >3)
    {
        // Directory inputs bieng passed by Stub process
       inputDirectory = argv[0];
       tempDirectory = argv[1];
       outputDirectory = argv[2];
    }
    else
    {
 
    }
  

    MyData data; //(MyData*) malloc(sizeof(MyData));
    data.inputDirectory = inputDirectory;
    data.tempDirectory = tempDirectory;
    data.outputDirectory = outputDirectory;

    cout << data.inputDirectory;
    cout << data.tempDirectory;
    cout << data.outputDirectory;
 
    system("pause");

    // Time variables
    time_t seconds;
    seconds = time(NULL);
    int delay = 1000;


    /*********** START THREAD WORKFLOW ***********/
    PMYDATA  pData;
    DWORD   dwThreadId;
    HANDLE  hThread;
    
    // Allocate memory for thread data.
    pData = (PMYDATA) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MYDATA));

    if (pData == NULL)
    {
        // If the array allocation fails, the system is out of memory
        // so there is no point in trying to print an error message.
        // Just terminate execution.
        ExitProcess(2);
    }

    cout << "Test 2" << endl;
    pData->inputDirectory = inputDirectory;
    pData->tempDirectory = tempDirectory;
    pData->outputDirectory = outputDirectory;

    // Create the thread to begin execution on its own.
    hThread = CreateThread(NULL, 0, Reducer, pData, 0, NULL);

    if (hThread == NULL)
    {
        std::cerr << "Failed to create thread." << endl;
        return 1;
    }

    // End of main thread creation loop.

    // Wait until all threads have terminated.
    //WaitForMultipleObjects(1, hThread, TRUE, INFINITE);

    // Close all thread handles and free memory allocations.
    CloseHandle(hThread);

    if(pData != NULL)
    {
        HeapFree(GetProcessHeap(), 0, pData);
        pData = NULL;    // Ensure address is not reused.
    }

    /*********** END THREAD WORKFLOW ***********/

    
    /*********** START SOCKET WORKFLOW ***********/

    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo* result = NULL,
        * ptr = NULL,
        hints;
    const char* sendbuf = "Reducer Process is running."; // Message to be sent to controller
    char recvbuf[DEFAULT_BUFLEN];
    int iResult;
    int recvbuflen = DEFAULT_BUFLEN;

     // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo(SERVER_ADDRESS, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }
    cout << "Test 3" << endl;
    // Attempt to connect to an address until one succeeds
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
            ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        // Connect to server.
        iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }

    // Reduce workflow for all FileMangement and ReduceDLL function calls in seperate thread running in parallel and sharing memory
    // Non-blocking so Main thread will continue to execute  
    //thread task(Reducer(inputDirectory, tempDirectory, outputDirectory));

    //reccurring heartbeat message to server (controller) every k = 1 second 
    while (reducer_end_flag != 0)
    {
        // Send an initial buffer
        iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
        if (iResult == SOCKET_ERROR) {
            printf("send failed with error: %d\n", WSAGetLastError());
            closesocket(ConnectSocket);
            WSACleanup();
            return 1;
        }
        
        Sleep(delay);
    }

    printf("Bytes Sent: %ld\n", iResult);

    // Reducer thread ended and joined back in main thread
    //task.join();

        // shutdown the connection since no more data will be sent
    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    // Receive until the peer closes the connection
    do {

        iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0)
            printf("Bytes received: %d\n", iResult);
        else if (iResult == 0)
            printf("Connection closed\n");
        else
            printf("recv failed with error: %d\n", WSAGetLastError());

    } while (iResult > 0);

    // cleanup
    closesocket(ConnectSocket);
    WSACleanup();

    /*********** END SOCKET WORKFLOW ***********/

    return 0;

}

DWORD WINAPI Reducer(LPVOID lpParam)
{
    string fileName = "";  // Temporary
    string fileString = "";  // Temporary
    string reduced_string;

    MyData* tempData = (MyData*)lpParam;

    MyData data = *tempData;

    cout << data.inputDirectory;
    cout << data.tempDirectory;
    cout << data.outputDirectory;

    FileManagement FileManage(data.inputDirectory, data.outputDirectory, data.tempDirectory); //Create file management class based on the user inputs
    cout << "FileManagement Class initialized.\n";

    HMODULE reduceDLL = LoadLibraryA("ReduceDLL.dll"); // load dll for library functions
    if (reduceDLL == NULL) // exit main function if reduceDLL is not found
    {
        cout << "Failed to load reduceDLL." << endl;
        return 1;
    }

    CREATE_REDUCER reducerPtr = (CREATE_REDUCER)GetProcAddress(reduceDLL, "CreateReduce");  // create pointer to function to create new Reduce object
    ReduceInterface* pReducer = reducerPtr();

    //Read from intermediate file and pass data to Reduce class
    fileString = FileManage.ReadSingleFile(data.tempDirectory);     //Read single file into single string
    //cout << "Single file read.\n";

    pReducer->import(fileString);
    //cout << "String imported by reduce class function and placed in vector.\n";

    pReducer->sort();
    //cout << "Vector sorted.\n";

    pReducer->aggregate();
    //cout << "Vector aggregated.\n";

    pReducer->reduce();
    //cout << "Vector reduced.\n";

    reduced_string = pReducer->vector_export();
    //cout << "Vector exported to string.\n";

    //Sorted, aggregated, and reduced output string is written into final output file
    FileManage.WriteToOutputFile(data.outputDirectory, reduced_string);
    cout << "Reduced string written to output file.\n";

    system("pause");

    FreeLibrary(reduceDLL);

    reducer_end_flag = 1;

    return 0;
}
