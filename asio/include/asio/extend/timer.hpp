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

	class AsioTimer : public Worker
	{
	public:
		AsioTimer() ASIO_NOEXCEPT
			:io_worker_(ioc_) {}
		~AsioTimer() {}

		void Run() override {
			ioc_.run();
		}

		void Stop() {
			if (!io_worker_.get_io_context().stopped())
			{
				io_worker_.get_io_context().stop();
			}
			this->Clear();
		}

		asio::io_context& GetIoContext() {
			return ioc_;
		}

		auto GetTimer(const std::string &key_name) -> std::shared_ptr<asio::steady_timer> {
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
			auto it = timer_list_.find(key_name);
			if (it != timer_list_.end()) {
				this->timer_list_.erase(it);
			}
		}
	protected:
		void Clear() {
			timer_list_.clear();
		}

	private:
		asio::io_context ioc_;
		asio::io_context::work io_worker_;
		using io_timer = std::shared_ptr<asio::steady_timer>;
		std::unordered_map<std::string, io_timer> timer_list_;
	};

}


#endif // __TIMER_HPP__
