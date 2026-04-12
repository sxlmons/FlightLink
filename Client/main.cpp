/**
 * @file main.cpp
 * @brief FlightLink Telemetry Client - Simulates aircraft data transmission
 *
 * A Linux-based TCP client that reads telemetry data from CSV files and
 * transmits it to the FlightLink server using the binary protocol format.
 * Simulates realistic flight data transmission with configurable delays.
 *
 * Usage: ./client <server_ip>
 *   server_ip - IPv4 address of the FlightLink server
 *
 * @author FlightLink Team
 * @date 2024
 */

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <random>
#include <fstream>
#include <string>
#include <sstream>
#include <thread>
#include <cstdint>

using namespace std;

/**
 * @struct AircraftPacket
 * @brief Binary packet structure matching the FlightLink protocol
 *
 * Packed to ensure consistent binary layout across platforms.
 */
#pragma pack(push, 1)
struct AircraftPacket {
    uint8_t message_type;  // 0x01 (Start), 0x02 (Data), or 0x03 (End)
    uint32_t plane_id;     // 4-byte ID

    double timestamp;      // 8-byte
    double fuel_remaining; // 8-byte fuel quantity
};
#pragma pack(pop)

/**
 * @brief Generates a random number between 0 and 3 for file selection
 * @return Random integer in range [0, 3]
 */
int getRandomNumber();

/**
 * @brief Generates a globally unique aircraft identifier
 *
 * Combines process ID, high-resolution timestamp, and random data
 * to create a unique 32-bit identifier for each client session.
 *
 * @return Unique 32-bit aircraft identifier
 */
uint32_t generateGlobalUniqueId();

/**
 * @brief Sends a binary packet to the server
 *
 * @param socket Connected socket to send on
 * @param type Message type (0x01, 0x02, or 0x03)
 * @param id Aircraft identifier
 * @param timestamp Timestamp value (ignored for non-telemetry packets)
 * @param fuel_remaining Fuel value (ignored for non-telemetry packets)
 * @return true if send succeeded, false otherwise
 */
bool sendPacket(int socket, uint8_t type, uint32_t id, double timestamp, double fuel_remaining);

/**
 * @brief Processes a telemetry data file and transmits all readings
 *
 * Reads timestamp/fuel pairs from a CSV file and transmits them
 * as telemetry packets with 750ms delay between readings.
 *
 * @param filename Path to telemetry CSV file
 * @param socket Connected socket for transmission
 * @param planeId Aircraft identifier to use
 * @return true if all packets sent successfully, false on failure
 */
bool processTelemetryFile(const std::string& filename, int socket, uint32_t planeId);

/**
 * @brief Gets the project directory path for locating telemetry files
 * @return Absolute path to the project directory
 */
string getProjectPath();

int main(int argc, char* argv[])
{
    auto clientId = generateGlobalUniqueId();

    string TelemetryPaths[4];
    TelemetryPaths[0] = "/Telemetry Data/katl-kefd-B737-700.txt";
    TelemetryPaths[1] = "/Telemetry Data/Telem_2023_3_12 16_26_4.txt";
    TelemetryPaths[2] = "/Telemetry Data/Telem_2023_3_12 14_56_40.txt";
    TelemetryPaths[3] = "/Telemetry Data/Telem_czba-cykf-pa28-w2_2023_3_1 12_31_27.txt";

    string path = getProjectPath() + TelemetryPaths[getRandomNumber()];

    int ClientSocket;
    ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ClientSocket == -1) {
        return 0;
    }

    sockaddr_in SvrAddr;
    SvrAddr.sin_family = AF_INET;						//Address family type internet
    SvrAddr.sin_port = htons(27000);					//port (host to network conversion)
    SvrAddr.sin_addr.s_addr = inet_addr(argv[1]);	    //IP address 10.144.114.37
    if ((connect(ClientSocket, (struct sockaddr*)&SvrAddr, sizeof(SvrAddr))) == -1) {
        close(ClientSocket);
        return 0;
    }

    // Send Start Packet
    if (!sendPacket(ClientSocket, 0x01, clientId, 0x00, 0x00))
        return 0;

    // Telemetry Data Send
    if (!processTelemetryFile(path, ClientSocket, clientId))
        return 0;

    // Send End Packet
    if (!sendPacket(ClientSocket, 0x03, clientId, 0x00, 0x00))
        return 0;

    close(ClientSocket);

    return 1;
}

string getProjectPath() {
    std::string fullPath = __FILE__;
    size_t lastSlashPos = fullPath.find_last_of("/\\");

    std::string projectPath = "";
    if (lastSlashPos != std::string::npos) {
        projectPath = fullPath.substr(0, lastSlashPos);
    } else {
        projectPath = ".";
    }

    return projectPath;
}

bool sendPacket(int socket, uint8_t type, uint32_t id, double timestamp = 0.0, double fuel_remaining = 0.0) {
    AircraftPacket pkt;
    pkt.message_type = type;
    pkt.plane_id = id;

    // Set data fields. For 0x01/0x03, these are the "Null Values"
    pkt.timestamp = timestamp;
    pkt.fuel_remaining = fuel_remaining;

    size_t bytesToSend = (type == 0x02) ? 21 : 5;

    if (send(socket, (char*)&pkt, bytesToSend, 0) == -1) {
        return false;
    }
    return true;
}

int getRandomNumber() {
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::mt19937 engine(seed);
    std::uniform_int_distribution<int> dist(0, 3);
    int random_num = dist(engine);

    return random_num;
}

uint32_t generateGlobalUniqueId() {
    std::random_device rd;

    auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();

    uint32_t pid = static_cast<uint32_t>(getpid());

    std::mt19937 gen(rd() ^ static_cast<uint32_t>(now) ^ pid);
    std::uniform_int_distribution<uint32_t> distrib(1, 0xFFFFFFFF);

    return distrib(gen);
}

bool processTelemetryFile(const std::string& filename, int socket, uint32_t planeId) {
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Error: Could not open telemetry file: " << filename << std::endl;
        return false;
    }

    std::string line;
    int failedPacketCounter = 0;

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string ts_str, fuel_str;

        if (std::getline(ss, ts_str, ',') && std::getline(ss, fuel_str, ',')) {
            try {
                double timestamp = std::stod(ts_str);
                double fuelRemaining = std::stod(fuel_str);

                if (!sendPacket(socket, 0x02, planeId, timestamp, fuelRemaining))
                    failedPacketCounter++;

                if (failedPacketCounter == 5)
                    return false;
                // 0.75s sleep
                std::this_thread::sleep_for(std::chrono::milliseconds(750));
            }
            catch (const std::exception& e) {
                // Log bad data but continue processing
                std::cerr << "Skipping malformed line: " << line << " (" << e.what() << ")" << std::endl;
            }
        }
    }

    file.close();
    return true;
}