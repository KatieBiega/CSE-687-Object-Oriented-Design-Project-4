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

    cout << "Listening for data...\n";

    recv(socket, buffer, bufferSize - 1, 0);
    return string(buffer);
}


int main() {

    string action = "0";
    string inputDirectory = "\0";
    string tempDirectory = "\0";
    string outputDirectory = "\0";
    string stubMessage = "\0"; // this should be formatted according to this example: "1inputdirectory tempdirectory outputdirectory"



    WSADATA wsaDataMapReduce;
    if (WSAStartup(MAKEWORD(2, 2), &wsaDataMapReduce) != 0) {
        cout << "Failed to initialize Winsock for Map or Reduce." << endl;
        return 1;
    }

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

    string controllerData = "\0";
    string mapData = "\0";
    string reduceData = "\0";

    SOCKET mapServerSocket = socket(AF_INET, SOCK_STREAM, 0);
    SOCKET reduceServerSocket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in mapServerAddr, reduceServerAddr;

    mapServerSocket = socket(AF_INET, SOCK_STREAM, 0);
    reduceServerSocket = socket(AF_INET, SOCK_STREAM, 0);

    mapServerAddr.sin_family = AF_INET;
    mapServerAddr.sin_port = htons(11112);  // Example port for map server
    mapServerAddr.sin_addr.s_addr = INADDR_ANY;

    reduceServerAddr.sin_family = AF_INET;
    reduceServerAddr.sin_port = htons(11112);  // Example port for reduce server
    reduceServerAddr.sin_addr.s_addr = INADDR_ANY;

    SOCKET mapClientSocket = NULL;
    SOCKET reduceClientSocket = NULL;

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

    //WORKFLOW//

    while (1) {

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

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


        if (action == "1") // run this code if the stub is instructed to create a map process
        {
            // Bind and listen for map server socket
            if (bind(mapServerSocket, (struct sockaddr*)&mapServerAddr, sizeof(mapServerAddr)) == SOCKET_ERROR) {
                cout << "Failed to bind map server socket." << endl;
                closesocket(mapServerSocket);
                WSACleanup();
                return 1;
            }
            else {
                cout << "Controller's map server socket bound successfully.\n";
            }

            if (listen(mapServerSocket, SOMAXCONN) == SOCKET_ERROR) {
                cout << "Listen failed on map server socket." << endl;
                closesocket(mapServerSocket);
                WSACleanup();
                return 1;
            }
            else {
                cout << "Controller is listening for map process...\n";
            }

            // Accept connection
            mapClientSocket = accept(mapServerSocket, nullptr, nullptr);
            if (reduceClientSocket == INVALID_SOCKET) {
                cout << "Failed to accept connection." << endl;
                closesocket(mapServerSocket);
                WSACleanup();
                return 1;
            }
            else {
                cout << "Controller accepted connection from map client.\n";
            }

            while (mapData != "Map error" && mapData != "Map complete") {
                mapData = ReceiveMessage(mapClientSocket);
                cout << "Received data in controller from map process: " << mapData << endl;
            }
            closesocket(mapClientSocket);
            closesocket(mapServerSocket);
        }

        else if (action == "2") // run this code if the stub is instructed to create a reduce process
        {

            // Bind and listen for reduce server socket
            if (bind(reduceServerSocket, (struct sockaddr*)&reduceServerAddr, sizeof(reduceServerAddr)) == SOCKET_ERROR) {
                cout << "Failed to bind reduce server socket." << endl;
                closesocket(reduceServerSocket);
                WSACleanup();
                return 1;
            }
            else {
                cout << "Controller's reduce server socket bound successfully.\n";
            }

            if (listen(reduceServerSocket, SOMAXCONN) == SOCKET_ERROR) {
                cout << "Listen failed on reduce server socket." << endl;
                closesocket(reduceServerSocket);
                WSACleanup();
                return 1;
            }
            else {
                cout << "Controller is listening for reduce process...\n";
            }

            // Accept connection
            reduceClientSocket = accept(reduceServerSocket, nullptr, nullptr);
            if (reduceClientSocket == INVALID_SOCKET) {
                cout << "Failed to accept connection." << endl;
                closesocket(reduceServerSocket);
                WSACleanup();
                return 1;
            }
            else {
                cout << "Controller accepted connection from reduce client.\n";
            }

            while (reduceData != "Reduce error" && reduceData != "Reduce complete") {
                reduceData = ReceiveMessage(reduceClientSocket);
                cout << "Received data in controller from reduce process: " << reduceData << endl;
            }

            closesocket(reduceClientSocket);
            closesocket(reduceServerSocket);
        }

        else {
            cout << "Closing stub and comtroller process." << "\n";
            break;
        }

    }

    closesocket(stubSocket);

    WSACleanup();

    cout << "All communication complete.\n";

    return 0;
}