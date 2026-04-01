#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>

using namespace std;

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed" << endl;
        return 1;
    }

    SOCKET ServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ServerSocket == INVALID_SOCKET) {
        cerr << "Could not create socket" << endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in SvrAddr = {};
    SvrAddr.sin_family = AF_INET;
    SvrAddr.sin_addr.s_addr = INADDR_ANY;
    SvrAddr.sin_port = htons(27000);

    if (bind(ServerSocket, (sockaddr*)&SvrAddr, sizeof(SvrAddr)) == SOCKET_ERROR) {
        closesocket(ServerSocket);
        WSACleanup();
        return 1;
    }

    if (listen(ServerSocket, 5) == SOCKET_ERROR) {
        closesocket(ServerSocket);
        WSACleanup();
        return 1;
    }

    cout << "Listening on port 27000" << endl;

    SOCKET ClientSocket = accept(ServerSocket, nullptr, nullptr);
    if (ClientSocket == INVALID_SOCKET) {
        closesocket(ServerSocket);
        WSACleanup();
        return 1;
    }

    cout << "Client connected" << endl;

    char buffer[1024];
    while (recv(ClientSocket, buffer, sizeof(buffer), 0) > 0) {
        // drop everything
    }

    cout << "Client disconnected" << endl;

    closesocket(ClientSocket);
    closesocket(ServerSocket);
    WSACleanup();
    return 0;
}