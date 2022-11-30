//
// object.hpp
// add by [11/24/2022 cuihaoyun]
//

#ifndef __OBJECT_HPP__
#define __OBJECT_HPP__
#include <asio/msgdef/message.hpp>

namespace asio {

	// NetGroup interface
	//--------------------------------------------------------------
	class Message;
	class NetObject
	{
	public:
		NetObject() ASIO_NOEXCEPT {}
		virtual ~NetObject() {}
		virtual void deliver(const Message& msg) {}
		virtual void Send(const Message& msg) {}
		virtual uint64_t SocketId() { return 0; }
		virtual void Close(){}
	};

	typedef std::shared_ptr<NetObject> NetObjectPtr;

	//--------------------------------------------------------------
	class NetClientEvent
	{
	public:
		NetClientEvent() = default;
		virtual ~NetClientEvent() {}
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
		virtual void Connect(NetObject* pObj) {}
		virtual void Disconnect(NetObject* pObj) {}
		virtual void HandleMessage(Message& msg) {}
		virtual void PostMsg(const Message& msg) {}
	};


}

#endif // __OBJECT_HPP__
