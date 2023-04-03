#ifndef __ROOM_HPP__
#define __ROOM_HPP__
#include <asio/extend/nocopyobj>
#include <asio/extend/object.hpp>
#include <asio/extend/snowflake>
#include <asio/msgdef/message.hpp>
#include <map>
#include <unordered_map>

namespace asio {
	// One server one room
	// Server -> Room
	class NetObject;
	class Room : protected NoCopyObj
	{
	public:
		typedef std::shared_ptr<NetObject> NetObjectPtr;
		typedef std::weak_ptr<NetObject>   NetObjectWeakPtr;

		typedef std::set<NetObjectPtr> ObjList;
		typedef std::unordered_map<uint64_t, NetObjectPtr> SocketObjMap; // socket -> NetObject
		typedef std::unordered_map<uint64_t, NetObjectPtr> SessionObjMap;// session id -> NetObject
		Room() {
			this->uuid_.init(1, 1);
		}

		void Join(NetObjectPtr obj)
		{
			obj_list_.insert(obj);
			socket_obj_map_[obj->SocketId()] = obj;
#if 0
			for (const auto& msg : recent_msgs_)
				obj->Deliver(msg);
#endif
			// generator guid for session id
			uint64_t sessionId = this->uuid_.nextid();
			obj->setSessionId(sessionId);
			session_obj_map_[sessionId] = obj;
		}

		void Leave(NetObjectPtr obj)
		{
			obj_list_.erase(obj);
			socket_obj_map_.erase(obj->SocketId());
			session_obj_map_.erase(obj->sessionId());
		}

		NetObjectPtr FindSocketObj(const uint64_t &id)
		{
			auto it = this->socket_obj_map_.find(id);
			if (it != this->socket_obj_map_.end())
			{
				return it->second;
			}
			return nullptr;
		}

	private:
		void Deliver(const Message& msg)
		{
			while (recent_msgs_.size() > max_recent_msgs)
				recent_msgs_.pop_front();
			for (auto& obj : obj_list_)
				obj->Deliver(msg);

		}

	private:
		ObjList   obj_list_;
		SocketObjMap socket_obj_map_;
		enum { max_recent_msgs = 100 };
		MessageQueue recent_msgs_;

		// guid snowflake
		SnowFlake uuid_;
		SessionObjMap session_obj_map_;
	};


}

#endif // __ROOM_HPP__
