//
// state.hpp
// add by [11/21/2022 cuihaoyun]
// ����״̬
//

#ifndef __STATE_HPP__
#define __STATE_HPP__

namespace asio {

	//
	// 网络连接状态
	// 
	enum class ConnectState : char
	{
		ST_STARTING, ST_STARTED, ST_CONNECTING, ST_CONNECTED, ST_STOPPING, ST_STOPPED
	};


}


#endif // __STATE_HPP__
