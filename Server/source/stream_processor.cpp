/**
 * @file stream_processor.cpp
 * @brief Implementation of the client connection processing logic
 */

#include "stream_processor.h"
#include "protocol.h"
#include <iostream>

// stream_processor.cpp
/**
 * @brief Receives exactly the specified number of bytes from a socket
 *
 * This function handles the fact that TCP may fragment messages across
 * multiple packets. It continues reading until all requested bytes
 * have been received or an error occurs.
 *
 * @param socket The socket to read from
 * @param buffer Destination buffer for received data
 * @param length Exact number of bytes to receive
 * @return true if all bytes were received successfully, false otherwise
 */
static bool recv_exact(SOCKET socket, char* buffer, size_t length) {
	size_t received = 0;

	while (received < length) {
		int result = recv(socket, buffer + received, static_cast<int>(length - received), 0);
		
		if (result <= 0)
			return false;

		received += static_cast<size_t>(result);
	}

	return true;
}

void process_client(SOCKET client) {
	std::cout << "[stream] Client connected" << std::endl;

	while (true) {
		// read first byte
		uint8_t msg_type;
		if (!recv_exact(client, reinterpret_cast<char*>(&msg_type), 1)) {
			std::cout << "[stream] Client disconnected." << std::endl;
			break;
		}

		switch (msg_type) {
		case MSG_FLIGHT_START: {
			PacketHeader header;
			if (!recv_exact(client, reinterpret_cast<char*>(&header), HEADER_PAYLOAD_SIZE)) {
				std::cout << "[stream] disconnected during flight start." << std::endl;
				return;
			}

			// TODO: Create flight session

			std::cout << "[stream] Flight started | plane_id: " << header.plane_id << std::endl;
			break;
		}
			case MSG_TELEMETRY: {
			TelemetryData telemetry;
			if (!recv_exact(client, reinterpret_cast<char*>(&telemetry), TELEMETRY_PAYLOAD_SIZE)) {
				std::cout << "[stream] Disconnected during telemetry." << std::endl;
				return;
			}

			// TODO: Pass to FlightSessionManager for fuel calculation.

			std::cout << "[stream] Telemetry | plane_id: " 
				<< telemetry.plane_id << " | timestamp: " 
				<< telemetry.timestamp << " | fuel: " 
				<< telemetry.fuel_remaining 
				<< std::endl;
			break;
		}
			case MSG_FLIGHT_END: {
			PacketHeader header;
			if (!recv_exact(client, reinterpret_cast<char*>(&header), HEADER_PAYLOAD_SIZE)) {
				std::cout << "[stream] Disconnected during flight end." << std::endl;
				return;
			}

			// TODO: Finalize flight session via FlightSessionManager.
			std::cout << "[stream] Flight ended  | plane_id: " << header.plane_id << std::endl;
			break;

		}
		default: {
			std::cout << "[stream] Malformed packet | unknown type: 0x"
				<< std::hex << static_cast<int>(msg_type)
				<< std::dec << std::endl;
			break;
		}
		}
	}
}