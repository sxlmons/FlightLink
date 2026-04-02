#pragma once

#include <WinSock2.h>
#include <vector>
#include <thread>
#include <functional>
#include "bounded_queue.h"

// connection_manager.h
// Manages the worker thread pool and dispatches client connections
// from the bounded queue to available threads 

class ConnectionManager {
public:
	// Creates the bounded queue (capacity = 2 * pool_size)
	// Spins up pool_size worker threads.
	ConnectionManager(size_t pool_size, std::function<void(SOCKET)> handler);

	// Enqueues a client socket for processing 
	// Blocks if the queue is full
	void enqueue(SOCKET client);

	void shutdown();

	~ConnectionManager();

	ConnectionManager(const ConnectionManager&) = delete;
	ConnectionManager& operator=(const ConnectionManager&) = delete;

private:
	std::function<void(SOCKET)> handler_;
	BoundedQueue                queue_;
	std::vector<std::thread>    workers_;
	bool                        shutdown_called_;
};