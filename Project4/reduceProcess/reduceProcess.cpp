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
#include <chrono>
#include <ctime>
#include <synchapi.h>

#include "File Management.h"
#include "../ReduceDLL/ReduceInterface.h"


// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "11113"
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


typedef ReduceInterface* (*CREATE_REDUCER) ();

//Data structure for thread creation
struct dstruct {
    string inputDirectory = "\0";
    string tempDirectory = "\0";
    string outputDirectory = "\0";
};


DWORD WINAPI Reducer(LPVOID lpParam)
{
    int longDelay = 2000;
    string reduced_string;
    string tempFilename = "TempFile.txt";
    string outputFilename = "FinalOutput.txt";
    string tempFileContent = "\0";
    string successFilename = "SUCCESS.txt";
    string successString = "\0";

    cout << "Attempting to cast lpParam to tempDirectories structure...\n";
    dstruct* tempDirectories = (dstruct*)lpParam;

    cout << tempDirectories->inputDirectory << "\n";
    cout << tempDirectories->tempDirectory << "\n";
    cout << tempDirectories->outputDirectory << "\n";

    FileManagement FileManage(tempDirectories->inputDirectory, tempDirectories->outputDirectory, tempDirectories->tempDirectory); //Create file management class based on the user inputs
    cout << "FileManagement Class initialized.\n";

    HMODULE reduceDLL = LoadLibraryA("ReduceDLL.dll"); // load dll for library functions
    if (reduceDLL == NULL) // exit main function if reduceDLL is not found
    {
        cout << "Failed to load reduceDLL." << endl;
        return 1;
    }

    CREATE_REDUCER reducerPtr = (CREATE_REDUCER)GetProcAddress(reduceDLL, "CreateReduce");  // create pointer to function to create new Reduce object
    ReduceInterface* pReducer = reducerPtr();

    tempFileContent = FileManage.ReadFromTempFile(tempFilename);

    if (tempFileContent == "\0") {
        cout << "Temp file content at " + tempDirectories->tempDirectory + "/" + tempFilename + " empty.\n";
        return 1;
    }
    else {
        //cout << "New string read from temp file.\n";
    }

    Sleep(longDelay);// pause for 2 seconds

    pReducer->import(tempFileContent);
    //cout << "String imported by reduce class function and placed in vector.\n";

    Sleep(longDelay);// pause for 2 seconds

    pReducer->sort();
    //cout << "Vector sorted.\n";

    Sleep(longDelay);// pause for 2 seconds

    pReducer->aggregate();
    //cout << "Vector aggregated.\n";

    Sleep(longDelay);// pause for 2 seconds

    pReducer->reduce();
    //cout << "Vector reduced.\n";

    Sleep(longDelay);// pause for 2 seconds

    reduced_string = pReducer->vector_export();
    //cout << "Vector exported to string.\n";

    Sleep(longDelay);// pause for 2 seconds

    //Sorted, aggregated, and reduced output string is written into final output file
    FileManage.WriteToOutputFile(outputFilename, reduced_string);
    //cout << "string written to output file.\n";

    FileManage.WriteToOutputFile(successFilename, successString);

    FreeLibrary(reduceDLL);

    delete tempDirectories;

    reducer_end_flag = 1;

    return 0;
}


int main(int argc, char *argv[])
{
    cout << "reduceProcess Test 1" << "\n";
    // Directory inputs being passed by Stub process
    string inputDirectory = "\0";
    string tempDirectory = "\0";
    string outputDirectory = "\0";

    if (argc < 4) // this process should have 4 arguments: executable name argv[0] and 3 char arrays for directories
    {
        cout << "One or more arguments for file directories missing. Using default directory paths." << "\n";
        inputDirectory = "../../../io_files/input_directory";
        tempDirectory = "../../../io_files/temp_directory";
        outputDirectory = "../../../io_files/output_directory";
    }
    else
    {
        // Directory inputs bieng passed by Stub process
        inputDirectory = argv[1];
        tempDirectory = argv[2];
        outputDirectory = argv[3];
    }
  

    int iResult = 0;

    // Time variables
    time_t seconds;
    seconds = time(NULL);
    int delay = 1000;


    /*********** START THREAD WORKFLOW ***********/
        //DWORD   dwThreadId; //not needed if final parameter used in thread creation is is NULL?
    HANDLE  hThread;

    dstruct *data = new dstruct();

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
    hThread = CreateThread(NULL, 0, Reducer, (LPVOID * )data, 0, NULL);

    if (hThread == NULL)
    {
        std::cerr << "Failed to create thread." << "\n";
        return 1;
    }
    else {
        cout << "reduceProcess.exe thread created successfully.\n";
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
    const char* sendbuf = "Reducer Process is running."; // Message to be sent to controller

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
    while (1)
    {
        if (reducer_end_flag == 0) // mapper thread function incomplete
        {
            // Send a regular heartbeat
            iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
            if (iResult == SOCKET_ERROR) {
                printf("reduceProcess heartbeat send failed with error: %d\n", WSAGetLastError());
                closesocket(ConnectSocket);
                WSACleanup();
                return 1;
            }
            
        }
        else if (reducer_end_flag == 1) //mapper thread function complete
        {
            sendbuf = "reduceProcess has completed.";
            // Send a completion message
            iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
            if (iResult == SOCKET_ERROR) {
                printf("reduceProcess completion signal failed with error: %d\n", WSAGetLastError());
                closesocket(ConnectSocket);
                WSACleanup();
                return 1;
            }
            // Wait until all threads have terminated.
            WaitForSingleObject(hThread, INFINITE);

            // Close all thread handles and free memory allocations.
            CloseHandle(hThread);
            break; // exit while loop
        }
        else //this shouldn't happen unless the code has one or more logic errors
        {
            cout << "ERROR: reducer_end_flag set to invalid value: " << reducer_end_flag << "\n";
            break;
        }
        Sleep(delay);
    }

    printf("Bytes Sent: %ld\n", iResult);

    // Reducer thread ended and joined back in main thread

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
