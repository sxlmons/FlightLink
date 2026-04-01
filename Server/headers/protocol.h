#pragma once

#include <cstint>

// Protocol.h 
// Defines the binary packet format for the FlightLink protocol

// Message Types
constexpr uint8_t MSG_FLIGHT_START = 0x01;
constexpr uint8_t MSG_TELEMETRY    = 0x02;
constexpr uint8_t MSG_FLIGHT_END   = 0x03;

// Payload Sizes 
constexpr size_t HEADER_PAYLOAD_SIZE    = 4;
constexpr size_t TELEMETRY_PAYLOAD_SIZE = 20;

// Packet structs 

#pragma pack(push, 1)

struct PacketHeader {
	uint32_t plane_id;
};

struct TelemetryData {
	uint32_t plane id;
	double timestamp;
	double fuel_remianing;
};

# pragma(pop)
	
