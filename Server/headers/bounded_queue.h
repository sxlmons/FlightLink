/**
 * @file bounded_queue.h
 * @brief Thread-safe bounded queue implementation for socket distribution
 *
 * Provides a blocking queue with fixed capacity for managing client
 * socket connections between the acceptor thread and worker threads.
 */

#pragma once

#include <WinSock2.h>
#include <queue>
#include <mutex>
#include <condition_variable>

 /**
  * @class BoundedQueue
  * @brief A thread-safe queue with bounded capacity
  *
  * This queue implements the producer-consumer pattern with blocking
  * operations. Producers block when the queue is full, and consumers
  * block when the queue is empty. A shutdown mechanism allows graceful
  * termination of waiting threads.
  *
  * @note This class is designed specifically for SOCKET handles
  */

class BoundedQueue {
public:
	explicit BoundedQueue(size_t capacity) : capacity_(capacity), shutdown_(false) {}

	/**
   * @brief Enqueues a socket, blocking if the queue is full
   *
   * This method will wait until space becomes available in the queue
   * or until shutdown() is called.
   *
   * @param socket The client socket to enqueue
   */
	void push(SOCKET socket) {
		std::unique_lock<std::mutex> lock(mutex_);
		not_full_.wait(lock, [this]() {
			return queue_.size() < capacity_ || shutdown_;
		});

		if (shutdown_) return;

		queue_.push(socket);
		not_empty_.notify_one();
	}

	/**
 * @brief Dequeues a socket, blocking if the queue is empty
 *
 * This method will wait until an item becomes available in the queue
 * or until shutdown() is called.
 *
 * @return The dequeued socket, or INVALID_SOCKET if shutdown was called
 */
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

	/**
   * @brief Signals all waiting threads to wake up and exit
   *
   * This method should be called during server shutdown to allow
   * worker threads to terminate gracefully.
   */
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