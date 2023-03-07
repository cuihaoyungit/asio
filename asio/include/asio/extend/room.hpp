//
// room.hpp
// add by [11/25/2022 cuihaoyun]
//

#ifndef __ROOM_HPP__
#define __ROOM_HPP__
#include <asio/msgdef/message.hpp>

namespace asio {

	class Room
	{
	public:
		void Join(NetObjectPtr obj)
		{
			std::lock_guard lock(mutex_);
			obj_list_.insert(obj);
			for (const auto& msg : recent_msgs_)
				obj->Deliver(msg);
		}

		void Leave(NetObjectPtr obj)
		{
			std::lock_guard lock(mutex_);
			obj_list_.erase(obj);
		}

		void Deliver(const Message& msg)
		{
			recent_msgs_.push_back(msg);
			while (recent_msgs_.size() > max_recent_msgs)
				recent_msgs_.pop_front();

			for (auto& obj : obj_list_)
				obj->Deliver(msg);
		}

	private:
		std::mutex mutex_;
		std::set<NetObjectPtr> obj_list_;
		enum { max_recent_msgs = 100 };
		MessageQueue recent_msgs_;
	};


}

#endif // __ROOM_HPP__
