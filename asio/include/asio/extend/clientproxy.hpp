#ifndef __CLIENT_PROXY_HPP__
#define __CLIENT_PROXY_HPP__

namespace asio {
	class NetObject;
	typedef std::shared_ptr<NetObject> NetObjectPtr;
	typedef std::weak_ptr<NetObject>   NetObjectWeakPtr;

	typedef struct _ClientConnectProxy 
	{
		uint64_t clientId; // clientId -> sessionId
		int heartBeatTime; // last msg receive time
		NetObjectWeakPtr pNetObj;
	} ClientProxy;
}

#endif // __CLIENT_PROXY_HPP__
