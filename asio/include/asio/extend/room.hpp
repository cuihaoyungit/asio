#ifndef __ROOM_HPP__
#define __ROOM_HPP__
#include <asio/extend/nocopyobj.hpp>
#include <asio/extend/object.hpp>
#include <asio/msgdef/message.hpp>
#include <asio/extend/base.hpp>
#include <asio/extend/snowflake>
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
		typedef std::unordered_map<uint64, NetObjectPtr> SocketObjMap; // socket     -> NetObject
		typedef std::unordered_map<uint64, NetObjectPtr> SessionObjMap;// session id -> NetObject
		typedef std::unordered_map<uint64, NetObjectPtr> UserObjMap;   // user id    -> NetObject
		Room() {}

		void Init(int workId = 1, int subId = 1)
		{
			try
			{
				this->uuid_.init(workId, subId); // default id start [1, 1]
			}
			catch (const std::exception &ex)
			{
				std::cout << ex.what() << std::endl;
			}
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
			const int64 sessionId = this->uuid_.nextid();
			obj->setSessionId(sessionId);
			session_obj_map_[sessionId] = obj;
		}

		void Leave(NetObjectPtr obj)
		{
			obj_list_.erase(obj);
			socket_obj_map_.erase(obj->SocketId());
			session_obj_map_.erase(obj->sessionId());
		}

		// find Object by socket id
		NetObjectPtr FindObjBySocketId(const uint64 &socketId)
		{
			auto it = this->socket_obj_map_.find(socketId);
			if (it != this->socket_obj_map_.end())
			{
				return it->second;
			}
			return NetObjectPtr();
		}

		// find Object by session id
		NetObjectPtr FindObjBySessionId(const uint64& sessionId)
		{
			auto it = this->session_obj_map_.find(sessionId);
			if (it != this->session_obj_map_.end())
			{
				return it->second;
			}
			return NetObjectPtr();
		}

		// find Object by user id
		NetObjectPtr FindObjByUserId(const uint64& userId)
		{
			auto it = this->user_map_.find(userId);
			if (it != this->user_map_.end())
			{
				return it->second;
			}
			return NetObjectPtr();
		}

		ObjList& GetObjList() {
			return this->obj_list_;
		}

		SessionObjMap& GetSessionMap() {
			return this->session_obj_map_;
		}

		UserObjMap& GetUserObjMap() {
			return this->user_map_;
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
		// net user
		ObjList   obj_list_;
		// user socket
		SocketObjMap socket_obj_map_;
		enum { max_recent_msgs = 100 };
		MessageQueue recent_msgs_;

		// guid snowflake
		SnowFlake uuid_;
		SessionObjMap session_obj_map_;

		// user map
		UserObjMap user_map_;
	};


}

#endif // __ROOM_HPP__
