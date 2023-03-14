//
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
			this->WaitStop();
		}

		virtual void Run() {}

		bool Startup() {
			if (!thread_) {
				thread_ = std::make_unique<std::thread>(std::bind(&Worker::RunThread, this));
			}
			return true;
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
			this->Run();
			this->Exit();
		}
		virtual void Init() {}
		virtual void Exit() {}
		virtual void Tick() {}
	protected:
		virtual void HandleMessage(Message* msg) {}

		Worker(const Worker&) = delete;
		const Worker& operator = (const Worker&) = delete;
	private:
		std::unique_ptr<std::thread> thread_;
		std::string name_;
	};
}


#endif // __WORKER_HPP__
