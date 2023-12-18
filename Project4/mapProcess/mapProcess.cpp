#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <vector>
#include <Windows.h>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <tchar.h>
#include <strsafe.h>
#include <thread>

#include "../MapDLL/MapInterface.h"
#include "File Management.h"

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "11112"
#define SERVER_ADDRESS "127.0.0.1"
#define BUF_SIZE 255

int mapper_end_flag = 0;

using std::stringstream;
using std::replace;
using std::thread;
using std::cout;
using std::cin;
using std::endl;
using std::thread;

typedef MapInterface* (*CREATE_MAPPER) ();

// Sample custom data structure for threads to use.
// This is passed by void pointer so it can be any data type
// that can be passed using a single void pointer (LPVOID).
typedef struct MyData {
    string inputDirectory;
    string tempDirectory;
    string outputDirectory;
} MYDATA, * PMYDATA;


DWORD WINAPI Mapper(LPVOID lpParam)
{
    string fileName = "";  // Temporary
    string fileString = "";  // Temporary
    string inputDirectory = "";  // Temporary
    string outputDirectory = "";  // Temporary
    string tempDirectory = "";  // Temporary

    string mapped_string;

    FileManagement FileManage(inputDirectory, outputDirectory, tempDirectory); //Create file management class based on the user inputs
    cout << "FileManagement Class initialized.\n";

    HMODULE mapDLL = LoadLibraryA("MapDLL.dll"); // load dll for map functions
    if (mapDLL == NULL) // exit main function if mapDLL is not found
    {
        cout << "Failed to load mapDLL." << endl;
        return 1;
    }

    CREATE_MAPPER mapperPtr = (CREATE_MAPPER)GetProcAddress(mapDLL, "CreateMap"); // create pointer to function to create new Map object
    MapInterface* pMapper = mapperPtr();

    fileString = FileManage.ReadAllFiles();     //Read single file into single string
    //cout << "Single file read.\n";

    //cout << "Strings from files passed to map function.\n";
    pMapper->map(fileString);

    //cout << "Mapping complete; exporting resulting string.\n";
    mapped_string = pMapper->vector_export();     //Write mapped output string to intermediate file 


    FileManage.WriteToTempFile(outputDirectory, mapped_string);
    cout << "String from mapping written to temp file.\n";

    return 0;

    system("pause");

    FreeLibrary(mapDLL);

    mapper_end_flag = 1;

    return 0;
}

int main(int argc, char** argv)
{
    //Directory inputs bieng passed by Stub process
    cout << "mapProcess Test 1" << "\n";
    // Directory inputs bieng passed by Stub process
    string inputDirectory = "\0";
    string tempDirectory = "\0";
    string outputDirectory = "\0";

    if (argc = 3, argc >3)
    {
        // Directory inputs bieng passed by Stub process
       inputDirectory = argv[0];
       tempDirectory = argv[1];
       outputDirectory = argv[2];
    }
    else
    {
        cout << "One or more arguments for file directories missing. Using default values." << "\n";
        string inputDirectory = "0";
        string tempDirectory = "1";
        string outputDirectory = "2";
    }

    MyData data; //(MyData*) malloc(sizeof(MyData));
    data.inputDirectory = inputDirectory;
    data.tempDirectory = tempDirectory;
    data.outputDirectory = outputDirectory;

    cout << data.inputDirectory;
    cout << data.tempDirectory;
    cout << data.outputDirectory;

    // Time variables
    time_t seconds;
    seconds = time(NULL);
    int delay = 1000;


 /*********** START THREAD WORKFLOW ***********/
    PMYDATA  pData = &data;
    //DWORD   dwThreadId; //not needed if final parameter used in thread creation is is NULL?
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

    // Create the thread to begin execution on its own.
    hThread = CreateThread(NULL, 0, Mapper, pData, 0, NULL);

    if (hThread == NULL)
    {
        std::cerr << "Failed to create thread." << endl;
        return 1;
    }

    // End of main thread creation loop.

    // Wait until all threads have terminated.
    //WaitForMultipleObjects(1, hThread, TRUE, INFINITE);

    // Close all thread handles and free memory allocations.
    //CloseHandle(hThread);

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
    const char* sendbuf = "Map Process is running."; // Message to be sent to controller
    char recvbuf[DEFAULT_BUFLEN];
    int iResult;
    int recvbuflen = DEFAULT_BUFLEN;

    // Validate  parameters
    if (argc != 2) {
        printf("usage: %s server-name\n", argv[0]);
        return 1;
    }

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("Map WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("Map getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Attempt to connect to an address until one succeeds
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
            ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            printf("Map socket failed with error: %ld\n", WSAGetLastError());
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
        printf("Map Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }


    //reccurring heartbeat message to server (controller) every k = 1 second 
    while (mapper_end_flag != 0)
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

    //Map thread ended and joined back in main thread
  

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
