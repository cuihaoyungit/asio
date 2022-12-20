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
	class Message;
	class NetObject
	{
	public:
		NetObject() noexcept {}
		virtual ~NetObject()
		{
			userdata.clear();
		}
		virtual void deliver(const Message& msg) {}
		virtual void Send(const Message& msg) {}
		virtual uint64_t SocketId() { return 0; }
		virtual void Close(){}
		void setType(const int type) {
			this->type = type;
		}
		void setConnectName(const std::string &name) {
			this->connectName = name;
		}
		void setUserData(const std::string &key, const std::string &value) {
			this->userdata[key] = value;
		}
		bool userData(const std::string &key, std::string &out) {
			auto it = userdata.find(key);
			if (it != userdata.end())
			{
				out = it->second;
				return true;
			}
			return false;
		}

	private:
		int type = {0};
		std::string connectName;
		std::map<std::string, std::string> userdata;
	};

	typedef std::shared_ptr<NetObject> NetObjectPtr;

	//--------------------------------------------------------------
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
	};

	//--------------------------------------------------------------
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
	class Router;
	class Dispatcher
	{
	public:
		Dispatcher():m_router(nullptr) {}
		virtual ~Dispatcher() 
		{
			if (m_router)
				delete m_router;
		}
		virtual void Register() {}
		virtual void SetRouter(Router *router)
		{
			m_router = router;
		}
		virtual bool BindMsg(const int& MsgId, std::function<void(Message*)> fun) 
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
		Router* m_router;
	};


	//--------------------------------------------------------------
	class Router
	{
	public:
		Router() = default;
		virtual ~Router() {}
	};




}

#endif // __OBJECT_HPP__
