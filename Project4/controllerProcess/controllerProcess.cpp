#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define SERVER_ADDRESS "127.0.0.1"

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

void SendMessage(SOCKET socket, const string& message) // controller should ONLY be sending messages to the stub, not to map or reduce
{

    cout << "Message sent: " << message.c_str() << "\n";

    send(socket, message.c_str(), message.size(), 0);
}

void ReceiveMessage(SOCKET socket) // controller should ONLY receive messages from map and reduce, not the stub
{
    int bytesReceived = 0;
    const int bufferSize = 1024;
    char buffer[bufferSize];
    memset(buffer, 0, bufferSize);

    cout << "Listening for data...\n";
    do{
        bytesReceived = recv(socket, buffer, bufferSize - 1, 0);

        if (bytesReceived > 0) {
            cout << string(buffer) << "\n";

        }
        else if (bytesReceived == 0) {
            cout << "Client disconnected.\n";
            break;
        }
        else {
            cout << "Controller recv error: " << WSAGetLastError() << "\n";
            break;
        }
    } while (1);
}


int main() {

    string action = "NULL";
    string inputDirectory = "NULL";
    string tempDirectory = "NULL";
    string outputDirectory = "NULL";
    string stubMessage = "\0"; // this should be formatted according to this example: "1inputdirectory tempdirectory outputdirectory"

    
    
    SOCKET mapReceiverSocket = NULL;
    SOCKET mapListenerSocket = INVALID_SOCKET;

    sockaddr_in mapListenerAddr;
    mapListenerAddr.sin_family = AF_INET;
    mapListenerAddr.sin_port = htons(11112);  // Example port for map server
    mapListenerAddr.sin_addr.s_addr = INADDR_ANY;

    SOCKET reduceReceiverSocket = NULL;
    SOCKET reduceListenerSocket = INVALID_SOCKET;

    sockaddr_in reduceListenerAddr;
    reduceListenerAddr.sin_family = AF_INET;
    reduceListenerAddr.sin_port = htons(11113);  // Example port for reduce server
    reduceListenerAddr.sin_addr.s_addr = INADDR_ANY;

    sockaddr_in stubAddr;


    stubAddr.sin_family = AF_INET;
    stubAddr.sin_port = htons(12345);
    stubAddr.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);


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
        cout << "Failed to create stubSocket." << endl;
        WSACleanup();
        return 1;
    }

    string controllerData = "\0";
    string mapData = "\0";
    string reduceData = "\0"; 


    if (connect(stubSocket, (struct sockaddr*)&stubAddr, sizeof(stubAddr)) == SOCKET_ERROR) {
        cout << "Failed to connect to stub server." << endl;
        closesocket(stubSocket);
        WSACleanup();
        return 1;
    }

    //WORKFLOW//

    while (1) {

        

        cout << "\n\n====Map and Reduce====\n\n";

        cout << "Actions:\n";
        cout << "1: Map files in input directory\n";
        cout << "2: Reduce file in temp directory\n";
        cout << "Anything else: Terminate the stub process\n";
        cout << "Enter the number of an action: ";
        cin >> action;

        if (action != "1" && action != "2") {
            stubMessage = action;
            cout << "Sending termination command to stub process." << "\n";
        }
        else {
            //prompt for inputs
            if (action == "1")
            {
                cout << "Enter the input directory: ";
                cin >> inputDirectory;
            }

            cout << "Enter the temp directory: ";
            cin >> tempDirectory;

            if (action == "2")
            {
                cout << "Enter the output directory: ";
                cin >> outputDirectory;
            }

            stubMessage = action + inputDirectory + " " + tempDirectory + " " + outputDirectory + "\0";
        }

        SendMessage(stubSocket, stubMessage);

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


        if (action == "1") // run this code if the stub is instructed to create a map process
        {
            //create or re-create socket
            mapListenerSocket = socket(AF_INET, SOCK_STREAM, 0);
            if (mapListenerSocket == INVALID_SOCKET) {
                cout << "Failed to create mapListenerSocket." << endl;
                WSACleanup();
                return 1;
            }

            // Bind and listen for map server socket
            if (bind(mapListenerSocket, (struct sockaddr*)&mapListenerAddr, sizeof(mapListenerAddr)) == SOCKET_ERROR) {
                cout << "Failed to bind mapListenerSocket socket." << endl;
                closesocket(mapListenerSocket);
                WSACleanup();
                return 1;
            }
            else {
                cout << "Controller's mapProcess socket bound successfully.\n";
            }

            if (listen(mapListenerSocket, SOMAXCONN) == SOCKET_ERROR) {
                cout << "Listen failed on mapListenerSocket socket." << endl;
                closesocket(mapListenerSocket);
                WSACleanup();
                return 1;
            }
            else {
                cout << "Controller is listening for mapProcess...\n";
            }

            // Accept connection
            mapReceiverSocket = accept(mapListenerSocket, nullptr, nullptr);
            if (mapReceiverSocket == INVALID_SOCKET) {
                cout << "Failed to accept connection." << endl;
                closesocket(mapListenerSocket);
                WSACleanup();
                return 1;
            }
            else {
                cout << "Controller accepted connection from mapProcess client.\n";
            }

            cout << "Beginning to receive messages from mapProcess client...\n";
            ReceiveMessage(mapReceiverSocket);

            closesocket(mapReceiverSocket);
            closesocket(mapListenerSocket);
        }

        else if (action == "2") // run this code if the stub is instructed to create a reduce process
        {
            //create or re-create socket
            reduceListenerSocket = socket(AF_INET, SOCK_STREAM, 0);
            if (reduceListenerSocket == INVALID_SOCKET) {
                cout << "Failed to create reduceListenerSocket." << endl;
                WSACleanup();
                return 1;
            }

            // Bind and listen for reduce server socket
            if (bind(reduceListenerSocket, (struct sockaddr*)&reduceListenerAddr, sizeof(reduceListenerAddr)) == SOCKET_ERROR) {
                cout << "Failed to bind reduce server socket." << endl;
                closesocket(reduceListenerSocket);
                WSACleanup();
                return 1;
            }
            else {
                cout << "Controller's reduce server socket bound successfully.\n";
            }

            if (listen(reduceListenerSocket, SOMAXCONN) == SOCKET_ERROR) {
                cout << "Listen failed on reduce server socket." << endl;
                closesocket(reduceListenerSocket);
                WSACleanup();
                return 1;
            }
            else {
                cout << "Controller is listening for reduce process...\n";
            }

            // Accept connection
            reduceReceiverSocket = accept(reduceListenerSocket, nullptr, nullptr);
            if (reduceReceiverSocket == INVALID_SOCKET) {
                cout << "Failed to accept connection." << endl;
                closesocket(reduceListenerSocket);
                WSACleanup();
                return 1;
            }
            else {
                cout << "Controller accepted connection from reduce client.\n";
            }

            cout << "Beginning to receive messages from reduce client...\n";
            ReceiveMessage(reduceReceiverSocket);
   
            closesocket(reduceReceiverSocket);
            closesocket(reduceListenerSocket);
        }

        else {
            cout << "Closing controller process." << "\n";
            break;
        }

    }

    closesocket(stubSocket);

    WSACleanup();

    cout << "All communication complete.\n";

    return 0;
}