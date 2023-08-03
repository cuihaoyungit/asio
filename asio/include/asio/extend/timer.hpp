//
// timer.hpp
// add by [11/21/2022 cuihaoyun]
// wrap cpp11 thread

#ifndef __TIMER_HPP__
#define __TIMER_HPP__
#include <asio/extend/worker.hpp>
#include <asio/time_traits.hpp>
#include <asio/io_context.hpp>
#include <asio/steady_timer.hpp>
#include <unordered_map>
#include <optional>
#include <cstddef>
#include <iostream>

namespace asio {

	//
	class Timer
	{
		using clock_type =
			std::chrono::system_clock;

		clock_type::time_point when_;

	public:
		using duration =
			clock_type::duration;

		Timer()
			: when_(clock_type::now())
		{
		}

		duration
			elapsed() const
		{
			return clock_type::now() - when_;
		}
	};
	// duration_cast<milliseconds>(t.elapsed()).count() << "ms";

	// Async timer
	class AsioTimer : public Worker
	{
		using io_timer = std::shared_ptr<asio::steady_timer>;
	public:
		AsioTimer() noexcept {}
		~AsioTimer() {}

		void Stop() {
			if (!this->ioc_.stopped())
			{
				this->ioc_.stop();
			}
			this->Clear();
			this->WaitStop();
		}

		asio::io_context& GetContext() {
			return ioc_;
		}

		auto GetTimer(const std::string &key_name) -> io_timer {
			auto it = timer_list_.find(key_name);
			if (it != timer_list_.end())
			{
				return it->second;
			}
			return nullptr;
		}

		auto& InsertTimer(const std::string &key_name) {
			auto it = timer_list_.find(key_name);
			if (it != timer_list_.end())
			{
				return it->second;
			}
			std::shared_ptr<asio::steady_timer> timer(new asio::steady_timer(ioc_));
			this->timer_list_[key_name] = std::move(timer);
			return this->timer_list_[key_name];
		}

		void RemoveTimer(const std::string &key_name) {
			const auto it = timer_list_.find(key_name);
			if (it != timer_list_.end()) {
				this->timer_list_.erase(it);
			}
		}

	protected:
		void Run() override {
			this->Init();
			ioc_.run();
			this->Exit();
		}

		void Clear() {
			timer_list_.clear();
		}

	private:
		asio::io_context ioc_;
		std::unordered_map<std::string, io_timer> timer_list_;
	};

}


#endif // __TIMER_HPP__
