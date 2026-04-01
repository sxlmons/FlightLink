#include "connection_manager.h"

// connection_manager.cpp

ConnectionManager::ConnectionManager(size_t pool_size, std::function<void(SOCKET)> handler)
	: handler_(std::move(handler)),
	queue_(pool_size * 2),
	shutdown_called_(false)
{
	for (size_t i = 0; i < pool_size; i++) {
		workers_.emplace_back([this]() {
			while (true) {
				SOCKET client = queue_.pop();

				if (client == INVALID_SOCKET)
					break;

				handler_(client);
				closesocket(client);
			}
		});
	}
}

void ConnectionManager::enqueue(SOCKET client) {
	queue_.push(client);
}

void ConnectionManager::shutdown() {
	if (shutdown_called_) 
		return;

	shutdown_called_ = true;

	// Wake all workers waiting on an empty queue
	queue_.shutdown();

	// wait for every worker to finish its current client
	for (auto& worker : workers_) {
		if (worker.joinable()) {
			worker.join;
		}
	}
}

ConnectionManager::~ConnectionManager() {
	shutdown();
}