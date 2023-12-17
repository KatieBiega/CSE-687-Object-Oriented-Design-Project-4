#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS


#include <iostream>
#include <string>

#include <Windows.h>
#include <WinSock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

using std::string;
using std::cout;
using std::cin;
using std::endl;

void SendMessage(SOCKET socket, const string& message) {

    cout << "Message sent: " << message.c_str() << "\n";

    send(socket, message.c_str(), message.size(), 0);
}

string ReceiveMessage(SOCKET socket) {
    const int bufferSize = 1024;
    char buffer[bufferSize];
    memset(buffer, 0, bufferSize);
    recv(socket, buffer, bufferSize - 1, 0);
    return string(buffer);
}


int main() {

    string action = "0";
    string inputDirectory = "\0";
    string tempDirectory = "\0";
    string outputDirectory = "\0";
    string stubMessage = "\0"; // this should be formatted according to this example: "1inputdirectory tempdirectory outputdirectory"

//WORKFLOW//

// Communication with the stub process
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "Failed to initialize Winsock." << endl;
        return 1;
    }

    SOCKET stubSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (stubSocket == INVALID_SOCKET) {
        cout << "Failed to create socket." << endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in stubAddr;
    stubAddr.sin_family = AF_INET;
    stubAddr.sin_port = htons(12345);
    stubAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(stubSocket, (struct sockaddr*)&stubAddr, sizeof(stubAddr)) == SOCKET_ERROR) {
        cout << "Failed to connect to stub server." << endl;
        closesocket(stubSocket);
        WSACleanup();
        return 1;
    }

    //prompt for inputs
    cout << "Enter the input directory: ";
    cin >> inputDirectory;

    cout << "Enter the temp directory: ";
    cin >> tempDirectory;

    cout << "Enter the output directory: ";
    cin >> outputDirectory;

    cout << "Actions:\n";
    cout << "1: Map files in input directory\n";
    cout << "2: Reduce file in temp directory\n";
    cout << "Anything else: Terminate the stub process\n";
    cout << "Enter the number of an action:";
    cin >> action;

    stubMessage = action + inputDirectory + " " + tempDirectory + " " + outputDirectory + "\0";

    SendMessage(stubSocket, stubMessage);

    // Wait for the stub to respond or handle the response accordingly
    string stubResponse = ReceiveMessage(stubSocket);
    cout << "Stub response: " << stubResponse << endl;

    closesocket(stubSocket);
    WSACleanup();

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    int mapServerSocket = socket(AF_INET, SOCK_STREAM, 0);
    int reduceServerSocket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in mapServerAddr, reduceServerAddr;

    WSADATA wsaDataMapReduce;
    if (WSAStartup(MAKEWORD(2, 2), &wsaDataMapReduce) != 0) {
        cout << "Failed to initialize Winsock for MapReduce." << endl;
        return 1;
    }

    mapServerSocket = socket(AF_INET, SOCK_STREAM, 0);
    reduceServerSocket = socket(AF_INET, SOCK_STREAM, 0);

    mapServerAddr.sin_family = AF_INET;
    mapServerAddr.sin_port = htons(11112);  // Example port for map server
    mapServerAddr.sin_addr.s_addr = INADDR_ANY;

    reduceServerAddr.sin_family = AF_INET;
    reduceServerAddr.sin_port = htons(11112);  // Example port for reduce server
    reduceServerAddr.sin_addr.s_addr = INADDR_ANY;

    // Bind and listen for map server socket
    if (bind(mapServerSocket, (struct sockaddr*)&mapServerAddr, sizeof(mapServerAddr)) == SOCKET_ERROR) {
        cout << "Failed to bind map server socket." << endl;
        closesocket(mapServerSocket);
        WSACleanup();
        return 1;
    }

    if (listen(mapServerSocket, SOMAXCONN) == SOCKET_ERROR) {
        cout << "Listen failed on map server socket." << endl;
        closesocket(mapServerSocket);
        WSACleanup();
        return 1;
    }

    // Bind and listen for reduce server socket
    if (bind(reduceServerSocket, (struct sockaddr*)&reduceServerAddr, sizeof(reduceServerAddr)) == SOCKET_ERROR) {
        cout << "Failed to bind reduce server socket." << endl;
        closesocket(reduceServerSocket);
        WSACleanup();
        return 1;
    }

    if (listen(reduceServerSocket, SOMAXCONN) == SOCKET_ERROR) {
        cout << "Listen failed on reduce server socket." << endl;
        closesocket(reduceServerSocket);
        WSACleanup();
        return 1;
    }


    // Accept connections
    SOCKET mapClientSocket = accept(mapServerSocket, nullptr, nullptr);
    SOCKET reduceClientSocket = accept(reduceServerSocket, nullptr, nullptr);

    if (mapClientSocket == INVALID_SOCKET || reduceClientSocket == INVALID_SOCKET) {
        cout << "Failed to accept connection." << endl;
        closesocket(mapServerSocket);
        closesocket(reduceServerSocket);
        WSACleanup();
        return 1;
    }

    closesocket(mapServerSocket);
    closesocket(reduceServerSocket);

    string controllerData = "Data from controller to map process";
    SendMessage(mapClientSocket, controllerData);

    string mapData = ReceiveMessage(mapClientSocket);
    cout << "Received data in controller from map process: " << mapData << endl;

    controllerData = "Data from controller to reduce process";
    SendMessage(reduceClientSocket, controllerData);

    string reduceData = ReceiveMessage(reduceClientSocket);
    cout << "Received data in controller from reduce process: " << reduceData << endl;

    closesocket(mapClientSocket);
    closesocket(reduceClientSocket);
    WSACleanup();


    cout << "All communication complete.\n";

    return 0;
}