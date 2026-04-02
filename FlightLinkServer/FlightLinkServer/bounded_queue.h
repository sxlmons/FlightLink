#pragma once

#include <WinSock2.h>
#include <queue>
#include <mutex>
#include <condition_variable>

// bounded_queue.h
//
// push() blocks when the queue is at capacity
// pop() blocks when the queue is empty
// shutdown() wakes all waiting threads so they can exit

class BoundedQueue {
public:
	explicit BoundedQueue(size_t capacity) : capacity_(capacity), shutdown_(false) {}

	// Blocks until space is available, then enques the socket
	void push(SOCKET socket) {
		std::unique_lock<std::mutex> lock(mutex_);
		not_full_.wait(lock, [this]() {
			return queue_.size() < capacity_ || shutdown_;
			});

		if (shutdown_) return;

		queue_.push(socket);
		not_empty_.notify_one();
	}

	// Blocks until a socket is available, then dequeues and returns it
	SOCKET pop() {
		std::unique_lock<std::mutex> lock(mutex_);
		not_empty_.wait(lock, [this]() {
			return !queue_.empty() || shutdown_;
			});

		if (shutdown_ && queue_.empty()) return INVALID_SOCKET;

		SOCKET socket = queue_.front();
		queue_.pop();
		not_full_.notify_one();
		return socket;
	}

	// Signals all waiting threads to wake up and exit.
	void shutdown() {
		std::unique_lock<std::mutex> lock(mutex_);
		shutdown_ = true;
		not_full_.notify_all();
		not_empty_.notify_all();
	}

private:
	std::queue<SOCKET>	queue_;
	size_t				capacity_;
	bool				shutdown_;

	std::mutex				mutex_;
	std::condition_variable not_full_;
	std::condition_variable not_empty_;



};