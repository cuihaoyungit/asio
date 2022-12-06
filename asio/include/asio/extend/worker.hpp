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

		bool Startup() {
			if (!thread_) {
				thread_ = std::make_unique<std::thread>(std::bind(&Worker::RunThread, this));
			}
			return true;
		}

		void WaitStop() {
			if (thread_->joinable())
			{
				thread_->join();
			}
		}

		std::thread::id GetThreadId() {
			return thread_->get_id();
		}

		static std::thread::id GetCurrentThreadId() {
			return std::this_thread::get_id();
		}

		const std::string& Name() {
			return name_;
		}

		void SetName(const std::string &name) {
			this->name_ = name;
		}

	protected:
		void RunThread() {
			this->Run();
		}
		virtual void Init() {}
		virtual void Exit() {}
		Worker(const Worker&) = delete;
		Worker operator = (const Worker&) = delete;
	private:
		std::unique_ptr<std::thread> thread_;
		std::string name_;
	};
}


#endif // __WORKER_HPP__
