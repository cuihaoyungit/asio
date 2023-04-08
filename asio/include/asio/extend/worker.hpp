// worker.hpp
// add by [11/21/2022 cuihaoyun]
// wrap cpp11 thread
#ifndef __WORKER_HPP__
#define __WORKER_HPP__
#include <thread>
#include <iostream>
#include <functional>

namespace asio {
	class Message;
	class Worker
	{
	public:
		Worker() = default;

		virtual ~Worker() {
#if 0
			this->WaitStop(); // thread can't stop yourself [ must be called on the main thread]
#endif
		}

		bool Startup() {
			if (!thread_) {
				thread_ = std::make_unique<std::thread>(std::bind(&Worker::RunThread, this));
				return true;
			}
			return false;
		}

		// wait thread exit
		void WaitStop() {
			if (thread_->joinable())
			{
				thread_->join();
			}
		}

		std::thread::id GetThreadId() {
			return thread_->get_id();
		}

		const std::string& Name() {
			return name_;
		}

		void SetName(const std::string &name) {
			this->name_ = name;
		}

	private:
		void RunThread() {
			this->Init();
			this->AfterInit();
			this->Run();
			this->BeforeExit();
			this->Exit();
		}
	protected:
		virtual void Init() = 0;
		virtual void Exit() = 0;
		virtual void Run() {}

		virtual void AfterInit() {}
		virtual void BeforeExit() {}

		Worker(const Worker&) = delete;
		const Worker& operator = (const Worker&) = delete;
	private:
		std::unique_ptr<std::thread> thread_;
		std::string name_;
	};
}


#endif // __WORKER_HPP__
