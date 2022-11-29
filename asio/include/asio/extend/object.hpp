//
// object.hpp
// add by [11/24/2022 cuihaoyun]
//

#ifndef __OBJECT_HPP__
#define __OBJECT_HPP__
#include <asio/msgdef/message.hpp>

namespace asio {
	class Message;
	class NetObject
	{
	public:
		NetObject() ASIO_NOEXCEPT {}
		virtual ~NetObject() {}
		virtual void deliver(const Message& msg) {}
		virtual void Send(const Message& msg) {}
		virtual uint64_t SocketId() { return 0; }
	};

	typedef std::shared_ptr<NetObject> NetObjectPtr;



}

#endif // __OBJECT_HPP__
