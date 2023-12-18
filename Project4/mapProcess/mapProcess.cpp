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
#include <synchapi.h>

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

//Data structure for thread creation
struct dstruct {
    string inputDirectory = "\0";
    string tempDirectory = "\0";
    string outputDirectory = "\0";
};


DWORD WINAPI Mapper(LPVOID lpParam)
{
    int longDelay = 2000;
    string fileString = "\0";
    string tempFilename = "TempFile.txt";
    string mapped_string;

    cout << "Attempting to cast lpParam to tempDirectories structure...\n";
    dstruct* tempDirectories = (dstruct*)lpParam;

    cout << tempDirectories->inputDirectory << "\n";
    cout << tempDirectories->tempDirectory << "\n";
    cout << tempDirectories->outputDirectory << "\n";

    FileManagement FileManage(tempDirectories->inputDirectory, tempDirectories->outputDirectory, tempDirectories->tempDirectory); //Create file management class based on the user inputs
    cout << "FileManagement Class initialized.\n";

    Sleep(longDelay);// pause for 2 seconds

    HMODULE mapDLL = LoadLibraryA("MapDLL.dll"); // load dll for map functions
    if (mapDLL == NULL) // exit main function if mapDLL is not found
    {
        cout << "Failed to load mapDLL." << endl;
        return 1;
    }

    CREATE_MAPPER mapperPtr = (CREATE_MAPPER)GetProcAddress(mapDLL, "CreateMap"); // create pointer to function to create new Map object
    MapInterface* pMapper = mapperPtr();

    Sleep(longDelay);// pause for 2 seconds

    fileString = FileManage.ReadAllFiles();     //Read single file into single string
    //cout << "Single file read.\n";

    Sleep(longDelay);// pause for 2 seconds

    //cout << "Strings from files passed to map function.\n";
    pMapper->map(fileString);

    Sleep(longDelay);// pause for 2 seconds

    //cout << "Mapping complete; exporting resulting string.\n";
    mapped_string = pMapper->vector_export();     //Write mapped output string to intermediate file 

    Sleep(longDelay);// pause for 2 seconds

    FileManage.WriteToTempFile(tempFilename, mapped_string);

    Sleep(longDelay);// pause for 2 seconds

    FreeLibrary(mapDLL);

    mapper_end_flag = 1;

    return 0;
}

int main(int argc, char** argv)
{
    cout << "mapProcess Test 1" << "\n";
    // Directory inputs bieng passed by Stub process
    string inputDirectory = "../../../io_files/input_directory";
    string tempDirectory = "../../../io_files/temp_directory";
    string outputDirectory = "../../../io_files/output_directory";

    if (argc < 4) // this process should have 4 arguments: executable name [0] and 3 char arrays for directories
    {
        cout << "One or more arguments for file directories missing. Using default directory paths." << "\n";

    }
    else
    {
        // Directory inputs bieng passed by Stub process
        inputDirectory = argv[1];
        tempDirectory = argv[2];
        outputDirectory = argv[3];
        outputDirectory += "\0"; // need null terminator at end of final argument
    }


    int iResult = 0;

    // Time variables
    time_t seconds;
    seconds = time(NULL);
    int delay = 1000;


    /*********** START THREAD WORKFLOW ***********/
        //DWORD   dwThreadId; //not needed if final parameter used in thread creation is is NULL?
    HANDLE  hThread;

    dstruct* data = new dstruct();

    data->inputDirectory = inputDirectory;
    data->tempDirectory = tempDirectory;
    data->outputDirectory = outputDirectory;

    //cout << data->inputDirectory << "\n";
    //cout << data->tempDirectory << "\n";
    //cout << data->outputDirectory << "\n";

    if (data == NULL) // end process if structure object is not created
    {
        return(1);
    }

    cout << "Test 2" << "\n";

    // Create the thread to begin execution on its own.
    hThread = CreateThread(NULL, 0, Mapper, (LPVOID*)data, 0, NULL);

    if (hThread == NULL)
    {
        std::cerr << "Failed to create thread." << "\n";
        return 1;
    }
    else {
        cout << "mapProcess.exe thread created successfully.\n";
    }

    // End of main thread creation loop.

    cout << "Test 3" << "\n";

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
    while (1)
    {
        if (mapper_end_flag == 0) // mapper thread function incomplete
        {
            // Send a regular heartbeat
            iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
            if (iResult == SOCKET_ERROR) {
                printf("mapProcess heartbeat send failed with error: %d\n", WSAGetLastError());
                closesocket(ConnectSocket);
                WSACleanup();
                return 1;
            }
        }
        else if (mapper_end_flag == 1) //mapper thread function complete
        {
            sendbuf = "mapProcess has completed.";
            // Send a completion message
            iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
            if (iResult == SOCKET_ERROR) {
                printf("mapProcess completion signal failed with error: %d\n", WSAGetLastError());
                closesocket(ConnectSocket);
                WSACleanup();
                return 1;
            }
            // Wait until thread has been terminated.
            WaitForSingleObject(hThread, INFINITE);

            // Close thread handle and free memory allocations.
            CloseHandle(hThread);
            break; // exit while loop
        }
        else //this shouldn't happen unless the code has one or more logic errors
        {
            cout << "ERROR: mapper_end_flag set to invalid value: " << mapper_end_flag << "\n";
            break;
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

    // cleanup
    closesocket(ConnectSocket);
    WSACleanup();
    
     /*********** END SOCKET WORKFLOW ***********/

    return 0;

}
