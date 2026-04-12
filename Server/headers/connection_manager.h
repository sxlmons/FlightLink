/**
 * @file connection_manager.h
 * @brief Worker thread pool manager for client connection handling
 *
 * Manages a pool of worker threads and dispatches client connections
 * from a bounded queue to available threads for processing.
 */

#pragma once

#include <WinSock2.h>
#include <vector>
#include <thread>
#include <functional>
#include "bounded_queue.h"

 /**
  * @class ConnectionManager
  * @brief Manages a thread pool for processing client connections
  *
  * This class creates and manages a pool of worker threads that process
  * client connections from a bounded queue. It provides graceful shutdown
  * capabilities and ensures all connections are properly cleaned up.
  *
  * The queue capacity is set to 2 * pool_size to provide buffering
  * while preventing unbounded queue growth.
  */

class ConnectionManager {
public:
	/**
  * @brief Constructs the connection manager and starts worker threads
  *
  * Creates a bounded queue with capacity = 2 * pool_size and spins up
  * the specified number of worker threads.
  *
  * @param pool_size Number of worker threads to create
  * @param handler Function to process each client connection
  */
	ConnectionManager(size_t pool_size, std::function<void(SOCKET)> handler);

	/**
	* @brief Enqueues a client socket for processing
	*
	* This method may block if the queue is full. The socket will be
	* picked up by an available worker thread and passed to the handler.
	*
	* @param client The accepted client socket to enqueue
	*/
	void enqueue(SOCKET client);

	/**
  * @brief Initiates graceful shutdown of all worker threads
  *
  * Wakes all waiting workers, allows them to finish their current
  * client, and then joins all threads. This method is idempotent.
  */
	void shutdown();


	/**
	 * @brief Destructor - ensures proper shutdown
	 */
	~ConnectionManager();

	ConnectionManager(const ConnectionManager&) = delete;
	ConnectionManager& operator=(const ConnectionManager&) = delete;

private:
	std::function<void(SOCKET)> handler_;
	BoundedQueue                queue_;
	std::vector<std::thread>    workers_;
	bool                        shutdown_called_;
};