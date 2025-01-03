﻿#ifndef __WORKER_HPP__
#define __WORKER_HPP__
#include <thread>
#include <iostream>
#include <functional>
#include <mutex>
#include <sstream>
#if defined(_WIN32)
#include <Windows.h>
#endif

namespace asio {
	class Message;
	class Worker
	{
	public:
		// https://blog.csdn.net/zhu2695/article/details/9256149
		enum Priority
		{
			Priority_Idle,
			Priority_Lowest,
			Priority_Low,
			Priority_Normal,
			Priority_High,
			Priority_Highest,
			Priority_Realtime,
		};

	public:
		Worker() {
			static std::atomic<int> thread_counter;
			this->tIndex_ = ++thread_counter;
		}

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
			//can not stop self
			if (this->id_ == std::this_thread::get_id())
			{
				return;
			}
			try {
				if (this->thread_ && thread_->joinable())
				{
					thread_->join();
				}
			}
			catch (...) { std::cout << "system error" << std::endl; }
			// save id
			this->id_= std::this_thread::get_id();
		}

		std::uint64_t threadId() {
			std::stringstream ss;
			ss << this->id_;
			std::uint64_t uid = std::stoull(ss.str());
			return uid;
		}

		int thIndex()
		{
			return this->tIndex_;
		}

		const std::string& Name() {
			return name_;
		}

		void SetName(const std::string &name) {
			this->name_ = name;
		}

		// Windows support modify thread priority
		bool SetPriority(Priority priority)
		{
			std::thread::native_handle_type handle = this->thread_->native_handle();
			bool ret = true;
#if defined(_WIN32)
			switch (priority)
			{
			case Priority_Realtime:
				ret = SetThreadPriority(handle, THREAD_PRIORITY_TIME_CRITICAL) != 0;
				break;
			case Priority_Highest:
				ret = SetThreadPriority(handle, THREAD_PRIORITY_HIGHEST) != 0;
				break;
			case Priority_High:
				ret = SetThreadPriority(handle, THREAD_PRIORITY_ABOVE_NORMAL) != 0;
				break;
			case Priority_Normal:
				ret = SetThreadPriority(handle, THREAD_PRIORITY_NORMAL) != 0;
				break;
			case Priority_Low: 
				ret = SetThreadPriority(handle, THREAD_PRIORITY_BELOW_NORMAL) != 0;
				break;
			case Priority_Lowest:
				ret = SetThreadPriority(handle, THREAD_PRIORITY_LOWEST) != 0;
				break;
			case Priority_Idle:
				ret = SetThreadPriority(handle, THREAD_PRIORITY_IDLE) != 0;
				break;
			}
#endif
			return ret;
		}
		//
		void Sleep(unsigned long msecs)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(msecs));
		}
	private:
		void RunThread() 
		{
			// update thread info
			if (this->thread_)
			{
				this->id_ = this->thread_->get_id();
			}
			this->Init();
			this->Exec();
			this->Exit();
		}
	protected:
		virtual void Init() = 0;
		virtual void Exec() = 0;
		virtual void Exit() = 0;

		Worker(const Worker&) = delete;
		const Worker& operator = (const Worker&) = delete;
	private:
		std::unique_ptr<std::thread> thread_;
		std::string name_;
		std::thread::id id_;
		int tIndex_;
	};
}


#endif // __WORKER_HPP__
