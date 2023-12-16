/*
* CSE687 Project 1
*
* The map class will contain a public method map(), that accepts a key and value. 
* The key will be the file name and the value will be a single line of raw data from the file. 
* The map() function will tokenize the value into distinct words (remove punctuation, whitespace and capitalization). 
* The map() function will call a second function export() that takes a key and value as two parameters. 
* The export function will buffer output in memory and periodically write the data out to disk (periodicity can be based on the size of the buffer). 
* The intermediate data will be written to the temporary directory (specified via command line). 
* 
*/
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "pch.h"
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <vector>
#include "MapDLL.h"
#include <Windows.h>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

using std::stringstream;
using std::replace;
using std::thread;

int Map();

typedef MapInterface* (*CREATE_MAP) ();

int main(int argc, char** argv)
{
    // Time variables
    time_t seconds;
    seconds = time(NULL);
    int delay = 1000;


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


    thread task(Map);

    //reccurring heartbeat message to server (controller) every k = 1 second 
    while (Map() != 0)
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
    task.join();

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

    return 0;

}

void Map::map(string& line)
{
	//Initializing temporary string for buffering words
	string temp = "";

	//Remove all punctuation and special characters except spaces from the input line
	int index;
	while ((index = line.find_first_of(".,-&!?\\;*+[]<>()'")) != string::npos)
	{
		line.erase(index, 1);
		//cout << "Erase line in mapping.\n";
	}

	//Replace additonal special charcters with space for delimiting
	replace(line.begin(), line.end(), '\n', ' ');
	replace(line.begin(), line.end(), ':', ' ');
	replace(line.begin(), line.end(), '-', ' ');

	//Set all alphabetic characters in input line to lower case 
	transform(line.begin(), line.end(), line.begin(),
		[](unsigned char c) { return tolower(c); });

	//streaming input line string delimtted by spaces for tokenization
	stringstream ss(line);

	//Iterating through string stream to create and store vector of words 
	while (ss >> temp)
	{
		words.push_back(temp);
		//cout << "Pushing to temp string in map...\n";
	}
	
}

string Map::vector_export()
{
	//All words stored in vector from input file are written into the intermediate file as (key, value) pair 
	for (int i = 0; i < words.size(); i++)
	{
		content = content + "(" + "\"" + words[i] + "\"" + ", 1)\n";
		//cout << "Storing key-value pairs in string in map...\n";
	}

	return content;
}

Map* CreateMap()
{
	return new Map();
}
