//
// worker.hpp
// add by [11/21/2022 cuihaoyun]
// wrap cpp11 thread

#ifndef __WORKER_HPP__
#define __WORKER_HPP__
#include <thread>

namespace asio {

	class Worker
	{
	public:
		Worker() ASIO_NOEXCEPT {}

		virtual ~Worker() {
			this->WaitStop();
		}

		virtual void Run() {}

		void Startup() {
			thread_ = std::thread(std::bind(&Worker::RunThread, this));
		}

		void WaitStop() {
			if (thread_.joinable())
			{
				thread_.join();
			}
		}

		std::thread::id GetThreadId() {
			return thread_.get_id();
		}

	protected:
		void RunThread() {
			this->Run();
		}
		Worker(const Worker&) = delete;
		Worker operator = (const Worker&) = delete;
	private:
		std::thread thread_;
		//std::unique_ptr<std::thread> m_thread;
	};
}


#endif // __WORKER_HPP__
