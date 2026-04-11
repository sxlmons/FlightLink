#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <optional>

#include "sqlite3.h"
#include "protocol.h"
#include "connection_manager.h"
#include "stream_processor.h"

struct ServerConfig {
    int port;
    int pool_size;
};

std::optional<ServerConfig> parse_args(int argc, char* argv[]);

int main(int argc, char* argv[]) {
    auto config = parse_args(argc, argv);
    if (!config)
        return 1;

    int port = config->port;
    int pool_size = config->pool_size;

    

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed." << std::endl;
        return 1;
    }

    SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_socket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(static_cast<u_short>(port));

    if (bind(listen_socket, reinterpret_cast<sockaddr*>(&server_addr),
        sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }

    if (listen(listen_socket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }

    std::cout << "Server listening on port " << port << " with " << pool_size << " worker threads." << std::endl;

    // Start ConnectionManager
    ConnectionManager manager(static_cast<size_t>(pool_size), process_client);

    // Accept Loop
    while (true) {
        SOCKET client = accept(listen_socket, nullptr, nullptr);

        if (client == INVALID_SOCKET) {
            std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
            continue;
        }

        manager.enqueue(client);
    }

    closesocket(listen_socket);
    manager.shutdown();
    WSACleanup();

    return 0;
}



std::optional<ServerConfig> parse_args(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: server.exe <port> <pool_size>" << std::endl;
        return std::nullopt;
    }

    int port;
    int pool_size;
    try {
        port = std::stoi(argv[1]);
        pool_size = std::stoi(argv[2]);
    }
    catch (const std::exception& e) {
        std::cerr << "Invalid arguments: " << e.what() << std::endl;
        return std::nullopt;
    }

    if (port <= 0 || port > 65535 || pool_size <= 0) {
        std::cerr << "Port must be 1-65535 and pool size must "
            << "be a positive integer." << std::endl;
        return std::nullopt;
    }

    return ServerConfig{ port, pool_size };
}