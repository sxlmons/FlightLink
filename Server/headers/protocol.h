#pragma once

#include <cstdint>

/**
 * @file protocol.h
 * @brief Defines the binary packet format for the FlightLink protocol
 *
 * This file contains the message type definitions, payload sizes,
 * and packet structures used for communication between the flight
 * data client and server.
 */

 /**
  * @brief Message type indicating the start of a flight session
  */
constexpr uint8_t MSG_FLIGHT_START = 0x01;

/**
 * @brief Message type containing telemetry data during flight
 */
constexpr uint8_t MSG_TELEMETRY    = 0x02;

/**
 * @brief Message type indicating the end of a flight session
 */
constexpr uint8_t MSG_FLIGHT_END   = 0x03;

/**
 * @brief Size of the header payload in bytes (plane_id only)
 */
constexpr size_t HEADER_PAYLOAD_SIZE    = 4;

/**
 * @brief Size of the telemetry payload in bytes (plane_id + timestamp + fuel_remaining)
 */
constexpr size_t TELEMETRY_PAYLOAD_SIZE = 20;



/**
 * @struct PacketHeader
 * @brief Header structure for flight start and end messages
 *
 * Contains the unique identifier for the aircraft sending the message.
 */
#pragma pack(push, 1)

struct PacketHeader {
	uint32_t plane_id;
};

/**
 * @struct TelemetryData
 * @brief Complete telemetry packet structure
 *
 * Contains all data fields for a telemetry update during flight,
 * including aircraft identification, timestamp, and fuel status.
 */

struct TelemetryData {
	uint32_t plane_id;		///< Unique aircraft identifier
	double timestamp;		///< Unix timestamp of the reading
	double fuel_remaining;	///< Fuel quantity remaining in gallons
};

# pragma(pop)
	
