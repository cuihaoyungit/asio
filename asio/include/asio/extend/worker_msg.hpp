#ifndef __WORKER_MSG_HPP__
#define __WORKER_MSG_HPP__
#include <asio/extend/worker.hpp>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace asio {
	class Message;
	class MsgWorker : public Worker
	{
	public:
		MsgWorker() :stop(false) {}
		virtual ~MsgWorker()
		{
			this->Stop();
		}

		void Post(Message* msg) {
			try
			{
				std::unique_lock<std::mutex> lock(this->mutex_);
				if (stop) {
					throw std::runtime_error("enqueue on stopped.");
				}
				msg_queue.emplace(std::move(msg));
			}
			catch (std::exception& ex) { std::cout << ex.what() << std::endl; }
			// notify activate thread
			condition.notify_one();
		}

		void Stop() {
			{
				std::unique_lock<std::mutex> lock(this->mutex_);
				stop = true;
			}
			condition.notify_all();
		}

	protected:
		void Exec() override {
			for (;;)
			{
				std::unique_lock<std::mutex> lock(this->mutex_);
				this->condition.wait(lock,
					[this] { return this->stop || !this->msg_queue.empty(); });
				if (this->stop && this->msg_queue.empty())
					return;
				auto msg = std::move(this->msg_queue.front());
				this->msg_queue.pop();
				this->HandleMessage(msg);
			}
		}

		virtual void HandleMessage(Message* msg) {}
	private:
		std::queue<Message*> msg_queue;
		std::mutex mutex_;
		std::condition_variable condition;
		bool stop;
	};
}


#endif // __WORKER_MSG_HPP__
