#ifndef __OBJECT_HPP__
#define __OBJECT_HPP__
#include <asio/msgdef/message.hpp>
#include <asio/extend/base.hpp>
#include <asio/extend/snowflake.hpp>
#include <mutex>
using SnowFlake = snowflake<1534832906275L, std::mutex>;
#include <functional>
#include <unordered_map>
#include <memory>
#include <iostream>
#include <map>
#include <cassert>

namespace asio {

	// NetGroup interface
	//--------------------------------------------------------------
	// NetObject
	class Message;
	class NetObject
	{
	public:
		typedef std::unordered_map<std::string, std::string> UserDataList;

		NetObject() noexcept
			: is_connect_(false)
			, session_id_(0)
			, proxy_id_(0)
			, user_id_(0)
			, update_time_(0)
			, is_heartbeat_(false)
			, msg_queue_running_(true)
		{
			this->setUpdateTime(time(nullptr));
		}
		virtual ~NetObject()
		{
			userdata.clear();
		}
		virtual void Send(const Message& msg) { assert(!"virtual function."); }
		virtual void Post(const Message& msg) { assert(!"virtual function."); }
		virtual std::string Ip()   { return ""; }
		virtual std::string Port() { return ""; }
		// disconnect
		virtual void Close() {}
		// cache msg queue
		virtual void SetMsgQueueRun(bool bEnable) { this->msg_queue_running_ = bEnable; }
		// is msg queue running ?
		virtual bool IsMsgQueueRunning() { return this->msg_queue_running_; }
		// connect name
		void SetConnectName(const std::string &name)
		{
			this->connectName = name;
		}
		std::string& GetConnectName()
		{
			return this->connectName;
		}
		void SetUserData(const std::string &key, const std::string &value)
		{
			this->userdata[key] = value;
		}
		bool UserData(const std::string &key, std::string &out)
		{
			auto it = userdata.find(key);
			if (it != userdata.end())
			{
				out = it->second;
				return true;
			}
			return false;
		}
		bool IsConnect()
		{
			return is_connect_;
		}
		void SetConnect(const bool bConnect)
		{
			if (this->is_connect_ != bConnect)
			{
				this->is_connect_ = bConnect;
			}
		}
		void setSessionId(const uint64& sessionId)
		{
			this->session_id_ = sessionId;
		}
		const uint64 getSessionId() const
		{
			return this->session_id_;
		}
		void setProxyId(const int32& proxyId)
		{
			this->proxy_id_ = proxyId;
		}
		const int32 getProxyId() const
		{
			return this->proxy_id_;
		}
		void setUserId(const uint64& userId)
		{
			this->user_id_ = userId;
		}
		const uint64 getUserId()
		{
			return this->user_id_;
		}
		std::string Address()
		{
			return remote_address_;
		}
		void setAddress(const std::string& ip)
		{
			this->remote_address_ = ip;
		}
		void setUpdateTime(const time_t& time)
		{
			this->update_time_ = time;
		}
		time_t getUpdateTime()
		{
			return this->update_time_;
		}
		void setHeartbeat(bool value)
		{
			this->is_heartbeat_ = value;
		}
		bool isHeartbeat()
		{
			return this->is_heartbeat_;
		}
	private:
		bool is_connect_;
		std::string connectName;
		UserDataList userdata;
		uint64 session_id_;
		uint64 user_id_;
		int32 proxy_id_;
		std::string remote_address_;
		time_t update_time_;
		bool is_heartbeat_;
		bool msg_queue_running_;

	};

	//typedef std::shared_ptr<NetObject> NetObjectPtr;
	//typedef std::weak_ptr<NetObject>   NetObjectWeakPtr;

	//--------------------------------------------------------------
	// NetClient Event
	class NetEvent
	{
	public:
		NetEvent() = default;
		virtual ~NetEvent() {}
		virtual void Post(const Message& msg) = 0;
		virtual void Send(const Message& msg) = 0;
		virtual void Connect(NetObjectPtr pNetObj)    = 0;
		virtual void Disconnect(NetObjectPtr pNetObj) = 0;
		virtual void HandleMessage(NetObjectPtr pNetObj, const Message& msg)= 0;
		virtual void Reconnect(NetObjectPtr pNetObj) {}
		virtual void Error(int error) = 0;
	};

	//--------------------------------------------------------------
	// NetServer Event
	class NetServer
	{
	public:
		NetServer():main_id_(0),sub_id_(0), is_pack_session_id_(false) {}
		virtual ~NetServer() {}
		virtual void Connect(NetObjectPtr pNetObj)    = 0;
		virtual void Disconnect(NetObjectPtr pNetObj) = 0;
		virtual void HandleMessage(NetObjectPtr pNetObj, const Message& msg)= 0;
		virtual void Error(int error)                 = 0;
	public:
		// Server ID card
		const int MainId() const
		{
			return main_id_;
		}
		void SetMainId(const int id)
		{
			this->main_id_ = id;
		}
		// Server sub ID card
		const int SubId() const
		{
			return sub_id_;
		}
		void SetSubId(const int subId)
		{
			this->sub_id_ = subId;
		}
		bool IsPackSessionId()
		{
			return this->is_pack_session_id_;
		}
		void SetPackSessionId(bool bPack)
		{
			this->is_pack_session_id_ = bPack;
		}

		void InitUUID(const int mainId, const int subId)
		{
			try {
				this->uuid_.init(mainId, subId); // default id start [1, 1]
			}
			catch (const std::exception& ex)
			{
				std::cout << ex.what() << std::endl;
			}
		}
		uint64 GenerateUUId()
		{
			const uint64 sessionId = this->uuid_.nextid();
			return sessionId;
		}
	private:
		int main_id_; // app id
		int sub_id_;
		bool is_pack_session_id_;
	private:
		// guid snowflake
		SnowFlake uuid_;
	};

	//--------------------------------------------------------------
	// Dispatcher
	class Dispatcher
	{
	public:
		typedef std::function<void(Message*)> Task;
		typedef std::unordered_map<int, Task> TaskMap;

		Dispatcher() {}
		virtual ~Dispatcher() {}

		bool BindMsg(const int& MsgId, Task fun) 
		{
			const auto &it = m_taskList.find(MsgId);
			if (it == m_taskList.end()) {
				m_taskList[MsgId] = fun;
				return true;
			}
			return false;
		}
	
		TaskMap& GetTaskList()
		{
			return m_taskList;
		}

		Task FindCallback(const int &key)
		{
			auto it = m_taskList.find(key);
			if (it != m_taskList.end())
			{
				return it->second;
			}
			return nullptr;
		}
	private:
		TaskMap m_taskList;
	};

}

#endif // __OBJECT_HPP__
