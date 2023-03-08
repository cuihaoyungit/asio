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
		virtual void Deliver(const Message& msg) {}
		virtual void Send(const Message& msg) {};
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

	//--------------------------------------------------------------
	// NetClientEvent
	class NetClientEvent
	{
	public:
		NetClientEvent() = default;
		virtual ~NetClientEvent() {}
		virtual void Init() {}
		virtual void Exit() {}
		virtual void Connect(NetObject* pObject) {}
		virtual void Disconnect(NetObject* pObject) {}
		virtual void HandleMessage(NetObject* pObject, const Message& msg) {}
		virtual void PostMsg(const Message& msg) {}
		virtual void Reconnect() {}
	};

	//--------------------------------------------------------------
	// NetServerEvent
	class NetServerEvent
	{
	public:
		NetServerEvent() = default;
		virtual ~NetServerEvent() {}
		virtual void Init() {}
		virtual void Exit() {}
		virtual void Connect(NetObjectPtr pObj) {}
		virtual void Disconnect(NetObjectPtr pObj) {}
		virtual void HandleMessage(Message& msg) {}
		virtual void PostMsg(const Message& msg) {}
	};

	//--------------------------------------------------------------
	// Dispatcher
	class Router;
	class Dispatcher
	{
	public:
		Dispatcher() {}
		virtual ~Dispatcher() 
		{
			for (auto& [key, value] : m_routers)
			{
				delete value;
			}
			m_routers.clear();
		}
		virtual void Register() {}
		void SetRouter(const std::string &key, Router *router)
		{
			const auto it = this->m_routers.find(key);
			if (it != m_routers.end())
			{
				return;
			}
			m_routers[key] = router;
		}
		Router* GetRouter(const std::string& key) {
			const auto it = this->m_routers.find(key);
			if (it != m_routers.end())
			{
				return it->second;
			}
			return nullptr;
		}
		bool BindMsg(const int& MsgId, std::function<void(Message*)> fun) 
		{
			const auto &it = m_funs.find(MsgId);
			if (it == m_funs.end()) {
				m_funs[MsgId] = fun;
				return true;
			}
			return false;
		}
	protected:
		std::unordered_map<int, std::function<void(Message*)> > m_funs;
		std::unordered_map<std::string, Router*> m_routers;
	};


	//--------------------------------------------------------------
	// Router
	class Router
	{
	public:
		Router() = default;
		virtual ~Router() {}
	};




}

#endif // __OBJECT_HPP__
