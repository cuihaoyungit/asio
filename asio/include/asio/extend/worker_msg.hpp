//
// worker.hpp
// add by [11/21/2022 cuihaoyun]
// wrap cpp11 thread

#ifndef __WORKER_MSG_HPP__
#define __WORKER_MSG_HPP__
#include <asio/extend/worker.hpp>
#include <asio/msgdef/message.hpp>
#include <queue>
#include <mutex>

namespace asio {

	class MsgWorker : public Worker
	{
	public:
		MsgWorker() :stop(false) {}
		~MsgWorker()
		{
			this->Stop();
		}

		void PostMsg(asio::Message* msg) {
			{
				std::unique_lock<std::mutex> lock(this->queue_mutex);
				if (stop) {
					throw std::runtime_error("enqueue on stopped.");
				}
				msg_queue.emplace(std::move(msg));
			}
			condition.notify_one();
		}

		void Stop() {
			{
				std::unique_lock<std::mutex> lock(queue_mutex);
				stop = true;
			}
			condition.notify_all();
		}

	protected:
		void Run() override {
			for (;;)
			{
				std::unique_lock<std::mutex> lock(this->queue_mutex);
				this->condition.wait(lock,
					[this] { return this->stop || !this->msg_queue.empty(); });
				if (this->stop && this->msg_queue.empty())
					return;
				auto msg = std::move(this->msg_queue.front());
				this->msg_queue.pop();
				this->HandleMessage(msg);
			}
		}

		virtual void HandleMessage(asio::Message* msg) {}
	private:
		std::queue<asio::Message*> msg_queue;
		std::mutex queue_mutex;
		std::condition_variable condition;
		bool stop;
	};
}


#endif // __WORKER_MSG_HPP__
