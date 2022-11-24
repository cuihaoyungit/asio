//
// object.hpp
// add by [11/24/2022 cuihaoyun]
//

#ifndef __OBJECT_HPP__
#define __OBJECT_HPP__
#include <asio/msgdef/message.hpp>

namespace asio {
	class message;
	class NetObject
	{
	public:
		NetObject() ASIO_NOEXCEPT {}
		virtual ~NetObject() {}
		virtual void deliver(const message& msg) {}
		virtual void Send(const message& msg) {}
		virtual uint64_t SocketId() { return 0; }
	};
}

#endif // __OBJECT_HPP__
