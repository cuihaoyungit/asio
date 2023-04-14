#ifndef __CLIENT_PROXY_HPP__
#define __CLIENT_PROXY_HPP__
#include <asio/extend/base.hpp>

namespace asio {
	class NetObject;
	typedef std::shared_ptr<NetObject> NetObjectPtr;
	typedef std::weak_ptr<NetObject>   NetObjectWeakPtr;

	typedef struct _ClientConnectProxy 
	{
		uint64 clientId;     // clientId -> sessionId
		int32 heartBeatTime; // last msg receive time
		NetObjectWeakPtr pNetObj;
	} ClientProxy;
}

#endif // __CLIENT_PROXY_HPP__
