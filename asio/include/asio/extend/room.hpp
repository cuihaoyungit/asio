#ifndef __ROOM_HPP__
#define __ROOM_HPP__
#include <asio/extend/nocopyobj.hpp>
#include <asio/extend/object.hpp>
#include <asio/msgdef/message.hpp>
#include <asio/extend/base.hpp>
#include <map>
#include <set>
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
		typedef std::unordered_map<uint64, NetObjectWeakPtr> SessionObjMap;// session id -> NetObject
		typedef std::unordered_map<uint64, NetObjectWeakPtr> UserObjMap;   // user id    -> NetObject
		Room() {}
		~Room() {}

		void Join(NetObjectPtr obj)
		{
			obj_list_.insert(obj);
			/*
			for (const auto& msg : recent_msgs_)
			{
				obj->Deliver(msg);
			}
			*/
			const uint64 sessionId = obj->getSessionId();
			obj->setSessionId(sessionId);
			session_obj_map_[sessionId] = obj;
		}

		void Leave(NetObjectPtr obj)
		{
			obj_list_.erase(obj);
			session_obj_map_.erase(obj->getSessionId());
		}

		// find Object by session id
		bool FindObjBySessionId(NetObjectWeakPtr ptr, const uint64& sessionId)
		{
			auto it = this->session_obj_map_.find(sessionId);
			if (it != this->session_obj_map_.end())
			{
				ptr = it->second;
				return true;
			}
			return false;
		}

		// find Object by user id
		bool FindObjByUserId(NetObjectWeakPtr ptr, const uint64& userId)
		{
			auto it = this->user_map_.find(userId);
			if (it != this->user_map_.end())
			{
				ptr = it->second;
				return true;
			}
			return false;
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
			{
				recent_msgs_.pop_front();
			}
			// message boardcast
			for (auto& obj : obj_list_)
			{
				if (obj)
				{
					obj->Post(msg);
				}
			}
		}

	private:
		// net user
		ObjList   obj_list_;
		enum { max_recent_msgs = 100 };
		MessageQueue recent_msgs_;

		// guid session map
		SessionObjMap session_obj_map_;

		// user map
		UserObjMap user_map_;
	};


}

#endif // __ROOM_HPP__
