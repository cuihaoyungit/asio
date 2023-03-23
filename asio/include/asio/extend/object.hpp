//
// object.hpp
// add by [11/24/2022 cuihaoyun]
//

#ifndef __OBJECT_HPP__
#define __OBJECT_HPP__
#include <asio/msgdef/message.hpp>
#include <functional>
#include <unordered_map>
#include <memory>
#include <iostream>
#include <map>

namespace asio {

	// NetGroup interface
	//--------------------------------------------------------------
	// NetObject
	class Message;
	class NetObject
	{
	public:
		NetObject() noexcept:is_connect_(false), type(0) {}
		virtual ~NetObject()
		{
			userdata.clear();
		}
		virtual void Final() {}
		virtual void Deliver(const Message& msg) {}
		virtual void Send(const Message& msg) {};
		virtual void Post(const Message& msg) {};
		virtual void PostMsg(const Message& msg) {}

		virtual uint64_t SocketId() = 0;
		virtual void Close(){}
		void SetType(const int type) {
			this->type = type;
		}
		void SetConnectName(const std::string &name) {
			this->connectName = name;
		}
		std::string& GetConnectName() {
			return this->connectName;
		}
		void SetUserData(const std::string &key, const std::string &value) {
			this->userdata[key] = value;
		}
		bool UserData(const std::string &key, std::string &out) {
			auto it = userdata.find(key);
			if (it != userdata.end())
			{
				out = it->second;
				return true;
			}
			return false;
		}
		bool IsConnect() {
			return is_connect_;
		}
		void SetConnect(const bool bConnect) {
			this->is_connect_ = bConnect;
		}

	private:
		bool is_connect_ = {false};
		int type;
		std::string connectName;

		typedef std::unordered_map<std::string, std::string> UserDataList;
		UserDataList userdata;
	};

	typedef std::shared_ptr<NetObject> NetObjectPtr;
	typedef std::weak_ptr<NetObject>   NetObjectWeakPtr;

	//--------------------------------------------------------------
	// NetClient Event
	class NetClient
	{
	public:
		NetClient() = default;
		virtual ~NetClient() {}
		virtual void Init() {}
		virtual void Exit() {}
		virtual void Connect(NetObject* pNetObj) {}
		virtual void Disconnect(NetObject* pNetObj) {}
		virtual void HandleMessage(NetObject* pNetObj, const Message& msg) {}
		virtual void Reconnect() {}
	};

	//--------------------------------------------------------------
	// NetServer Event
	class NetServer
	{
	public:
		NetServer() = default;
		virtual ~NetServer() {}
		virtual void Init() {}
		virtual void Exit() {}
		virtual void Final() {}
		virtual void Connect(NetObjectPtr pNetObj) {}
		virtual void Disconnect(NetObjectPtr pNetObj) {}
		virtual void HandleMessage(Message& msg) {}
		virtual void PostMsg(const Message& msg) {}
	};

	//--------------------------------------------------------------
	// Dispatcher
	class Router;
	class Dispatcher
	{
	public:
		typedef std::function<void(Message*)> TaskCallback;
		typedef std::unordered_map<int, TaskCallback> TaskList;

		Dispatcher() {}
		virtual ~Dispatcher() 
		{
		}
		virtual void Register() {}

		bool BindMsg(const int& MsgId, TaskCallback fun) 
		{
			const auto &it = m_taskList.find(MsgId);
			if (it == m_taskList.end()) {
				m_taskList[MsgId] = fun;
				return true;
			}
			return false;
		}
	
		TaskList& GetTaskList()
		{
			return m_taskList;
		}

		TaskCallback FindCallback(const int &key) {
			auto it = m_taskList.find(key);
			if (it != m_taskList.end())
			{
				return it->second;
			}
			return nullptr;
		}
	private:
		TaskList m_taskList;
	};

}

#endif // __OBJECT_HPP__
