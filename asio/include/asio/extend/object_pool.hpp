//
// extend from object_pool >> #include <asio/detail/object_pool.hpp>
// object_pool.hpp
// add by [12/14/2022 cuihaoyun]
// thread safe object pool
// 

#ifndef __OBJECT_POOL_HPP__
#define __OBJECT_POOL_HPP__
#include <deque>
#include <asio/msgdef/message.hpp>
#include <asio/detail/io_object_impl.hpp>
#include <asio/detail/object_pool.hpp>
#include <mutex>
#include <atomic>
using namespace asio::detail;

namespace asio {

	template <typename Object>
	class ObjectPool : public object_pool<Object>
	{
	public:
		ObjectPool()  = default;
		~ObjectPool() = default;

		// constructe without args
		Object* alloc() {
			std::lock_guard lock(this->mutex_);
			return object_pool<Object>::alloc();
		}

		// constructe with args
		template <typename Arg>
		Object* alloc(Arg arg) {
			std::lock_guard lock(this->mutex_);
			return object_pool<Object>::alloc(arg);
		}

		void free(Object* o) {
			std::lock_guard lock(this->mutex_);
			object_pool<Object>::free(o);
		}

		// used size
		int live_size() {
			std::lock_guard lock(this->mutex_);
			int count = 0;
			auto obj = this->first();
			while (obj)
			{
				count++;
				obj = obj->next_;
			}
			return count;
		}
	private:
		std::mutex mutex_;
	};

}

#endif // __OBJECT_POOL_HPP__
