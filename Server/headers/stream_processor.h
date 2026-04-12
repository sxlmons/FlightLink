/**
 * @file stream_processor.h
 * @brief Per-connection read loop handler
 *
 * Reads binary packets from the TCP stream, parses them via direct memory
 * mapping, and routes each message type to the appropriate handler.
 */

#pragma once

#include <WinSock2.h>

 /**
  * @brief Processes a single client connection from first byte to disconnection
  *
  * This function runs the main read loop for a connected client, parsing
  * incoming binary packets according to the FlightLink protocol. It handles
  * flight start, telemetry data, and flight end messages, routing each to
  * the FlightSessionManager for processing.
  *
  * Intended as the worker callback for ConnectionManager.
  *
  * @param client The connected client socket to process
  *
  * @note This function will block until the client disconnects or an error occurs
  * @see ConnectionManager
  * @see FlightSessionManager
  */
void process_client(SOCKET client);
