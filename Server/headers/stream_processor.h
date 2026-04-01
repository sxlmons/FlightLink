#pragma once

#include <WinSock2.h>

// stream_processor.h 
// Per-connection read loop. Reads binary packets from the TCP stream, parses them 
// via direct memory mapping, and routes each message type to the appropriate handler.

// Processes a single client connection from first byte to
// disconnection. Intended as the worker callback for ConnectionManager.
void process_client(SOCKET client);
