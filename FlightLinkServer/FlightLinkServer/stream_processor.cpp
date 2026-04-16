#include "stream_processor.h"
#include "protocol.h"
#include "flight_session_manager.h"
#include <iostream>

// stream_processor.cpp

#include <iomanip>

// For debugging purposes 
static void dump_bytes(const char* label, const void* data, size_t len) {
	const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data);
	std::cout << "[dump] " << label << " (" << len << " bytes): ";
	for (size_t i = 0; i < len; i++) {
		std::cout << std::hex << std::setfill('0') << std::setw(2)
			<< static_cast<int>(bytes[i]) << " ";
	}
	std::cout << std::dec << std::endl;
}

// This function helps maintain data integrity as with TCP we can't guarantee 20 bytes
// will be sent in one packet, it may split into two
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

	FlightSession session{};

	while (true) {
		// read first byte
		uint8_t msg_type;
		if (!recv_exact(client, reinterpret_cast<char*>(&msg_type), 1)) {
			std::cout << "[stream] Client disconnected." << std::endl;
			interrupt_session(session);
			break;
		}

		switch (msg_type) {
		case MSG_FLIGHT_START: {
			PacketHeader header;
			if (!recv_exact(client, reinterpret_cast<char*>(&header), HEADER_PAYLOAD_SIZE)) {
				std::cout << "[stream] disconnected during flight start." << std::endl;
				interrupt_session(session);
				return;
			}
			std::cout << "[stream] Parsed plane_id: " << header.plane_id << std::endl;

			session = start_session(header.plane_id);
			break;
		}
		case MSG_TELEMETRY: {
			TelemetryData telemetry;
			if (!recv_exact(client, reinterpret_cast<char*>(&telemetry), TELEMETRY_PAYLOAD_SIZE)) {
				std::cout << "[stream] Disconnected during telemetry." << std::endl;
				interrupt_session(session);
				return;
			}  
			
			process_telemetry(session, telemetry.fuel_remaining);
			break;
		}
		case MSG_FLIGHT_END: {
			PacketHeader header;
			if (!recv_exact(client, reinterpret_cast<char*>(&header), HEADER_PAYLOAD_SIZE)) {
				std::cout << "[stream] Disconnected during flight end." << std::endl;
				interrupt_session(session);
				return;
			}

			finalize_session(session);
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